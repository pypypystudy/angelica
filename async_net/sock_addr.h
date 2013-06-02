/*
 * address.h
 * Created on: 2012-10-18
 *     Author: qianqians
 * IP address
 */
#ifndef _address_h
#define _address_h

#ifdef _WIN32
#include "win32/winhdef.h"
#endif

#include <string>

namespace angelica {
namespace async_net {
namespace win32 {
class socket_base_win32;
} // win32

class sock_addr {
private:
	std::string str_addr;
	unsigned short _port;

#ifdef _WIN32
	struct in_addr sin_addr;
#elif __linux__
	struct in_addr sin_addr;
#endif

public:
	sock_addr();
	sock_addr(const char * _addr, const unsigned short _port_);
	sock_addr(const sockaddr * addr);
	~sock_addr();

	void operator =(const sock_addr & addr);

	std::string str_address();
	unsigned int int_address();
	unsigned short port();

	friend class win32::socket_base_win32;
	friend class async_service;

};

} //async_net
} //angelica

#endif //_address_h
