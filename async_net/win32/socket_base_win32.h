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
	
	int closesocket();

	int disconnect();

	int async_accpet(int num, AcceptHandle onAccpet, bool bflag);

	int async_recv(RecvHandle onRecv, bool bflag);
	
	int async_connect(sock_addr addr, ConnectHandle onConnect);

	int async_send(char * buff, unsigned int lenbuff, SendHandle onSend);

public:
	void OnAccept(socket_base * sClient, DWORD llen, _error_code err);
	
	void OnRecv(DWORD llen, _error_code err);
	
	void OnSend(_error_code err);
	
	void OnConnect(_error_code err);
	
	void onDeconnect(_error_code err);
	
	static void onClose(SOCKET fd);

private:
	int do_async_accpet();
	
	int do_async_recv();

	int do_async_send();

	int do_async_connect();

	int do_disconnect(LPOVERLAPPED povld);

private:
	boost::atomic_flag flagAcceptHandle;
	AcceptHandle onAcceptHandle;
	boost::atomic_flag flagRecvHandle;
	RecvHandle onRecvHandle;
	boost::atomic_flag flagConnectHandle;
	ConnectHandle onConnectHandle;

private:
	SOCKET fd;

	bool isListen;
	
};

} //win32
} //async_net
} //angelica

#endif //_WIN32
#endif //_BASE_SOCKET_WIN32_H