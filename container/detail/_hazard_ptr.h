/*
 * _hazard_ptr.hpp
 *  Created on: 2012-8-26
 *	    Author: qianqians
 * _hazard_ptr: Used to solve the ABA problem
 */

#ifndef _HAZARD_PTR_H
#define _HAZARD_PTR_H

#include <vector>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/foreach.hpp>
#include <boost/atomic.hpp>
#include <boost/pool/pool_alloc.hpp>

namespace angelica{
namespace container{
namespace detail{
	
// _hazard_ptr
template <typename X>
struct _hazard_ptr{
	X * _hazard; //
	boost::atomic_int32_t _active; // 0 使用中/1 未使用
};

// hazard system 
template <typename T>
class _hazard_system{
public:
	_hazard_system(){
		llen.store(0);

		re_list_set.resize(8);
		for(int i = 0; i < 8; i++){
			re_list_set[i] = new recover_list;
			re_list_set[i]->active.store(1);
		}

		_head = _get_node();
	}

	~_hazard_system(){
		BOOST_FOREACH(recover_list * _re_list, re_list_set){
			if(!_re_list->re_vector.empty()){
				BOOST_FOREACH(_deallocate_data var, _re_list->re_vector){
					var.second(var.first);
				}
				_re_list->re_vector.clear();
			}
		}
		re_list_set.clear();
	}

private:
	// lock-free list(push only) storage _hazard_ptr
	typedef _hazard_ptr<typename T> _hazard_ptr_;
	typedef struct _list_node{
		_hazard_ptr_ _hazard;
		boost::atomic<_list_node *> next;
	} _list_head;

	// allocator 
	boost::pool_allocator<_list_node> _alloc_list_node;

	// hazard ptr list
	boost::atomic<_list_head *> _head;
	// list lenght
	boost::atomic_uint32_t llen;

	_list_node * _get_node(){
		_list_node * _node = _alloc_list_node.allocate(1);
		_node->_hazard._hazard = 0;
		_node->_hazard._active = 1;
		_node->next = 0;

		return _node;
	}

	void _put_node(_list_node * _node){
		_alloc_list_node.deallocate(_node, 1);
	}
	
public:
	_hazard_ptr_ * acquire(){
		// try to reuse a retired hazard ptr
		for(_list_node * _node = _head; _node; _node = _node->next){
			if (_node->_hazard._active.exchange(0) == 0)
				continue;
			return &_node->_hazard;
		}

		// alloc a new node 
		_list_node * _new_node = _get_node();
		_new_node->_hazard._active.store(0);
		// increment the list length
		llen++;
		// push into list
		_new_node->next = _head.exchange(_new_node);

		return &(_new_node->_hazard);
	}

	void release(_hazard_ptr_ * ptr){
		ptr->_hazard = 0;
		ptr->_active.store(1);
	}

private:
	//Recover flag
	boost::atomic_flag recoverflag;

	// deallocate function
	typedef boost::function<void(typename T * )> fn_dealloc;
	// deallocate struct data
	typedef std::pair<typename T *, fn_dealloc> _deallocate_data;
	// recover list
	struct recover_list {
		std::vector<_deallocate_data> re_vector;
		boost::atomic_int32_t active; // 0 使用中 / 1 未使用
	};

	// 回收队列集合
	std::vector<recover_list * > re_list_set;

public:
	void retire(T * p,  fn_dealloc fn){
		// get tss rvector
		recover_list * _rvector_ptr = 0;
		while(1){
			for(int i = 0; i < 8; i++) {
				if (re_list_set[i]->active.exchange(0) == 0){
					continue;
				}else{
					_rvector_ptr = re_list_set[i];
					break;
				}
			}

			if (_rvector_ptr != 0){
				break;
			}else{
				boost::this_thread::yield();
			}
		}

		// push into rvector
		_rvector_ptr->re_vector.push_back(std::make_pair(p, fn));
	

		// scan
		if(_rvector_ptr->re_vector.size() > 32 && _rvector_ptr->re_vector.size() > llen.load()){
			// scan hazard pointers list collecting all non-null ptrs
			std::vector<void *> hvector;
			for(_list_node * _node = _head; _node; _node = _node->next){
				void * p = _node->_hazard._hazard;
				if (p != 0)
					hvector.push_back(p);
			}

			// sort hazard list
			std::sort(hvector.begin(), hvector.end(), std::less<void*>());
		
			// deallocator
			std::vector<_deallocate_data>::iterator iter = _rvector_ptr->re_vector.begin();
			while(iter != _rvector_ptr->re_vector.end()){
				if(!std::binary_search(hvector.begin(), hvector.end(), iter->first)){
					iter->second(iter->first);

					if(iter->first != _rvector_ptr->re_vector.back().first){
						*iter = _rvector_ptr->re_vector.back();
						_rvector_ptr->re_vector.pop_back();
					}
					else{
						iter = _rvector_ptr->re_vector.erase(iter);
					}
				}
				else{
					++iter;
				}
			}
		}
		
		_rvector_ptr->active.store(1);
	}

};

} /* angelica */
} /* container */
} /* detail */
#endif // _HAZARD_PTR_H

