/*
 * socket.cpp
 * Created on: 2012-10-18
 *	   Author: qianqians
 * socket ½Ó¿Ú
 */
#include "sock_addr.h"
#include "sock_buff.h"
#include "socket.h"
#include "simple_pool.h"

namespace angelica {
namespace async_net {

socket::socket(async_service & _service_) : 
	isclosed(false),
	isrecv(false),
	isaccept(false),
	isdisconnect(true),
	_service(&_service_),
	_read_buff(detail::GetReadBuff()),
	_write_buff(detail::GetWriteBuff()),
#ifdef _WIN32
	fd(_service_._impl)
#endif
{
	fn_onAccpet = boost::bind(&socket::defaultonAccpet, this, _1, _2, _3);
	fn_onConnect = boost::bind(&socket::defaultonConnect, this, _1);
	fn_onRecv = boost::bind(&socket::defaultonRevc, this, _1, _2, _3);
	fn_onSend = boost::bind(&socket::defaultonSend, this, _1);
}

socket::socket() : 
	isclosed(false),
	isrecv(false),
	isaccept(false),
	isdisconnect(true),
	_service(0),
	_read_buff(0),
	_write_buff(0)
{
	fn_onAccpet = boost::bind(&socket::defaultonAccpet, this, _1, _2, _3);
	fn_onConnect = boost::bind(&socket::defaultonConnect, this, _1);
	fn_onRecv = boost::bind(&socket::defaultonRevc, this, _1, _2, _3);
	fn_onSend = boost::bind(&socket::defaultonSend, this, _1);
}

socket::socket(const socket & s) :
	isclosed(s.isclosed),
	isrecv(s.isrecv),
	isaccept(s.isaccept),
	isdisconnect(s.isdisconnect),
	_service(s._service),
	_read_buff(detail::GetReadBuff()),
	_write_buff(detail::GetWriteBuff()),
	fd(s.fd)
{
	fn_onAccpet = s.fn_onAccpet;
	fn_onConnect = s.fn_onConnect;
	fn_onRecv = s.fn_onRecv;
	fn_onSend = s.fn_onSend;
}

void socket::operator=(const socket & s){
	isclosed = s.isclosed;
	isrecv = s.isrecv;
	isaccept = s.isaccept;
	isdisconnect = s.isdisconnect;
	_service = s._service;
	fd = s.fd;

	fn_onAccpet = s.fn_onAccpet;
	fn_onConnect = s.fn_onConnect;
	fn_onRecv = s.fn_onRecv;
	fn_onSend = s.fn_onSend;
}

socket::~socket(){
	if (_read_buff != 0){
		_read_buff->fn_Release();
	}
	if (_write_buff != 0){
		_write_buff->fn_Release();
	}
}

int socket::register_accpet(boost::function<void(socket s, sock_addr & addr, _error_code err)> onAccpet){
	if(isclosed){
		return is_closed; 
	}

	fn_onAccpet = onAccpet;

	return socket_succeed;
}

int socket::register_connect(boost::function<void(_error_code err)> onConnect){
	if(isclosed){
		return is_closed;
	}
	
	fn_onConnect = onConnect;

	return socket_succeed;
}

int socket::register_recv(boost::function<void(char * buff, unsigned int lenbuff, _error_code err) > onRecv){
	if(isclosed){
		return is_closed;
	}
	
	fn_onRecv = onRecv;

	return socket_succeed;
}

int socket::register_send(boost::function<void(_error_code err) > onSend){
	if(isclosed){
		return is_closed;
	}

	fn_onSend = onSend;

	return socket_succeed;
}

int socket::bind(sock_addr addr){
	int ret = socket_succeed;

	if(isclosed){
		ret = is_closed;
	}else if(!isdisconnect){
		ret = is_connected;
	}else{
#ifdef _WIN32
		ret = fd.bind(addr);
#endif
	}

	return ret;
}
		
int socket::start_accpet(int num){
	if(isclosed){
		return is_closed;
	}else if(!isdisconnect){
		return is_connected;
	}

	if (num != 0){
		_service->nMaxConnect = num;
	}
	isaccept = true;

#ifdef _WIN32
	return fd.start_accpet();
#endif
}

int socket::end_accpet(){
	if(isclosed){
		return is_closed;
	}else if(!isdisconnect){
		return is_connected;
	}else{
		isaccept = false;
	}

	return socket_succeed;
}

int socket::start_recv(){
	if(isclosed){
		return is_closed;
	}else if(isdisconnect){
		return is_disconnected;
	}
		
	isrecv = true;
#ifdef _WIN32
	return fd.start_recv();
#endif
}
	
int socket::end_recv(){
	if(isclosed){
		return is_closed;
	}else if(isdisconnect){
		return is_disconnected;
	}else{
		isrecv = false;
	}

	return socket_succeed;
}

int socket::async_connect(sock_addr addr){
	if(isclosed){
		return is_closed;
	}else if(!isdisconnect){
		return is_connected;
	}

#ifdef _WIN32
	return fd.async_connect(addr);
#endif
}

int socket::async_send(char * buff, unsigned int lenbuff){
	if(isclosed){
		return is_closed;
	}else if(isdisconnect){
		return is_disconnected;
	}

	_write_buff->write(buff, lenbuff);
#ifdef _WIN32
	return fd.async_send();
#endif
}

int socket::closesocket(){
	if(isclosed){
		return is_closed;
	}

#ifdef _WIN32
	return fd.closesocket();
#endif
}

int socket::disconnect(){
	if(isclosed){
		return is_closed;
	}else if(isdisconnect){
		return is_disconnected;
	}

	isdisconnect = true;
#ifdef _WIN32
	return fd.disconnect();
#endif
}

} //async_net
} //angelica