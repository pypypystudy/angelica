/*
 * optimistic_queue.h
 * Created on: 2013-5-6
 *	   Author: qianqians
 * optimistic_queue:lock-free deque
 */
#ifndef _OPTIMISTIC_QUEUE_H
#define _OPTIMISTIC_QUEUE_H

#include <boost/atomic.hpp>
#include <boost/pool/pool_alloc.hpp>

#include <angelica/container/detail/_hazard_ptr.h>

namespace angelica{
namespace container{

template<class T, class _Ax = boost::pool_allocator<T> >
class optimistic_queue{
private:
	struct node{
		node(){}
		node(T & _data) : data(_data){}
		~node(){}

		T data;
		boost::atomic<void *> next, prev;
	};

	struct list{
		boost::atomic<node *> detail;
		boost::atomic_uint32_t size;
	};

	typedef typename _Ax::template rebind<node>::other _Alloc_node; 
	typedef typename _Ax::template rebind<list>::other _Alloc_list;

public:
	optimistic_queue(){
		_list.store(get_list());
	}

	~optimistic_queue(){
		put_list(_list.load());
	}

	void clear(){
		list * _tmplist = get_list();
		_tmplist = _list.exchange(_tmplist);
		put_list(_tmplist);
	}

	size_t size(){
		return _list->size.load();
	}

	void push_front(const T & data){
		node * _new = get_node(data);

		detail::_hazard_ptr<node> _ptr_detail = _hsys.acquire();
		detail::_hazard_ptr<node> _ptr_prev = _hsys.acquire();
		while(1){
			_ptr_detail->_hazard = _list->detail.load();
			_ptr_prev->_hazard = _ptr_detail->_hazard->prev.load();

			_new->next = _ptr_detail->_hazard;
			_new->prev = _ptr_prev->_hazard;

			if (_ptr_detail->_hazard != _list->detail.load()){
				continue;
			}

			if (_ptr_prev->_hazard->next.compare_exchange_weak(_ptr_detail->_hazard, _new)){
				_ptr_detail->_hazard->prev.store(_new);
				_list->size++;
				break;
			}
		}

		_hsys.release(_ptr_detail);
		_hsys.release(_ptr_prev);
	}

	bool pup_front(T & data){
		bool ret = true;
		
		detail::_hazard_ptr<node> _ptr_detail = _hsys.acquire();
		detail::_hazard_ptr<node> _ptr_prev = _hsys.acquire();
		detail::_hazard_ptr<node> _ptr_next = _hsys.acquire();
		while(1){
			_ptr_detail->_hazard = _list->detail.load();
			_ptr_prev->_hazard = _ptr_detail->_hazard->prev.load();
			_ptr_next->_hazard = _ptr_detail->_hazard->next.load();

			if (_ptr_detail->_hazard != _list->detail.load()){
				continue;
			}

			if (_ptr_detail->_hazard == _ptr_next->_hazard){
				ret = false;
				break;
			}

			if (_ptr_detail->_hazard != _list->detail.load()){
				continue;
			}
		
			if (_ptr_prev->_hazard->next.compare_exchange_weak(_ptr_detail->_hazard, _ptr_next->_hazard)){
				_ptr_next->_hazard->prev.store(_ptr_prev->_hazard);
				data = _ptr->_hazard->data;
				_list.detail.compare_exchange_weak(_ptr_detail->_hazard, _ptr_prev->_hazard);
				_hsys.retire(_ptr_detail->_hazard, boost::bind(&optimistic_queue::put_node, this, _1));
				_list->size--;
				break;
			}
		}

		_hsys.release(_ptr_detail);
		_hsys.release(_ptr_prev);
		_hsys.release(_ptr_next);

		return ret;
	}

	void push_back(const T & data){
		node * _new = get_node(data);
		_new->next.store(_list->detail);

		detail::_hazard_ptr<node> _ptr_detail = _hsys.acquire();
		detail::_hazard_ptr<node> _ptr_next = _hsys.acquire();
		while(1){
			_ptr->_hazard = _list->detail.load();
		}
	}

	bool pop_back(T & data){
		bool ret = true;
		
		detail::_hazard_ptr<node> _ptr_detail = _hsys.acquire();
		detail::_hazard_ptr<node> _ptr_prev = _hsys.acquire();
		detail::_hazard_ptr<node> _ptr_next = _hsys.acquire();
		while(1){
			_ptr_detail->_hazard = _list->detail.load();
			_ptr_prev->_hazard = _ptr_detail->_hazard->prev.load();
			_ptr_next->_hazard = _ptr_detail->_hazard->next.load();

			if (_ptr_detail->_hazard != _list->detail.load()){
				continue;
			}

			if (_ptr_detail->_hazard == _ptr_next->_hazard){
				ret = false;
				break;
			}

			if (_ptr_detail->_hazard != _list->detail.load()){
				continue;
			}
		
			if (_ptr_prev->_hazard->next.compare_exchange_weak(_ptr_detail->_hazard, _ptr_next->_hazard)){
				_ptr_next->_hazard->prev.store(_ptr_prev->_hazard);
				data = _ptr->_hazard->data;
				_list.detail.compare_exchange_weak(_ptr_detail->_hazard, _ptr_next->_hazard);
				_hsys.retire(_ptr_detail->_hazard, boost::bind(&optimistic_queue::put_node, this, _1));
				_list->size--;
				break;
			}
		}

		_hsys.release(_ptr_detail);
		_hsys.release(_ptr_prev);
		_hsys.release(_ptr_next);

		return ret;
	}

private:
	node * get_node(){
		node * _node = _alloc_node.allocator(1);
		::new (_node) node();

		return _node;
	}

	node * get_node(T & data){
		node * _node = _alloc_node.allocator(1);
		::new (_node) node(data);

		return _node;
	}

	void put_node(node * _node){
		_node->~node();
		_alloc_node.deallocator(_node, 1);
	}

	list * get_list(){
		list * _list = _alloc_list.allocator(1);
		::new (_list) list();

		_list->size.store(0);

		node * _node = get_node();
		_list->detail = _node;
		_list->detail->next.store(_node);
		_list->detail->prev.store(_node);

		return _list;
	}

	void put_list(list * _list){
		node * _next = 0;
		do{
			node * _next = _list->detail->next;
			put_node(_next);
		}while(_next != _list->detail);
	}

private:
	boost::atomic<list *> _list;

	_Alloc_node _alloc_node;
	_Alloc_list _alloc_list;

	detail::_hazard_system<node> _hsys;

};

}// container
}// angelica

#endif //_OPTIMISTIC_QUEUE_H