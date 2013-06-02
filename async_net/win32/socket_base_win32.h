/*
 * base_socket_win32.h
 *         Created on: 2012-10-16
 *			   Author: qianqians
 * base socket at win32
 */
#ifndef _BASE_SOCKET_WIN32_H
#define _BASE_SOCKET_WIN32_H

#ifdef _WIN32

#include "winhdef.h"

#include "../socket_base.h"

#include <boost/atomic.hpp>
#include <boost/function.hpp>

#include <angelica/container/no_blocking_pool.h>

namespace angelica {
namespace async_net {
namespace win32 {

class socket_base_win32 : public socket_base{
public:
	socket_base_win32(async_service & _impl);
	~socket_base_win32();

private:
	void operator =(const socket_base_win32 & s){};

public:
	int bind(sock_addr addr);

	int opensocket();

	int closesocket();

public:
	int async_accpet(int num, bool bflag);

	int async_accpet(bool bflag);
	
	int async_recv(bool bflag);
	
	int async_connect(sock_addr addr);

	int async_send(char * buff, unsigned int lenbuff);

public:
	void OnAccept(socket_base * sClient, DWORD llen, _error_code err);
	
	void OnRecv(DWORD llen, _error_code err);
	
	void OnSend(_error_code err);
	
	void OnConnect(_error_code err);
	
	void onDeconnect(_error_code err);
	
	void onClose();

private:
	int do_async_accpet();
	
	int do_async_recv();

	int do_async_send();

	int do_async_connect();

	int do_disconnect(LPOVERLAPPED povld);

private:
	SOCKET fd;

	bool isListen;
	
};

} //win32
} //async_net
} //angelica

#endif //_WIN32
#endif //_BASE_SOCKET_WIN32_H
