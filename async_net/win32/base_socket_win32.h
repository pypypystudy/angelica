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

namespace angelica {
namespace async_net {

class socket;
class sock_addr;
class async_service;

namespace detail {
struct simple_buff;
class read_buff;
class write_buff;
} // detail

namespace win32 {

class iocp_impl;

namespace detail {

void InitWin32SOCKETPool();
SOCKET Getfd();
void Releasefd(SOCKET fd);
void DestroyWin32SOCKETPool();
} // detail

class base_socket_win32 {
private:
	base_socket_win32();
	base_socket_win32(iocp_impl & _impl);
	base_socket_win32(const base_socket_win32 & s_);
	~base_socket_win32();

	int bind(sock_addr addr);

	int start_accpet();
	int do_async_accpet();
	void OnAccept(async_net::detail::read_buff * _buf, SOCKET fd, DWORD llen, _error_code err);

	int start_recv();
	int do_async_recv();
	void OnRecv(DWORD llen, _error_code err);

	int async_send();
	int do_async_send();
	void OnSend(DWORD llen, _error_code err);

	int async_connect(sock_addr addr);
	int do_async_connect();
	void OnConnect(DWORD llen, _error_code err);

	int disconnect();
	int do_disconnect(LPOVERLAPPED povld);
	void onDeconnect(DWORD llen, _error_code err);

	int closesocket();
	static void onClose(SOCKET fd, DWORD llen, _error_code err);

private:
	SOCKET fd;
	socket * s;
	iocp_impl * impl;
	async_service * _service;

	sock_addr _remote_addr;
	int tryconnectcount;

	friend class async_net::socket;

};

} //win32
} //async_net
} //angelica

#endif //_WIN32
#endif //_BASE_SOCKET_WIN32_H