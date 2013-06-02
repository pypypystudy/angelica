/*
 * socket_base_linux.h
 * Created on: 2013-5-19
 *	   Author: qianqians
 * socket_base_linux �ӿ�
 */
#ifdef __linux__
#ifndef _SOCKET_BASE_LINUX_H
#define _SOCKET_BASE_LINUX_H

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <boost/thread.hpp>
#include <boost/atomic.hpp>

#include "../async_service.h"
#include "../socket_base.h"

namespace angelica{
namespace async_net{
namespace _linux{

class socket_base_linux : public socket_base{
public:
	socket_base_linux(boost::async_service & _service);
	~socket_base_linux();

private:
	void InitSocket(int fd);

private:
	socket_base_linux(int fd);

	socket_base_linux(){}
	socket_base_linux(const socket_base_linux & s){}
	void operator =(const socket_base_linux & s){}

public:
	int bind(sock_addr addr);

	int opensocket(sock_addr addr);

	int closesocket();

public:
	int async_accpet(int num, bool bflag);

	int async_accpet(bool bflag);

	int async_recv(bool bflag);
	
	int async_connect(const sock_addr & addr);

	int async_send(char * buff, unsigned int lenbuff);

	sock_addr get_remote_addr(){return _remote_addr;}

public:
	void OnAccept(int fd);

	void OnConnect(int err);

	void OnSend();

	void OnRecv();

private:
	int do_send();

private:
	boost::thread * _th;

	boost::atomic_flag _sendflag;

	boost::shared_mutex _send_shared_mutex;

	int _socket;

};

}// linux
}// async_net
}// angelica

#endif //_SOCKET_BASE_LINUX_H

#endif //__linux__
