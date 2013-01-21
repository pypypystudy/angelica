/*
 * mirco_active.cpp
 *
 *  Created on: 2012-3-27
 *      Author: qianqians
 */

#include "mirco_active.h"
#include "active_server.h"

namespace angelica {
namespace active {

mirco_active::mirco_active() {
	// TODO Auto-generated constructor stub
}

mirco_active::~mirco_active() {
	// TODO Auto-generated destructor stub
}

void mirco_active::post(event_handle _handle, active_server * _active_server){
	event_que.push(_handle);
	_active_server->post(this);
}

bool mirco_active::do_one(){
	event_handle _handle;

	if (!event_que.pop(_handle)){
		return false;
	}

	_handle();

	return true;
}

void mirco_active::run(){
	event_handle _handle;

	while(1){
		if (event_que.pop(_handle)){
			_handle();
		}else{
			break;
		}
	}
}

} /* namespace active */
} /* namespace angelica */
