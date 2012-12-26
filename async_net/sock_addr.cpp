/*
 * address.cpp
 * Created on: 2012-10-18
 *     Author: qianqians
 * IP address
 */
#include "sock_addr.h"

namespace angelica {
namespace async_net {

sock_addr::sock_addr(){
}

sock_addr::sock_addr(char * _addr, unsigned short _port_) :
	str_addr(_addr), _port(_port_) {	
#ifdef _WIN32
	sin_addr.S_un.S_addr = inet_addr(_addr);
#endif
}

sock_addr::sock_addr(sockaddr * addr){
#ifdef _WIN32
	sin_addr = ((sockaddr_in *)addr)->sin_addr;
	_port = ((sockaddr_in *)addr)->sin_port;
	str_addr = inet_ntoa(sin_addr);
#endif
}

sock_addr::~sock_addr(){
}

void sock_addr::operator=(const sock_addr & addr){
#ifdef _WIN32
	sin_addr = addr.sin_addr;
	_port = addr._port;
	str_addr = inet_ntoa(sin_addr);
#endif	
}

std::string sock_addr::address() {
	return str_addr;
}

unsigned short sock_addr::port() {
	return _port;
}

} //async_net
} //angelica