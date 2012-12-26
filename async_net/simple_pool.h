/*
 * simple_pool.h
 *   Created on: 2012-10-16
 *	     Author: qianqians
 * 一个简易对象池:基于无锁队列
 */
#ifndef _SIMPLE_POOL_H
#define _SIMPLE_POOL_H

#include <angelica/container/concurrent_queue.h>

namespace angelica {
namespace async_net {
namespace detail {

template <typename T>
class simple_pool{
private:
	angelica::container::concurrent_queue<typename T * > * pool_; 

public:
	simple_pool(){
		pool_ = new angelica::container::concurrent_queue<typename T * >;
	}

	~simple_pool(){
		delete pool_;
	}

	T * get(){
		T * ptr = 0;
		pool_->pop(ptr);
		
		return ptr;
	}
	
	void release(T * ptr){
		pool_->push(ptr);
	}

	std::size_t size(){
		return pool_->size();
	}

};

} //detail
} //async_net
} //angelica

#endif //_SIMPLE_POOL_H