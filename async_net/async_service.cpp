/*
 * async_service.h
 *   Created on: 2012-11-14
 *       Author: qianqians
 * async_service
 */
#include "async_service.h"
#include "socket.h"
#include "sock_buff.h"

namespace angelica { 
namespace async_net { 

async_service::async_service() : nConnect(0), nMaxConnect(0) {
	detail::InitMemPagePool();
	detail::InitReadBuffPool();
	detail::InitWriteBuffPool();
}

async_service::~async_service(){
	detail::DestryMemPagePool();
	detail::DestryReadBuffPool();
	detail::DestryWriteBuffPool();
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