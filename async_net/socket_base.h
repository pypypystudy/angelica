/*
 * socket_base.h
 * Created on: 2012-10-16
 *	   Author: qianqians
 * socket ½Ó¿Ú
 */
#ifndef _SOCKET_BASE_H
#define _SOCKET_BASE_H

#ifdef _WIN32
#include "win32\base_socket_win32.h"
#endif 

#include <boost/function.hpp>
#include <boost/atomic.hpp>

#include <angelica/detail/tools.h>

#include "async_service.h"
#include "error_code.h"

namespace angelica {
namespace async_net {
	
namespace detail {
class write_buff;
class read_buff;
} //detail

class sock_addr;
class socket;

class socket_base {
public:
	socket_base(async_service & _impl);
	~socket_base();

private:
	socket_base(){};

	socket_base(const socket_base & s){};
	void operator =(const socket_base & s){};

public:
	int bind(sock_addr addr);
	
	int closesocket();

	int disconnect();

public:
	int async_accpet(int num, boost::function<void(socket s, sock_addr & addr, _error_code err)> onAccpet, bool bflag);

	int async_recv(boost::function<void(char * buff, unsigned int lenbuff, _error_code err) > onRecv, bool bflag);
	
	int async_connect(sock_addr addr, boost::function<void(_error_code err)> onConnect);

	int async_send(char * buff, unsigned int lenbuff, boost::function<void(_error_code err) > onSend);
	
private:
	boost::function<void (socket s, sock_addr & addr, _error_code err) > fn_onAccpet;
	boost::function<void (_error_code err) > fn_onConnect;
	boost::function<void (char * buff, unsigned int lenbuff, _error_code err) > fn_onRecv;
	boost::function<void (_error_code err) > fn_onSend;

private:
#ifdef _WIN32
	friend class win32::base_socket_win32;
	win32::base_socket_win32 fd;
#endif
	
	detail::read_buff * _read_buff;
	detail::write_buff * _write_buff;
	
	bool isclosed;
	bool isdisconnect;
	bool isrecv;
	bool isaccept;
	
	async_service * _service;
	
private:
	friend class async_service;
	friend class socket;

};		
	
} //async_net
} //angelica

#endif //_SOCKET_BASE_H