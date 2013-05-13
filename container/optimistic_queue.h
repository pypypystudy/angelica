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
		node() : next(0), prev(0){}
		node(T & _data) : data(_data), next(0), prev(0){}
		~node(){}

		T data;
		boost::atomic<void *> next, prev;
	};

	struct list{
		boost::atomic<node *> head;
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

		detail::_hazard_ptr<node> _ptr_head = _hsys.acquire();
		detail::_hazard_ptr<node> _ptr_next = _hsys.acquire();
		while(1){
			_ptr_head->_hazard = _list->head.load();
			_ptr_next->_hazard = _ptr_head->_hazard->next.load();

			_new->next = _ptr_next->_hazard;
			_new->prev = _ptr_head->_hazard;

			if (_ptr_head->_hazard != _list->head.load()){
				continue;
			}

			if (_list->head.compare_exchange_weak(_ptr_head->_hazard, _new)){
				_ptr_next->_hazard->prev.store(_new);
				_list->size++;
				break;
			}
		}

		_hsys.release(_ptr_detail);
		_hsys.release(_ptr_prev);
	}

	bool pop_front(T & data){
		bool ret = true;
		
		detail::_hazard_ptr<node> _ptr_head = _hsys.acquire();
		detail::_hazard_ptr<node> _ptr_next = _hsys.acquire();
		while(1){
			_ptr_head->_hazard = _list->head.load();
			_ptr_next->_hazard = _ptr_head->_hazard->next.load();

			if (_ptr_next->_hazard == 0){
				ret = false;
				break;
			}

			if (_ptr_head->_hazard != _list->head.load()){
				continue;
			}
		
			if (_list->head.compare_exchange_weak(_ptr_head->_hazard, _ptr_next->_hazard)){
				_ptr_next->_hazard->prev.store(0);
				data = _ptr_next->_hazard->data;

				_hsys.retire(_ptr_head->_hazard, boost::bind(&optimistic_queue::put_node, this, _1));
				_list->size--;
				
				break;
			}
		}

		_hsys.release(_ptr_head);
		_hsys.release(_ptr_next);

		return ret;
	}

	void push_back(const T & data){
		node * _new = get_node(data);

		detail::_hazard_ptr<node> _ptr_detail = _hsys.acquire();
		while(1){
			_ptr_detail->_hazard = _list->detail.load();

			if(_ptr_detail->_hazard->next != 0){
				_list->detail.store(_ptr_detail->_hazard->next);
				continue;
			}

			_new->prev = _ptr_detail->_hazard;

			if (_ptr_detail->_hazard != _list->detail.load()){
				continue;
			}

			node * endnode = 0;
			if (_ptr_detail->_hazard->next.compare_exchange_weak(endnode, _new)){
				_list->detail.compare_exchange_weak(_ptr_detail, _new);
				_list->size++;
				break;
			}
		}
	}

	bool pop_back(T & data){
		bool ret = true;
		
		detail::_hazard_ptr<node> _ptr_detail = _hsys.acquire();
		detail::_hazard_ptr<node> _ptr_prev = _hsys.acquire();
		while(1){
			_ptr_detail->_hazard = _list->detail.load();
			_ptr_prev->_hazard = _ptr_detail->_hazard->prev.load();
			
			if(_ptr_detail->_hazard->next != 0){
				_list->detail.store(_ptr_detail->_hazard->next);
				continue;
			}

			if (_ptr_detail->_hazard != _list->detail.load()){
				continue;
			}

			if (_ptr_prev->_hazard == _list->head.load()){
				ret = false;
				break;
			}

			if (_ptr_detail->_hazard != _list->detail.load()){
				continue;
			}
		
			if (_ptr_prev->_hazard->next.compare_exchange_weak(_ptr_detail->_hazard, 0)){
				data = _ptr->_hazard->data;
				
				_list.detail.compare_exchange_weak(_ptr_detail->_hazard, _ptr_prev->_hazard);
				
				_hsys.retire(_ptr_detail->_hazard, boost::bind(&optimistic_queue::put_node, this, _1));
				_list->size--;
				
				break;
			}
		}

		_hsys.release(_ptr_detail);
		_hsys.release(_ptr_prev);

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
		_list->head = _node;

		return _list;
	}

	void put_list(list * _list){
		node * _next = _list->detail.load();
		do{
			node * _tmpnext = _next;
			_next = _tmpnext->next.load();
			put_node(_tmpnext);
		}while(_next != 0);
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