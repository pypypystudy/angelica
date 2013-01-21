/*
 * active.cpp
 *
 *  Created on: 2013-1-21
 *      Author: qianqians
 */

#include "active.h"
#include "mirco_active.h"
#include "active_server.h"

namespace angelica {
namespace active {

active::active(active_server & _active_server_) : _active_server(&_active_server_) {
	_mirco_active = _active_server->get_mirco_active();
}

active::~active() {
	_active_server->release_mirco_active(_mirco_active);
}

void active::post(event_handle _handle){
	_mirco_active->post(_handle, _active_server);
}

} /* namespace active */
} /* namespace angelica */