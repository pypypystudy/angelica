/*
 * _mark_sweep_sys.hpp
 *		 Created on: 2012-9-9
 *	         Author: qianqians
 *  _mark_sweep_sys: mark-sweep
 *
 */

#ifndef _MARK_SWEEP_SYS_H
#define _MARK_SWEEP_SYS_H

#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>
#include <list>
#include <vector>
#include <boost/foreach.hpp>

namespace angelica{
namespace smart_ptr{
namespace detail{

template <typename Y>
struct _list_node_ {
	int index;
	typename std::list<Y *>::iterator iter;
};

template <typename T>
class _mark_sweep_sys{
private:
	// deallocate function
	typedef boost::function<void(typename T *)> fn_dealloc;
	// deallocate struct data
	typedef std::pair<typename T *, fn_dealloc > _deallocate_data;
	// _list_node 
	typedef _list_node_<typename T> _list_node;

	struct _micro_mark_sweep {
		int index;
		std::list<T *> _mark_list;
		std::list<_deallocate_data > _young_list;
		boost::mutex _mu;
	};

public:
	_mark_sweep_sys() {
		_root.resize(8);
		for(int i = 0; i < 8; i++) {
			_root[i] = new _micro_mark_sweep();
			_root[i]->index = i;
		}
	}

	~_mark_sweep_sys() {
		BOOST_FOREACH(_deallocate_data var, _old_list ) {
			var.second(var.first);
		}

		BOOST_FOREACH(_micro_mark_sweep * var, _root) {
			BOOST_FOREACH(_deallocate_data data, var->_young_list) {
				data.second(data.first);
			}
			delete var;
		}
	}

	void mount(T * data, fn_dealloc fn) {
		if (_middle_list.size() > 256) {
			sweep();
		}

		int index = 0;
		while(1) {
			if(_root[index]->_mu.try_lock())
				break;

			if(++index >= 8)
				index = 0;
		}

		_root[index]->_young_list.push_back(std::make_pair(data, fn));

		if(_root[index]->_young_list.size() > 32) {
			boost::mutex::scoped_lock _overall_lock(_mu);

			std::copy(_root[index]->_young_list.begin(), _root[index]->_young_list.end(), std::back_inserter(_middle_list));
			_root[index]->_young_list.clear();
		}

		_root[index]->_mu.unlock();
	}

	_list_node mark(T * ptr) {
		int index = 0;
		while(1) {
			if(_root[index]->_mu.try_lock())
				break;

			if(++index >= 8)
				index = 0;
		}

		_root[index]->_mark_list.push_back(ptr);
		
		_list_node node;
		node.index = index;
		node.iter = --(_root[index]->_mark_list.end());

		_root[index]->_mu.unlock();

		return node;
	}

	void umark(_list_node node) {
		boost::mutex::scoped_lock lock(_root[node.index]->_mu);
		_root[node.index]->_mark_list.erase(node.iter);
	}

private:
	void sweep() {
		boost::mutex::scoped_lock _overall_lock(_mu, boost::try_to_lock);
		if(!_overall_lock.owns_lock())
			return;

		std::vector<T *> _mark_list;
		BOOST_FOREACH(_micro_mark_sweep * var, _root) {
			boost::mutex::scoped_lock lock(var->_mu);

			std::copy(var->_mark_list.begin(), var->_mark_list.end(), std::back_inserter(_mark_list));
		}
		std::sort(_mark_list.begin(), _mark_list.end(), std::less<void*>());

		for(std::list<_deallocate_data >::iterator iter = _middle_list.begin(); iter != _middle_list.end(); ) {
			if(!std::binary_search(_mark_list.begin(), _mark_list.end(), iter->first)) {
				iter->second(iter->first);

				if(iter->first != _middle_list.back().first){
					*iter = _middle_list.back();
					_middle_list.pop_back();
				}
				else{
					iter = _middle_list.erase(iter);
				}
			}
			else {
				++iter;
			}
		}

		if (_middle_list.size() < 32) {
			std::copy(_middle_list.begin(), _middle_list.end(), std::back_inserter(_old_list));
			_middle_list.clear();
		}

		if (_old_list.size() > 256) {
			for(std::list<_deallocate_data >::iterator iter = _old_list.begin(); iter != _old_list.end(); ) {
				if(!std::binary_search(_mark_list.begin(), _mark_list.end(), iter->first)) {
					iter->second(iter->first);

					if(iter->first != _old_list.back().first){
						*iter = _old_list.back();
						_old_list.pop_back();
					}
					else{
						iter = _old_list.erase(iter);
					}
				}
				else {
					++iter;
				}
			}
		}
	}

private:
	std::vector<_micro_mark_sweep * > _root;

	std::list<_deallocate_data > _middle_list, _old_list;
	boost::mutex _mu;

};

}//angelica
}//smart_ptr
}//detail
#endif //_MARK_SWEEP_SYS_H