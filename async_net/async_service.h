/*
 * async_service.h
 *   Created on: 2012-10-16
 *       Author: qianqians
 * async_service
 */
#ifndef _NET_SERVICE_H
#define _NET_SERVICE_H

#include <boost/function.hpp>

#include <angelica/container/swapque.h>
#include <angelica/container/no_blocking_pool.h>

#include "error_code.h"

namespace angelica { 
namespace async_net { 
namespace win32 { 
#ifdef _WIN32
class socket_base_win32;
#elif __linux__ 
class socket_base_linux;
#endif
}// win32

typedef boost::function<void()> fnHandle;

class async_service{
public:
	async_service();
	~async_service();

	void run();

	void post(fnHandle fn);

private:
	void Init();

	bool network();

	bool do_one();

private:
#ifdef _WIN32
	HANDLE hIOCP;
	friend class win32::socket_base_win32;
#elif __linux__
	int epollfd_write, epollfd_read;
	friend class win32::socket_base_linux;
#endif //_WIN32

	boost::atomic_uint32_t thread_count;
	
	boost::atomic_ulong nConnect;
	unsigned long nMaxConnect;

	angelica::container::swapque<fnHandle > event_que;

};  

} //async_net
} //angelica


#endif //_NET_SERVICE_H
