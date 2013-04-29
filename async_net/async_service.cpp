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

bool async_service::do_one(){
	fnHandle do_fn;
	if(event_que.pop(do_fn)){
		do_fn();
		return true;
	}

	return false;
}

void async_service::start(unsigned int nCurrentNum) {
	if (nCurrentNum == 0){
		nCurrentNum = current_num;
	}

	for (unsigned int i = 0; i < nCurrentNum; i++) {
		if (_th_group.create_thread(boost::bind(&async_service::serverwork, this)) == 0){
			BOOST_THROW_EXCEPTION(std::logic_error("Error: CreateThread failed. "));
		}
	}
}

} //async_net
} //angelica