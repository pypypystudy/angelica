/*
 * active_server.cpp
 *
 *  Created on: 2012-4-8
 *      Author: qianqians
 */

#include "active_server.h"
#include "mirco_active.h"

#include <Windows.h>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace angelica {
namespace active {

active_server::active_server() {
	// TODO Auto-generated constructor stub
	
	_run_flag.store(false);
	_empty_flag.store(false);

	SYSTEM_INFO info;
	GetSystemInfo(&info);
		
	current_num = info.dwNumberOfProcessors;
}

active_server::~active_server() {
	// TODO Auto-generated destructor stub
}

bool active_server::do_one() {
	mirco_active * _active = _active_pool.pop();
	if (_active == 0){
		return false;
	}
	_active->run();
	_active->_run_flag.store(false);

	return true;
}

void active_server::start(unsigned int  nCurrentNum){
	if (_run_flag.exchange(true) == false){
		while(1){
			if (!do_one()){
				if (_run_flag.load() == false){
					break;
				}

				if (_active_pool.size() == 0){
					_empty_flag.store(false);
				
					boost::mutex::scoped_lock lock(_mu);
					_cond.timed_wait(lock, boost::get_system_time() + boost::posix_time::seconds(3));
				}
			}
		}
	}
}

void active_server::post(mirco_active * _mirco_active){
	if (_mirco_active->_run_flag.exchange(true) == false){
		_active_pool.put(_mirco_active);

		if (_empty_flag.exchange(true) == false){
			_cond.notify_all();
		}
	}
}

mirco_active * active_server::get_mirco_active(){
	mirco_active * _mirco_active = _free_active_pool.pop();
	if (_mirco_active == 0){
		_mirco_active = new mirco_active();
	}

	return _mirco_active;
}

void active_server::release_mirco_active(mirco_active * _mirco_active){
	_free_active_pool.put(_mirco_active);
}

} /* namespace active */
} /* namespace angelica */
