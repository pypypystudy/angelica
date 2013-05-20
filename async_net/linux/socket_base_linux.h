/*
 * socket_base_linux.h
 * Created on: 2013-5-19
 *	   Author: qianqians
 * socket_base_linux ½Ó¿Ú
 */
#ifdef __linux__
#ifndef _SOCKET_BASE_LINUX_H
#define _SOCKET_BASE_LINUX_H

#include <sys/epoll.h>

#include <boost/thread.hpp>

#include "../async_service.h"
#include "../socket_base.h"

namespace angelica{
namespace async_net{
namespace linux{

class socket_base_linux : public socket_base{
public:
	socket_base_linux(boost::async_service & _service);
	~socket_base_linux();

private:
	socket_base_linux(){};
	socket_base_linux(const socket_base_linux & s){};
	void operator =(const socket_base_linux & s){};

public:
	void register_accpet_handle(AcceptHandle onAccpet);

	void register_recv_handle(RecvHandle onRecv);
	
	void register_connect_handle(ConnectHandle onConnect);
	
	void register_send_handle(SendHandle onSend);

public:
	virtual int bind(sock_addr addr) = 0;
	
	virtual int closesocket() = 0;

	virtual int disconnect() = 0;

public:
	virtual int async_accpet(int num, bool bflag) = 0;

	virtual int async_accpet(bool bflag) = 0;

	virtual int async_recv(bool bflag) = 0;
	
	virtual int async_connect(sock_addr addr) = 0;

	virtual int async_send(char * buff, unsigned int lenbuff) = 0;

	sock_addr get_remote_addr(){return _remote_addr;}

private:
	boost::thread th;

	SOCKET s;

};

}// linux
}// async_net
}// angelica

#endif //_SOCKET_BASE_LINUX_H

#endif //__linux__