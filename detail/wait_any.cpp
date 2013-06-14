/*
 * wait_any.cpp
 *  Created on: 2013-5-5
 *      Author: qianqians
 * wait_any
 */
#include "wait_any.h"

#include <boost/thread.hpp>

namespace angelica{
namespace base{

angelica::container::concurrent_interval_table<std::string, void*> wait_data;

bool addwait(std::string key, void * data){
	return wait_data.insert(key, data);
}

void* getwait(std::string key){
	void * var;
	wait_data.search(key, var);
	return var;
}

void wait_any(std::string key, boost::function<bool() > WaitConditionHandle, boost::function<void() > DoWaitHandle){
	while(1){
		if (!WaitConditionHandle()){
			DoWaitHandle();
		}else{
			wait_data.erase(key);
			break;
		}
	}
}

void wait(){
	boost::this_thread::sleep(boost::posix_time::milliseconds(0));
}

}// base
}// angelica