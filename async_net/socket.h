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

	int async_accpet(int num, boost::function<void(socket s, sock_addr & addr, _error_code err)> onAccpet, bool bflag);

	int async_recv(boost::function<void(char * buff, unsigned int lenbuff, _error_code err) > onRecv, bool bflag);
	
	int async_connect(sock_addr addr, boost::function<void(_error_code err)> onConnect);

	int async_send(char * buff, unsigned int lenbuff, boost::function<void(_error_code err) > onSend);

private:
	socket_base * _socket;
	boost::atomic_uint * _ref;

	friend class win32::base_socket_win32;

};

} //async_net
} //angelica

#endif