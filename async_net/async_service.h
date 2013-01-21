/*
 * async_service.h
 *   Created on: 2012-10-16
 *       Author: qianqians
 * async_service
 */
#ifndef _NET_SERVICE_H
#define _NET_SERVICE_H

#ifdef _WIN32
#include "win32\iocp_impl.h"
#endif 

#include <boost/function.hpp>
#include <angelica/container/swapque.h>

namespace angelica { 
namespace async_net { 

class socket;
typedef boost::function<void()> fnHandle;

class async_service{
public:
	async_service();
	~async_service();

	void start(unsigned int nCurrentNum = 0);
	void stop();

	void post(fnHandle fn);

private:
	bool do_one();

private:
#ifdef _WIN32	
	friend class win32::base_socket_win32;
	friend class win32::iocp_impl;

	win32::iocp_impl _impl;
#endif
	
	boost::atomic_ulong nConnect;
	unsigned long nMaxConnect;

	angelica::container::swapque<fnHandle > event_que;

	friend class socket;

}; 

} //async_net
} //angelica


#endif //_NET_SERVICE_H