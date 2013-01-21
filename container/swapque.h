/*
 * swapque.h
 *  Created on: 2013-1-16
 *	    Author: qianqians
 * swapque:a fifo que
 */
#ifndef _SWAPQUE_H
#define _SWAPQUE_H

#include <boost/thread/shared_mutex.hpp>
#include <boost/atomic.hpp>
#include <boost/pool/pool_alloc.hpp>

#include <angelica/container/detail/_hazard_ptr.h>

namespace angelica{
namespace container{

template <typename T, typename _Allocator = boost::pool_allocator<T> >
class swapque{
private:
	struct _que_node{
		_que_node () : _next(0) {}
		_que_node (const T & val) : data(val), _next(0) {}
		~_que_node () {}

		T data;
		boost::atomic<_que_node *> _next;
	};

	struct _mirco_que{
		boost::atomic<_que_node *> _begin;
		boost::atomic<_que_node *> _end;
	};

	struct _que{
		_mirco_que * _frond, * _back;
		boost::atomic_uint32_t _size;
		boost::shared_mutex _mu;
	};

	typedef angelica::container::detail::_hazard_ptr<_que_node> _hazard_ptr;
	typedef angelica::container::detail::_hazard_system<_que_node> _hazard_system;
	typedef typename _Allocator::template rebind<_que_node>::other _node_alloc;
	typedef typename _Allocator::template rebind<_mirco_que>::other _mirco_que_alloc;
	typedef typename _Allocator::template rebind<_que>::other _que_alloc;

public:
	swapque(){
		__que.store(get_que());
	}

	~swapque(){
		put_que(__que.load());
	}

	bool empty(){
		return (__que.load()->_size.load() == 0);
	}

	std::size_t size(){
		return __que.load()->_size.load();
	}

	void clear(){
		_que * _new_que = get_que();
		_que * _old_que = __que.exchange(_new_que);
		boost::unique_lock<boost::shared_mutex> lock(_old_que->_mu);
		put_que(_old_que);
	}

	void push(const T & data){
		_que_node * _node = 0;
		while((_node = get_node(data)) == 0);

		while(1){
			_que * _tmp_que = __que.load();
			boost::shared_lock<boost::shared_mutex> lock(_tmp_que->_mu, boost::try_to_lock);
			if (lock.owns_lock()){
				_que_node * _old_end = _tmp_que->_back->_end.exchange(_node);
				_old_end->_next = _node;
				break;
			}
		}
	}

	bool pop(T & data){
		if (__que.load()->_size.load() == 0){
			return false;
		}

		while(1){
			_que * _tmp_que = __que.load();
			boost::upgrade_lock<boost::shared_mutex> lock(_tmp_que->_mu, boost::try_to_lock);
			if (lock.owns_lock()){
				_hazard_ptr * _hp_begin = _hazard_sys.acquire();
				_hp_begin->_hazard = _tmp_que->_frond->_begin.load();
				while(1){
					if (_hp_begin->_hazard == _tmp_que->_frond->_end.load()){
						if (_tmp_que->_size.load() == 0){
							return false;
						}

						boost::unique_lock<boost::shared_mutex> uniquelock(boost::move(lock));
						std::swap(_tmp_que->_frond, _tmp_que->_back);
						_hp_begin->_hazard = _tmp_que->_frond->_begin.load();
						if (_hp_begin->_hazard == _tmp_que->_frond->_end.load()){
							return false;
						}
					}

					_que_node * _next = _hp_begin->_hazard->_next;
					if(_tmp_que->_frond->_begin.compare_exchange_strong(_hp_begin->_hazard, _next)){
						data = _next->data;
					}
				}
			}
		}

		return true;
	}

private:
	_que * get_que(){
		_que * __que = __que_alloc.allocate(1);
		__que->_size = 0;

		__que->_frond = __mirco_que_alloc.allocate(1);
		_que_node * _node = __node_alloc.allocate(1);
		_node->_next.store(0);
		__que->_frond->_begin.store(_node);
		__que->_frond->_end.store(_node);

		__que->_back = __mirco_que_alloc.allocate(1);
		_node = __node_alloc.allocate(1);
		_node->_next.store(0);
		__que->_back->_begin.store(_node);
		__que->_back->_end.store(_node);

		return __que;
	}

	void put_que(_que * _p){
		_que_node * _node = _p->_frond->_begin;
		do{
			_que_node * _tmp = _node;
			_node = _node->_next;

			_hazard_sys.retire(_tmp, boost::bind(&angelica::container::swapque<T>::put_node, this, _1));
		}while(_node != 0);
		__mirco_que_alloc.deallocate(_p->_frond, 1);

		_node = _p->_back->_begin;
		do{
			_que_node * _tmp = _node;
			_node = _node->_next;

			_hazard_sys.retire(_tmp, boost::bind(&angelica::container::swapque<T>::put_node, this, _1));
		}while(_node != 0);
		__mirco_que_alloc.deallocate(_p->_back, 1);

		__que_alloc.deallocate(_p, 1);
	}

	_que_node * get_node(const T & data){
		_que_node * _node = __node_alloc.allocate(1);
		_node->data = data;
		_node->_next = 0;
		
		return _node;
	}

	void put_node(_que_node * _p){
		__node_alloc.deallocate(_p, 1);
	}

private:
	boost::atomic<_que *> __que;
	_que_alloc __que_alloc;
	_mirco_que_alloc __mirco_que_alloc;
	_node_alloc __node_alloc;
	_hazard_system _hazard_sys;

};	
	
} //container
} //angelica

#endif //_SWAPQUE_H