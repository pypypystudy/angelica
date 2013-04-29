/*
 * msque.hpp
 *		 Created on: 2012-8-30
 *			 Author: qianqians
 * msque:
 */

#ifndef _MSQUE_H
#define _MSQUE_H

#include <boost/bind.hpp>
#include <boost/atomic.hpp>
#include <boost/pool/pool_alloc.hpp>

#include <angelica/container/detail/_hazard_ptr.h>

namespace angelica {
namespace container{

template <typename T, typename _Allocator = boost::pool_allocator<T> >
class msque{
private:
	struct _list_node{
		_list_node () {_next = 0;}
		_list_node (const T & val) : data(val) {_next = 0;}
		~_list_node () {}

		T data;
		boost::atomic<_list_node *> _next;
	};

	struct _list{
		boost::atomic<_list_node *> _begin;
		boost::atomic<_list_node *> _end;
		boost::atomic_uint32_t _size;
	};
	
	typedef angelica::container::detail::_hazard_ptr<_list_node> _hazard_ptr;
	typedef angelica::container::detail::_hazard_system<_list_node> _hazard_system;
	typedef typename _Allocator::template rebind<_list_node>::other _node_alloc;
	typedef typename _Allocator::template rebind<_list>::other _list_alloc;
		
public:
	msque(void){
		__list.store(get_list());
	}

	~msque(void){
		put_list(__list.load());
	}

	bool empty(){
		return (__list.load()->_size.load() == 0);
	}

	std::size_t size(){
		return __list.load()->_size.load();
	}

	void clear(){
		_list * _new_list = get_list();
		_list * _old_list = __list.exchange(_new_list);
		put_list(_old_list);
	}

	void push(const T & data){
		_list_node * _null = 0;
		
		_list_node * _node = 0;
		while(_node == 0){
			_node = get_node(data);
		}

		_hazard_ptr * _hp = _hazard_sys.acquire();
		_hp->_hazard = __list.load()->_end.load();
		while(1){
			if(_hp->_hazard != __list.load()->_end.load()){
				_hp->_hazard = __list.load()->_end.load();
				continue;
			}

			_list_node * next = _hp->_hazard->_next.load();

			if(_hp->_hazard != __list.load()->_end.load()){
				_hp->_hazard = __list.load()->_end.load();
				continue;
			}
			 
			if(next != 0){
				__list.load()->_end.compare_exchange_weak(_hp->_hazard, next);
				_hp->_hazard = __list.load()->_end.load();
				continue;
			}

			if (_hp->_hazard->_next.compare_exchange_weak(_null, _node)){
				break;
			}else{
				_null = 0;
				_hp->_hazard = __list.load()->_end.load();
			}
		}
		__list.load()->_end.compare_exchange_weak(_hp->_hazard, _node); 

		_hazard_sys.release(_hp);

		__list.load()->_size++;
	}

	bool pop(T & data){
		bool ret = true;
		
		_hazard_ptr * _hp_begin = _hazard_sys.acquire();
		_hazard_ptr * _hp_next = _hazard_sys.acquire();
		_hp_begin->_hazard = __list.load()->_begin.load();
		while(1){	
			if(_hp_begin->_hazard != __list.load()->_begin.load()){
				_hp_begin->_hazard = __list.load()->_begin.load();
				continue;
			}

			_list_node * end = __list.load()->_end.load();
			_hp_next->_hazard = _hp_begin->_hazard->_next.load();

			if(_hp_next->_hazard == 0){
				ret = false;
				goto end;
			}
			
			if(_hp_begin->_hazard != __list.load()->_begin.load()){
				_hp_begin->_hazard = __list.load()->_begin.load();
				continue;
			}
			
			if(end == _hp_begin->_hazard){
				__list.load()->_end.compare_exchange_weak(end, _hp_next->_hazard);
				_hp_begin->_hazard = __list.load()->_begin.load();
				continue;
			}

			if(__list.load()->_begin.compare_exchange_strong(_hp_begin->_hazard, _hp_next->_hazard)){
				break;
			}
		}
		data = _hp_next->_hazard->data;
		_hp_next->_hazard->~_list_node();
		_hazard_sys.retire(_hp_begin->_hazard, boost::bind(&angelica::container::msque<T>::put_node, this, _1));

		__list.load()->_size--;

	end:
		_hazard_sys.release(_hp_begin);
		_hazard_sys.release(_hp_next);

		return ret;
	}

private:
	_list * get_list(){
		_list * __list = __list_alloc.allocate(1);
		__list->_size = 0;

		_list_node * _node = __node_alloc.allocate(1);
		_node->_next.store(0);
		__list->_begin.store(_node);
		__list->_end.store(_node);

		return __list;
	}

	void put_list(_list * _p){
		_list_node * _node = _p->_begin;
		do{
			_list_node * _tmp = _node;
			_node = _node->_next;

			_hazard_sys.retire(_tmp, boost::bind(&angelica::container::msque<T>::put_node, this, _1));
		}while(_node != 0);
		__list_alloc.deallocate(_p, 1);
	}

	_list_node * get_node(const T & data){
		_list_node * _node = __node_alloc.allocate(1);
		new (_node) _list_node(data);
		_node->_next = 0;
		
		return _node;
	}

	void put_node(_list_node * _p){
		__node_alloc.deallocate(_p, 1);
	}

private:
	boost::atomic<_list *> __list;
	_list_alloc __list_alloc;
	_node_alloc __node_alloc;
	_hazard_system _hazard_sys;

	_Allocator __T_alloc;

};

} /* angelica */
} /* container */
#endif //_MSQUE_H