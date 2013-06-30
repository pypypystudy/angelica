/*
 * socket.cpp
 * Created on: 2012-3-24
 *	   Author: qianqians
 * socket �ӿ�
 */
#include "socket.h"
#include "socket_pool.h"

#ifdef _WIN32
#include "win32/socket_base_win32.h"
#endif //_WIN32

namespace angelica {
namespace async_net {

socket::socket(){
	_ref = 0;
	_socket = 0;
}

socket::socket(async_service & _impl){
	_socket = async_net::detail::SocketPool::get(_impl);
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
		async_net::detail::SocketPool::release(_socket);
	}
}

void socket::operator =(const socket & _s){
	_s._ref->operator++();
	_socket = _s._socket;
	if (_ref != 0){
		_ref->operator--();
	}
	_ref = _s._ref;
}

bool socket::operator ==(const socket & _s){
	return (_socket == _s._socket);
}

bool socket::operator !=(const socket & _s){
	return (_socket != _s._socket);
}

int socket::bind(sock_addr addr){
	return _socket->bind(addr);
}

int socket::opensocket(async_service & _impl){
	if (_socket == 0 && _ref == 0){
		_socket = async_net::detail::SocketPool::get(_impl);
		_ref = new boost::atomic_uint(1);
	}else{
		return _socket->opensocket();
	}

	return socket_succeed;
}

int socket::closesocket(){
	return _socket->closesocket();
}

int socket::async_accpet(int num, bool bflag){
	return _socket->async_accpet(num, bflag);
}

int socket::async_accpet(bool bflag){
	return _socket->async_accpet(bflag);
}

int socket::async_recv(bool bflag){
	return _socket->async_recv(bflag);
}
	
int socket::async_connect(sock_addr addr){
	return _socket->async_connect(addr);
}

int socket::async_send(char * buff, unsigned int lenbuff){
	return _socket->async_send(buff, lenbuff);
}

sock_addr socket::get_remote_addr(){
	return _socket->get_remote_addr();
}

void socket::register_accpet_handle(AcceptHandle onAccpet){
	_socket->register_accpet_handle(onAccpet);
}

void socket::register_recv_handle(RecvHandle onRecv){
	_socket->register_recv_handle(onRecv);
}
	
void socket::register_connect_handle(ConnectHandle onConnect){
	_socket->register_connect_handle(onConnect);
}
	
void socket::register_send_handle(SendHandle onSend){
	_socket->register_send_handle(onSend);
}


} //async_net
} //angelica
