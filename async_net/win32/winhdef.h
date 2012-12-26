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

#endif //_WIN32
#endif //_WINHDEF_H