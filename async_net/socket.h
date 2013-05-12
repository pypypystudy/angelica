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
	socket();

	socket(async_service & _impl);
	socket(const socket & _s);

	~socket();

	void operator =(const socket & _s);

	bool operator ==(const socket & _s);

	bool operator !=(const socket & _s);

public:
	void register_accpet_handle(AcceptHandle onAccpet);

	void register_recv_handle(RecvHandle onRecv);
	
	void register_connect_handle(ConnectHandle onConnect);
	
	void register_send_handle(SendHandle onSend);

public:
	int opensocket(async_service & _impl);

	int bind(sock_addr addr);
	
	int closesocket();

	int disconnect();

public:


public:
	int async_accpet(int num, bool bflag);

	int async_accpet(bool bflag);

	int async_recv(bool bflag);
	
	int async_connect(sock_addr addr);

	int async_send(char * buff, unsigned int lenbuff);

	sock_addr get_remote_addr();

private:
	socket_base * _socket;
	boost::atomic_uint * _ref;

	friend class async_service;
	friend class win32::socket_base_win32;

};

} //async_net
} //angelica

#endif