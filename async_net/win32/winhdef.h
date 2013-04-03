/*
 * winhdef.h
 *   Created on: 2012-10-16
 *       Author: qianqians
 * winhdef
 */
#ifndef _WINHDEF_H
#define _WINHDEF_H

#ifdef _WIN32

#include <WinSock2.h>
#include <Mswsock.h>
#include <MSTcpIP.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

enum win32_event_type{
	win32_tcp_send_complete = 1,
	win32_tcp_recv_complete = 2,
	win32_tcp_connect_complete = 3,
	win32_tcp_accept_complete = 4,
	win32_tcp_disconnect_complete = 5,
	win32_tcp_close_complete = 6,
	win32_stop_,
};

#endif //_WIN32
#endif //_WINHDEF_H