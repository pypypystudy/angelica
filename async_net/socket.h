/*
 * socket.h
 * Created on: 2013-2-24
 *	   Author: qianqians
 * socket ½Ó¿Ú
 */
#ifndef _SOCKET_H
#define _SOCKET_H

#include "socket_base.h"
#include <boost/atomic.hpp>

namespace angelica {
namespace async_net {
namespace win32 {
class socket_base_win32;
} // win32

class socket{
public:
	socket(async_service & _impl);
	socket(const socket & _s);

	~socket();

	void operator =(const socket & _s);

private:
	socket();

public:
	int bind(sock_addr addr);
	
	int closesocket();

	int disconnect();

	int async_accpet(int num, AcceptHandle onAccpet, bool bflag);

	int async_recv(RecvHandle onRecv, bool bflag);
	
	int async_connect(sock_addr addr, ConnectHandle onConnect);

	int async_send(char * buff, unsigned int lenbuff, SendHandle onSend);

private:
	socket_base * _socket;
	boost::atomic_uint * _ref;

	friend class async_service;
	friend class win32::socket_base_win32;

};

} //async_net
} //angelica

#endif