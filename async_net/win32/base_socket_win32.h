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
#include "../sock_addr.h"
#include "../error_code.h"

#include <boost/function.hpp>

#include <angelica/container/no_blocking_pool.h>

namespace angelica {
namespace async_net {

class socket_base;
class sock_addr;
class async_service;

namespace detail {
struct simple_buff;
class read_buff;
class write_buff;
} // detail

namespace win32 {

class iocp_impl;

class base_socket_win32 {
private:
	base_socket_win32();
	base_socket_win32(iocp_impl & _impl);
	base_socket_win32(const base_socket_win32 & s_);
	~base_socket_win32();

	int bind(sock_addr addr);

	int start_accpet();
	int do_async_accpet();
	void OnAccept(socket_base * sClient, DWORD llen, _error_code err);

	int start_recv();
	int do_async_recv();
	void OnRecv(DWORD llen, _error_code err);

	int async_send();
	int do_async_send();
	void OnSend(_error_code err);

	int async_connect(sock_addr addr);
	int do_async_connect();
	void OnConnect(_error_code err);

	int disconnect();
	int do_disconnect(LPOVERLAPPED povld);
	void onDeconnect(_error_code err);

	int closesocket();
	static void onClose(SOCKET fd);

private:
	static SOCKET Getfd();
	static void Releasefd(SOCKET fd);

private:
	SOCKET fd;
	socket_base * s;
	iocp_impl * impl;
	async_service * _service;

	sock_addr _remote_addr;
	int tryconnectcount;
	
	static angelica::container::no_blocking_pool<unsigned int> _sock_pool;

	friend class async_net::socket_base;
	friend class iocp_impl;

};

} //win32
} //async_net
} //angelica

#endif //_WIN32
#endif //_BASE_SOCKET_WIN32_H