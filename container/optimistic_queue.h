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
		node(const T & _data) : data(_data), next(0), prev(0){}
		~node(){}

		T data;
		boost::atomic<node *> next, prev;
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
		detail::_hazard_ptr<list> * _plist = _hsys_list.acquire();
		list * _tmplist = get_list();
		_plist->_hazard = _list.exchange(_tmplist);
		_hsys_list.retire(_plist->_hazard);
		_hsys_list.release(_plist);
	}

	size_t size(){
		return _list->size.load();
	}

	void push_front(const T & data){
		node * _new = get_node(data);

		detail::_hazard_ptr<list> * _plist = _hsys_list.acquire();
		detail::_hazard_ptr<node> * _ptr_head = _hsys.acquire();
		detail::_hazard_ptr<node> * _ptr_next = _hsys.acquire();
		while(1){
			_plist->_hazard = _list.load();

			_ptr_head->_hazard = _plist->_hazard->head.load();
			_ptr_next->_hazard = _ptr_head->_hazard->next.load();
			
			_new->prev = _ptr_head->_hazard;
			_new->next = _ptr_next->_hazard;

			if (_ptr_head->_hazard != _plist->_hazard->head.load()){
				continue;
			}

			if (_ptr_head->_hazard->next.compare_exchange_weak(_ptr_next->_hazard, _new)){
				if (_ptr_next->_hazard != 0){
					_ptr_next->_hazard->prev.store(_new);
				}
				_plist->_hazard->size++;
				break;
			}
		}

		_hsys.release(_ptr_head);
		_hsys.release(_ptr_next);

		_hsys_list.release(_plist);
	}

	bool pop_front(T & data){
		bool ret = true;
		
		detail::_hazard_ptr<list> * _plist = _hsys_list.acquire();
		detail::_hazard_ptr<node> * _ptr_head = _hsys.acquire();
		detail::_hazard_ptr<node> * _ptr_next = _hsys.acquire();
		while(1){
			_plist->_hazard = _list.load();

			_ptr_head->_hazard = _plist->_hazard->head.load();
			_ptr_next->_hazard = _ptr_head->_hazard->next.load();

			if (_ptr_next->_hazard == 0){
				ret = false;
				break;
			}

			if (_ptr_next->_hazard == _plist->_hazard->detail.load()){
				ret = false;
				break;
			}

			if (_ptr_head->_hazard != _plist->_hazard->head.load()){
				continue;
			}
		
			if (_plist->_hazard->head.compare_exchange_weak(_ptr_head->_hazard, _ptr_next->_hazard)){
				_ptr_next->_hazard->prev.store(0);
				data = _ptr_next->_hazard->data;
				_hsys.retire(_ptr_head->_hazard, boost::bind(&optimistic_queue::put_node, this, _1));
				_plist->_hazard->size--;
				
				break;
			}
		}

		_hsys.release(_ptr_head);
		_hsys.release(_ptr_next);

		_hsys_list.release(_plist);

		return ret;
	}

	void push_back(const T & data){
		node * _new = get_node(data);

		detail::_hazard_ptr<list> * _plist = _hsys_list.acquire();
		detail::_hazard_ptr<node> * _ptr_detail = _hsys.acquire();
		while(1){
			_plist->_hazard = _list.load();

			_ptr_detail->_hazard = _plist->_hazard ->detail.load();

			if (_ptr_detail->_hazard == 0){
				_plist->_hazard->detail.store(_plist->_hazard->head.load());
				_ptr_detail->_hazard = _plist->_hazard ->detail.load();
			}

			while(_ptr_detail->_hazard->next != 0){
				_plist->_hazard ->detail.compare_exchange_weak(_ptr_detail->_hazard, _ptr_detail->_hazard->next);
				_ptr_detail->_hazard = _plist->_hazard ->detail.load();
			}

			if (_ptr_detail->_hazard != _plist->_hazard ->detail.load()){
				continue;
			}

			_new->prev = _ptr_detail->_hazard;

			if (_ptr_detail->_hazard != _plist->_hazard ->detail.load()){
				continue;
			}

			if (_plist->_hazard ->detail.compare_exchange_weak(_ptr_detail->_hazard, _new)){
				_ptr_detail->_hazard->next.store(_new);
				_plist->_hazard ->size++;
				break;
			}
		}

		_hsys.release(_ptr_detail);

		_hsys_list.release(_plist);
	}

	bool pop_back(T & data){
		bool ret = true;
		
		detail::_hazard_ptr<list> * _plist = _hsys_list.acquire();
		detail::_hazard_ptr<node> * _ptr_detail = _hsys.acquire();
		detail::_hazard_ptr<node> * _ptr_prev = _hsys.acquire();
		while(1){
			_plist->_hazard = _list.load();

			_ptr_detail->_hazard = _plist->_hazard->detail.load();

			if (_ptr_detail->_hazard == 0){
				_plist->_hazard->detail.store(_plist->_hazard->head.load());
				continue;
			}

			if(_ptr_detail->_hazard->next != 0){
				_plist->_hazard ->detail.compare_exchange_weak(_ptr_detail->_hazard, _ptr_detail->_hazard->next);
				continue;
			}

			if (_ptr_detail->_hazard != _plist->_hazard->detail.load()){
				continue;
			}

			_ptr_prev->_hazard = _ptr_detail->_hazard->prev.load();

			if (_ptr_detail->_hazard != _plist->_hazard->detail.load()){
				continue;
			}

			if (_ptr_prev->_hazard == _plist->_hazard->head.load()){
				ret = false;
				break;
			}
			
			if (_ptr_prev->_hazard == 0){
				ret = false;
				break;
			}

			if (_ptr_detail->_hazard != _plist->_hazard->detail.load()){
				continue;
			}
		
			if (_plist->_hazard->detail.compare_exchange_weak(_ptr_detail->_hazard, _ptr_prev->_hazard)){
				data = _ptr_detail->_hazard->data;
				_hsys.retire(_ptr_detail->_hazard, boost::bind(&optimistic_queue::put_node, this, _1));
				
				_ptr_prev->_hazard->next.compare_exchange_weak(_ptr_detail->_hazard, 0);
				
				_plist->_hazard->size--;
				
				break;
			}
		}

		_hsys.release(_ptr_detail);
		_hsys.release(_ptr_prev);

		_hsys_list.release(_plist);

		return ret;
	}

private:
	node * get_node(){
		node * _node = new node;//_alloc_node.allocate(1);
		//::new (_node) node();

		return _node;
	}

	node * get_node(const T & data){
		node * _node = new node(data);//_alloc_node.allocate(1);
		//::new (_node) node(data);

		return _node;
	}

	void put_node(node * _node){
		delete _node;
		//_node->~node();
		//_alloc_node.deallocate(_node, 1);
	}

	list * get_list(){
		list * _list = new list;//_alloc_list.allocate(1);
		//::new (_list) list();

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

		delete _list;
	}

private:
	boost::atomic<list *> _list;

	_Alloc_node _alloc_node;
	_Alloc_list _alloc_list;

	detail::_hazard_system<node> _hsys;
	detail::_hazard_system<list> _hsys_list;

};

}// container
}// angelica

#endif //_OPTIMISTIC_QUEUE_H