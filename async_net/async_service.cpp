/*
 * async_service.cpp
 *   Created on: 2012-11-14
 *       Author: qianqians
 * async_service
 */
#include "async_service.h"

namespace angelica { 
namespace async_net { 

void async_service::post(fnHandle fn){
	event_que.push(fn);
}

int async_service::Init(){
	detail::SocketPool::Init();
	detail::BuffPool::Init(detail::page_size);
	detail::ReadBuffPool::Init();
	detail::WriteBuffPool::Init();
}

bool async_service::do_one(){
	fnHandle do_fn;
	if(event_que.pop(do_fn)){
		do_fn();
		return true;
	}

	return false;
}

void async_service::run(){
	thread_count++;

	try{
		while(do_one());

		network();
	}catch(...){
		//error log	
	}
	
	thread_count--;
}

} //async_net
} //angelica
