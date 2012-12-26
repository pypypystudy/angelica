/*
 * no_blocking_pool.h
 *   Created on: 2012-12-1
 *	     Author: qianqians
 * 一个对象池:基于thread cache和锁实现 
 * 由于对象的重复使用与线程无关,
 * 假定cpu逻辑核心为N个
 * thread cache采用N个有锁的list实现，而非线程本地存储
 * 一个线程访问对象池操作在理想情况下try_lock次数最大为N次,最小为1次
 * 所以可以认为这个对象池是write-free的.
 */
#ifndef _NO_BLOCKING_POOL_H
#define _NO_BLOCKING_POOL_H

#include <list>
#include <boost/thread/mutex.hpp>

#ifdef _WIN32
#include "win32/winhdef.h"
#endif 

namespace angelica {
namespace async_net {
namespace detail {

template <typename T>
class no_blocking_pool{
	struct mirco_pool{
		std::list<typename T * > _pool; 	
		boost::mutex _mu;
	};

public:
	no_blocking_pool(){
#ifdef _WIN32
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		_count = info.dwNumberOfProcessors;
#endif

		_pool = new mirco_pool[_count];
	}

	~no_blocking_pool(){
		delete[] _pool;
	}

	T * get(){
		unsigned int slide = 0;
		T * ret = 0;

		for(unsigned int i = 0; i < _count; i++){
			boost::mutex::scoped_lock lock(_pool[slide]._mu, boost::try_to_lock);
			if (lock.owns_lock()){
				if (!_pool[slide]._pool.empty()){
					ret = _pool[slide]._pool.front();
					_pool[slide]._pool.pop_front();

					break;
				}
				continue;
			}
		}

		return ret;
	}

	void release(T * ptr){
		unsigned int slide = 0;
		while(1){
			boost::mutex::scoped_lock lock(_pool[slide]._mu, boost::try_to_lock);
			if (lock.owns_lock()){
				_pool[slide]._pool.push_back(ptr);
				break;
			}

			if(++slide == _count){
				slide = 0;
			}
		}
	}

	std::size_t unsafe_size(){
		std::size_t size = 0;
		for(unsigned int i = 0; i < _count; i++){
			while(1){
				boost::mutex::scoped_lock lock(_pool[i]._mu, boost::try_to_lock);
				if (lock.owns_lock()){
					size += _pool[i]._pool.size();
					break;
				}
			}
		}

		return size;
	}

private:
	mirco_pool * _pool;
	unsigned int _count;

};

}// detail
}// async_net
}// angelica

#endif //_NO_BLOCKING_POOL_H