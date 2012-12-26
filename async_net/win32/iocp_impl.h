/*
 * iocp_impl.h
 *  Created on: 2012-10-1
 *      Author: qianqians
 * IOCP
 */
#ifndef _IOCP_IMPL_H
#define _IOCP_IMPL_H

#ifdef _WIN32

#include <boost/atomic.hpp>
#include "winhdef.h"

namespace angelica {
namespace async_net {
namespace win32 {

class iocp_impl {
public:
	iocp_impl();
	~iocp_impl();

	void start(unsigned int nCurrentNum);
	void stop();

private:
	static DWORD WINAPI serverwork(void * impl );

private:
	HANDLE hIOCP;
	boost::atomic_uint32_t thread_count;
	unsigned int current_num;

	friend class base_socket_win32;

};

} //win32
} //async_net
} //angelica

#endif //_WIN32
#endif //_IOCP_IMPL_H