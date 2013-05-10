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
		node * detail;
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

	void push(const T & data){
		node * _new = get_node(data);
		_new->next.store(_list->detail);

		detail::_hazard_ptr<node> _ptr = _hsys.acquire();
		while(1){
			_ptr->_hazard = _list->detail->prev.load();
			
			_new->prev = _ptr->_hazard;

			if (_ptr->_hazard != _list->detail->prev.load()){
				continue;
			}

			node * _detail = _list->detail;
			if (_ptr->_hazard->next.compare_exchange_weak(_detail, _new)){
				_list->detail->prev.store(_new);
				_list->size++;
				break;
			}
		}

		_hsys.release(_ptr);
	}

	bool pop(T & data){
		bool ret = true;

		detail::_hazard_ptr<node> _ptr = _hsys.acquire();
		detail::_hazard_ptr<node> _ptr_next = _hsys.acquire();
		while(1){
			_ptr->_hazard = _list->detail->next.load();
			_ptr_next->_hazard = _ptr->_hazard->next.load();

			if (_ptr->_hazard == _ptr_next->_hazard){
				ret = false;
				break;
			}

			if (_ptr->_hazard != _list->detail->next.load()){
				continue;
			}

			if (_ptr_next->_hazard->prev.compare_exchange_strong(_ptr->_hazard,  _list->detail)){
				_list->detail->next.store(_ptr_next->_hazard);
				data = _ptr->_hazard->data;
				_hsys.retire(_ptr, boost::bind(&optimistic_queue::put_node, this, _1));
				_list->size--;
				break;
			}
		}

		_hsys.release(_ptr);
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