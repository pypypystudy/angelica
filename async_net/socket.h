/*
 * socket.h
 * Created on: 2012-10-16
 *	   Author: qianqians
 * socket ½Ó¿Ú
 */
#ifndef _SOCKET_H
#define _SOCKET_H

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

class socket {
public:
	socket(async_service & _impl);
	~socket();

	socket(const socket & s);
	void operator =(const socket & s);

private:
	socket();

public:
	int bind(sock_addr addr);
	
	int closesocket();
	int disconnect();

public:
	int register_accpet(boost::function<void(socket s, sock_addr & addr, _error_code err)> onAccpet);
	int register_connect(boost::function<void(_error_code err)> onConnect);
	int register_recv(boost::function<void(char * buff, unsigned int lenbuff, _error_code err) > onRecv);
	int register_send(boost::function<void(_error_code err) > onSend);

public:
	int start_accpet(int num);
	int end_accpet();

public:
	int start_recv();
	int end_recv();

public:
	int async_connect(sock_addr addr);
	int async_send(char * buff, unsigned int lenbuff);
	
private:
	boost::function<void(socket s, sock_addr & addr, _error_code err) > fn_onAccpet;
	boost::function<void(_error_code err) > fn_onConnect;
	boost::function<void(char * buff, unsigned int lenbuff, _error_code err) > fn_onRecv;
	boost::function<void(_error_code err) > fn_onSend;

private:
	void defaultonAccpet(socket s, sock_addr & addr, _error_code err){};
	void defaultonConnect(_error_code err){};
	void defaultonRevc(char * buff, unsigned int lenbuff, _error_code err){};
	void defaultonSend(_error_code err){};

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
	
};		
	
} //async_net
} //angelica

#endif //_SOCKET_H