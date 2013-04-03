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
#include <boost/atomic.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace angelica {
namespace container {

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

		_size.store(0);
		_pool = new mirco_pool[_count];
	}

	~no_blocking_pool(){
		delete[] _pool;
	}

	T * pop(){
		T * ret = 0;

		while(_size.load() != 0){
			for(unsigned int i = 0; i < _count; i++){
				boost::mutex::scoped_lock lock(_pool[i]._mu, boost::try_to_lock);
				if (lock.owns_lock()){
					if (!_pool[i]._pool.empty()){
						ret = _pool[i]._pool.front();
						_pool[i]._pool.pop_front();
						_size--;

						return ret;
					}
				}
			}
		}

		return ret;
	}

	void put(T * ptr){
		unsigned int slide = 0;
		while(1){
			boost::mutex::scoped_lock lock(_pool[slide]._mu, boost::try_to_lock);
			if (lock.owns_lock()){
				_pool[slide]._pool.push_back(ptr);
				_size++; 
				break;
			}

			if(++slide == _count){
				slide = 0;
			}
		}
	}

	std::size_t size(){
		return _size.load();
	}

private:
	mirco_pool * _pool;
	unsigned int _count;

	boost::atomic_uint _size;

};

}// container
}// angelica

#endif //_NO_BLOCKING_POOL_H