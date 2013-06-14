/*
 * wait_any.h
 *  Created on: 2013-5-5
 *      Author: qianqians
 * wait_any
 */
#ifndef _WAIT_ANY_H
#define _WAIT_ANY_H

#include <string>

#include <boost/function.hpp>

#include <angelica/container/concurrent_interval_table.h>

namespace angelica{
namespace base{

extern angelica::container::concurrent_interval_table<std::string, void*> wait_data;

bool addwait(std::string key, void * data);

void* getwait(std::string key);

void wait_any(std::string key, boost::function<bool() > WaitConditionHandle, boost::function<void() > DoWaitHandle);

void wait();

}// base
}// angelica

#endif //_WAIT_ANY_H