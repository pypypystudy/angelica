/*
 * async_service.h
 *   Created on: 2012-11-14
 *       Author: qianqians
 * async_service
 */
#include "async_service.h"
#include "socket.h"
#include "sock_buff.h"
#include "socket_pool.h"
#include "buff_pool.h"
#include "read_bufff_pool.h"
#include "write_buff_pool.h"

namespace angelica { 
namespace async_net { 

async_service::async_service() : nConnect(0), nMaxConnect(0) {
	detail::SocketPool::Init();
	detail::BuffPool::Init(detail::page_size);
	detail::ReadBuffPool::Init();
	detail::WriteBuffPool::Init();
}

async_service::~async_service(){
}

void async_service::start(unsigned int nCurrentNum){
#ifdef _WIN32
	_impl.start(nCurrentNum);
#endif
}

void async_service::stop(){
#ifdef _WIN32
	_impl.stop();
#endif	
}

void async_service::post(fnHandle fn){
	event_que.push(fn);
}

bool async_service::do_one(){
	fnHandle do_fn;
	if(event_que.pop(do_fn)){
		do_fn();
		return true;
	}

	return false;
}

} //async_net
} //angelica