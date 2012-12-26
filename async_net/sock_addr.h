/*
 * address.h
 * Created on: 2012-10-18
 *     Author: qianqians
 * IP address
 */
#ifndef _address_h
#define _address_h

#include <string>

#ifdef _WIN32
#include "win32/winhdef.h"
#endif

namespace angelica {
namespace async_net {

#ifdef _WIN32
namespace win32 {
class base_socket_win32;
}//win32
#endif //_WIN32

class sock_addr {
private:
	std::string str_addr;
	unsigned short _port;

#ifdef _WIN32
	struct in_addr sin_addr;
#endif

public:
	sock_addr();
	sock_addr(char * _addr, unsigned short _port_);
	sock_addr(sockaddr * addr);
	~sock_addr();

	void operator =(const sock_addr & addr);

	std::string address();
	unsigned short port();

private:
#ifdef _WIN32
	friend class win32::base_socket_win32;
#endif

};

} //async_net
} //angelica

#endif //_address_h