/*
 * xorlist.h
 * Created on: 2013-5-6
 *	   Author: qianqians
 * xorlist
 */
#ifndef _XORLIST_H
#define _XORLIST_H

#include <boost/atomic.hpp>
#include <boost/pool/pool_alloc.hpp>

#include <angelica/container/detail/_hazard_ptr.h>

namespace angelica{
namespace container{

template<class T, class _Ax = boost::pool_allocator<T> >
class xorlist{
private:
	struct node{
		node(){}
		node(T & _data) : data(_data){}
		~node(){}

		T data;
		boost::atomic<void *> xor;
	};

	struct list{
		boost::atomic<node *> begin;
		boost::atomic<node *> end;
		boost::atomic_uint32_t size;
	};

	typedef typename _Ax::template rebind<node>::other _Alloc_node; 
	typedef typename _Ax::template rebind<list>::other _Alloc_list;

public:
	xorlist(){
		_list.store(get_list());
	}

	~xorlist(){
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
		node * _node = get_node(data);
		
		detail::_hazard_ptr<node> _ptr = _hsys.acquire();
		while(1){
			_ptr._hazard = _list->begin.load();
			
			void * xorold = _ptr._hazard->xor.load();
			void * xor = (xorold^0)^_node;

			if (_ptr._hazard != _list->begin.load()){
				continue;
			}

			if (_ptr._hazard->xor.compare_exchange_weak(xorold, xor)){
				_node->xor.store(_ptr._hazard^0);
				break;
			}
		}
		_list->begin.compare_exchange_weak(_ptr._hazard, _node);

		_hsys.release(_ptr);
	}

	bool pop_front(T & data){
		bool ret = true;

		detail::_hazard_ptr<node> _ptr = _hsys.acquire();
		detail::_hazard_ptr<node> _ptr_next = _hsys.acquire();
		while(1){
			_ptr->_hazard = _list->begin.load();
			_ptr_next->_hazard = (_ptr->_hazard->xor.load())^0;
			
			if (_ptr_next->_hazard == 0){
				ret = false;
				break;
			}

			if (_ptr->_hazard != _list->begin.load()){
				continue;
			}

			void * xorold = _ptr_next->_hazard->xor.load();
			void * xor = (xorold^(_ptr->_hazard.load()))^0;

			if (_ptr->_hazard != _list->begin.load()){
				continue;
			}

			if (_ptr_next->_hazard->xor.compare_exchange_weak(xorold, xor)){
				data = _ptr_next->_hazard->data;
				_list->begin.store(_ptr_next->_hazard.load());
				_hsys.retire(_ptr->_hazard);
				break;
			}
		}

		_hsys.release(_ptr);
		_hsys.release(_ptr_next);

		return ret;
	}

	void push_back(const T & data){
		node * _node = get_node(data);

		detail::_hazard_ptr<node> _ptr_end = _hsys.acquire();
		detail::_hazard_ptr<node> _ptr_prev = _hsys.acquire();
		while(1){
			_ptr_end->_hazard = _list->end.load();
			_ptr_prev->_hazard = (_ptr_end->_hazard->xor.load())^0;

			if (_ptr_prev->_hazard == 0){
				if (_ptr_end->_hazard != _list->end.load()){
					continue;
				}



			}else{
				if (_ptr_end->_hazard != _list->end.load()){
					continue;
				}
			}
		}
	}

	bool pop_back(T & data){
	}

	void for_each(boost::function<void(T & data)> handle){
		
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
		_list->begin.store(_node);
		_list->end.store(_node);

		return _list;
	}

	void put_list(list * _list){
		node * _next = 0;
		do{
			node * _next = _list->begin.load()->xor.load() ^ 0;
			put_node(_next);
		}while(_next == 0);
	}

private:
	boost::atomic<list *> _list;

	_Alloc_node _alloc_node;
	_Alloc_list _alloc_list;

	detail::_hazard_system<node> _hsys;

};	

}// container
}// angelica

#endif //_XORLIST_H