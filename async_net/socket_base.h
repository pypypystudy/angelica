/*
 * socket_base.h
 * Created on: 2012-10-16
 *	   Author: qianqians
 * socket ½Ó¿Ú
 */
#ifndef _SOCKET_BASE_H
#define _SOCKET_BASE_H

#include "sock_addr.h"

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

class socket;

typedef boost::function<void(socket s, sock_addr & addr, _error_code err)> AcceptHandle;
typedef boost::function<void(char * buff, unsigned int lenbuff, _error_code err) > RecvHandle;
typedef boost::function<void(_error_code err)> ConnectHandle;
typedef boost::function<void(char * buff, unsigned int lenbuff, _error_code err) > SendHandle;

class socket_base {
public:
	socket_base(async_service & _impl);
	~socket_base();

private:
	socket_base(){};
	socket_base(const socket_base & s){};
	void operator =(const socket_base & s){};

public:
	virtual int bind(sock_addr addr) = 0;
	
	virtual int closesocket() = 0;

	virtual int disconnect() = 0;

	virtual int async_accpet(int num, AcceptHandle onAccpet, bool bflag) = 0;

	virtual int async_recv(RecvHandle onRecv, bool bflag) = 0;
	
	virtual int async_connect(sock_addr addr, ConnectHandle onConnect) = 0;

	virtual int async_send(char * buff, unsigned int lenbuff, SendHandle onSend) = 0;

protected:
	sock_addr _remote_addr;

	detail::read_buff * _read_buff;
	detail::write_buff * _write_buff;
	
	bool isclosed;
	bool isdisconnect;
	bool isrecv;
	bool isaccept;

	int tryconnectcount;

	async_service * _service;

};		
	
} //async_net
} //angelica

#endif //_SOCKET_BASE_H