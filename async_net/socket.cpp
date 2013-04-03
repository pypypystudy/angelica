/*
 * socket.cpp
 * Created on: 2012-3-24
 *	   Author: qianqians
 * socket ½Ó¿Ú
 */
#include "socket.h"

namespace angelica {
namespace async_net {

socket::socket(){
	_ref = new boost::atomic_uint(1);
}

socket::socket(async_service & _impl){
	_socket = new socket_base(_impl);
	_ref = new boost::atomic_uint(1);
}

socket::socket(const socket & _s){
	_s._ref->operator++();
	_socket = _s._socket;
	_ref = _s._ref;
}

socket::~socket(){
	_ref->operator--();
	if (*_ref == 0){
		delete _socket;
	}
}

void socket::operator =(const socket & _s){
	_s._ref->operator++();
	_socket = _s._socket;
	_ref = _s._ref;
}

int socket::bind(sock_addr addr){
	return _socket->bind(addr);
}
	
int socket::closesocket(){
	return _socket->closesocket();
}

int socket::disconnect(){
	return _socket->disconnect();
}

int socket::async_accpet(int num, boost::function<void(socket s, sock_addr & addr, _error_code err)> onAccpet, bool bflag){
	return _socket->async_accpet(num, onAccpet, bflag);
}

int socket::async_recv(boost::function<void(char * buff, unsigned int lenbuff, _error_code err) > onRecv, bool bflag){
	return _socket->async_recv(onRecv, bflag);
}
	
int socket::async_connect(sock_addr addr, boost::function<void(_error_code err)> onConnect){
	return _socket->async_connect(addr, onConnect);
}

int socket::async_send(char * buff, unsigned int lenbuff, boost::function<void(_error_code err) > onSend){
	return _socket->async_send(buff, lenbuff, onSend);
}

} //async_net
} //angelica