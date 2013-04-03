/*
 * base_socket_win32.cpp
 *         Created on: 2012-10-17
 *			   Author: qianqians
 * base_socket_win32й╣ож
 */
#ifdef _WIN32

#include <sstream>
#include <string>

#include <angelica/detail/tools.h>

#include "iocp_impl.h"
#include "base_socket_win32.h"
#include "Overlapped.h"

#include "../buff_pool.h"
#include "../socket_pool.h"
#include "../socket.h"
#include "../sock_addr.h"
#include "../sock_buff.h"

namespace angelica {
namespace async_net {
namespace win32 {

angelica::container::no_blocking_pool<unsigned int> base_socket_win32::_sock_pool;

base_socket_win32::base_socket_win32(iocp_impl & _impl) : impl(&_impl), tryconnectcount(0) {
	fd = Getfd();

	if (CreateIoCompletionPort((HANDLE)fd, _impl.hIOCP, (ULONG_PTR)this, 0) != _impl.hIOCP){
		DWORD err = WSAGetLastError();
	}

	s = container_of(this, socket_base, fd);
	_service = container_of(impl, async_service, _impl);
}

base_socket_win32::base_socket_win32() : fd(0), impl(0){
	s = container_of(this, socket_base, fd);
	_service = container_of(impl, async_service, _impl);
}

base_socket_win32::base_socket_win32(const base_socket_win32 & s_){
	fd = s_.fd;
	impl = s_.impl;
	
	_remote_addr = s_._remote_addr;
	tryconnectcount = s_.tryconnectcount;

	s = container_of(this, socket_base, fd);
	_service = container_of(impl, async_service, _impl);
}

base_socket_win32::~base_socket_win32(){
}

int base_socket_win32::bind(sock_addr addr){
	sockaddr_in service;
	memset(&service, 0, sizeof(sockaddr_in));
	service.sin_family = AF_INET;
	service.sin_addr = addr.sin_addr;
	service.sin_port = addr._port;

	int ret = socket_succeed;
	if (::bind(fd, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR){
		ret = GetLastError();
	}

	return ret;
}

int base_socket_win32::do_async_accpet(){
	DWORD dwBytes;
	socket_base * _client_socket = async_net::detail::SocketPool::get(*s->_service);
	OverlappedEX_Accept * olp = detail::OverlappedEXPool<OverlappedEX_Accept >::get();
	olp->socket_ = _client_socket;
	olp->overlapex.type = win32_tcp_accept_complete;
			
	if (!AcceptEx(fd, 
				  _client_socket->fd.fd, 
				  _client_socket->_read_buff->buff, 
				  0, 
				  sizeof(sockaddr_in) + 16, 
				  sizeof(sockaddr_in) + 16, 
				  &dwBytes, 
				  &olp->overlapex.overlap)){
		
		DWORD err = WSAGetLastError();
		if (err != ERROR_IO_PENDING){
			return err;
		}
	}

	return socket_succeed;
}

int base_socket_win32::start_accpet(){
	if (listen(fd, 100) == SOCKET_ERROR) {
		return WSAGetLastError();
	} 
	
	_error_code ret = socket_succeed;

	SYSTEM_INFO info;
	GetSystemInfo(&info);
	for(DWORD i = 0; i < info.dwNumberOfProcessors * 64; i++){
		ret = do_async_accpet();
		if(ret) break;
	}

	return socket_succeed;
}

void base_socket_win32::OnAccept(socket_base * sClient, DWORD llen, _error_code err) {
	sock_addr addr;

	if ((_service->nConnect++ < _service->nMaxConnect) && s->isaccept){
		s->fd.do_async_accpet();
	}
	
	if(s->isclosed){
		err = is_closed;
		Releasefd(sClient->fd.fd);
	}

	if (err){
		Releasefd(sClient->fd.fd);
	}else{
		LPSOCKADDR local_addr = 0;
		int local_addr_length = 0;
		LPSOCKADDR remote_addr = 0;
		int remote_addr_length = 0;
		GetAcceptExSockaddrs(
			sClient->_read_buff->buff, 
			llen, 
			sizeof(sockaddr_in) + 16, 
			sizeof(sockaddr_in) + 16, 
			&local_addr,
			&local_addr_length,
			&remote_addr,
			&remote_addr_length);
	
		if (remote_addr_length > sizeof(sockaddr_in) + 16) {
			Releasefd(sClient->fd.fd);
		}else{
			if (llen > 0){
				sClient->_read_buff->slide += llen;
			}

			setsockopt(sClient->fd.fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&fd, sizeof(fd));
			if (CreateIoCompletionPort((HANDLE)sClient->fd.fd, impl->hIOCP, 0, 0) != impl->hIOCP){
				DWORD err = WSAGetLastError();
			}

			addr = remote_addr;

			sClient->isdisconnect = false;
		}
	}

	socket socket_;
	socket_._socket = sClient;
	if (!s->fn_onAccpet.empty()){
		s->fn_onAccpet(socket_, addr, err);
	}
}

int base_socket_win32::start_recv() {
	if (s->_read_buff->slide > 0){
		if (!s->fn_onRecv.empty()){
			s->fn_onRecv(s->_read_buff->buff, s->_read_buff->slide, socket_succeed);
		}
		s->_read_buff->slide = 0;
	}

	return do_async_recv();
}

int base_socket_win32::do_async_recv(){
	DWORD flag = 0;
	DWORD llen = 0;
	
	OverlappedEX * ovl = detail::OverlappedEXPool<OverlappedEX >::get();
	ovl->type = win32_tcp_recv_complete;
	
	s->_read_buff->_wsabuf.buf = 0;
	s->_read_buff->_wsabuf.len = 0;
	if (WSARecv(this->fd, &s->_read_buff->_wsabuf, 1, &llen, &flag, &ovl->overlap, 0) == SOCKET_ERROR){
		DWORD err = WSAGetLastError();

		if (err != WSA_IO_PENDING){
			return err;
		}
	}

	return socket_succeed;
}	

void base_socket_win32::OnRecv(DWORD llen, _error_code err) { 
	if (!err){
		while(1){	
			std::size_t size = s->_read_buff->buff_size - s->_read_buff->slide;
			char * buff = s->_read_buff->buff + s->_read_buff->slide;
			if((llen = recv(fd, buff, size, 0)) == SOCKET_ERROR){
				DWORD error = WSAGetLastError();
				if(error != WSAEWOULDBLOCK){
					err = error;
				}
				break;
			}

			if(llen == 0) {
				err = is_disconnected;
				break;
			}
			
			if (llen == size){
				unsigned int newllen = s->_read_buff->buff_size*2;
				char * tmp = async_net::detail::BuffPool::get(newllen);

				memcpy(tmp, s->_read_buff->buff, s->_read_buff->buff_size);
				async_net::detail::BuffPool::release(s->_read_buff->buff, s->_read_buff->buff_size);
				s->_read_buff->buff = tmp;
				s->_read_buff->buff_size = newllen;
			}

			s->_read_buff->slide += llen;
		}

		if (s->isrecv){
			if (!s->fn_onRecv.empty()){
				s->fn_onRecv(s->_read_buff->buff, s->_read_buff->slide, 0);
			}
			s->_read_buff->slide = 0;
			
			if (!err){
				err = do_async_recv();
			}
		}
	}
	
	if (err){
		if (!s->fn_onRecv.empty()){
			s->fn_onRecv(0, 0, err);
		}
	}
}	

int base_socket_win32::async_send(){
	return do_async_send();
}

int base_socket_win32::do_async_send(){
	if (s->_write_buff->send_buff()){
		DWORD flag = 0;
		DWORD llen = 0;

		OverlappedEX * ovl = detail::OverlappedEXPool<OverlappedEX >::get();
		ovl->type = win32_tcp_send_complete;
		
		if (WSASend(fd, 
					s->_write_buff->send_buff_.load()->_wsabuf, 
					s->_write_buff->send_buff_.load()->_wsabuf_slide.load(), 
					&llen,
					flag, 
					&ovl->overlap, 
					0) == SOCKET_ERROR){
			DWORD err = WSAGetLastError();
			if (err != WSA_IO_PENDING){
				return err;
			}
		}
	}

	return socket_succeed;
}

void base_socket_win32::OnSend(_error_code err){
	if (!err){
		s->_write_buff->clear();

		if (!s->fn_onSend.empty()){
			s->fn_onSend(0);
		}

		err = do_async_send();
	}

	if (err){
		if (!s->fn_onSend.empty()){
			s->fn_onSend(err);
		}
	}
}	

int base_socket_win32::async_connect(sock_addr addr){
	_remote_addr = addr;
	tryconnectcount = 0;
	
	return do_async_connect();
}

int base_socket_win32::do_async_connect() {
	sockaddr_in service;
	memset(&service, 0, sizeof(sockaddr_in));
	service.sin_family = AF_INET;
	service.sin_addr = _remote_addr.sin_addr;
	service.sin_port = _remote_addr._port;
	
	DWORD llen = 0;
	
	OverlappedEX * ovl = detail::OverlappedEXPool<OverlappedEX >::get();
	ovl->type = win32_tcp_connect_complete;
	
	DWORD dwBytes;
	GUID GuidConnectEx = WSAID_CONNECTEX;
	LPFN_CONNECTEX fn_CONNECTEX;
	WSAIoctl(
		fd, 
		SIO_GET_EXTENSION_FUNCTION_POINTER, 
		&GuidConnectEx, 
		sizeof(GuidConnectEx),
		&fn_CONNECTEX, 
		sizeof(fn_CONNECTEX), 
		&dwBytes, 
		NULL, 
		NULL);

	if (!fn_CONNECTEX(fd, (sockaddr *)&service, sizeof(sockaddr), 0, 0, &llen, &ovl->overlap)){
		DWORD err = WSAGetLastError();
		if (err != ERROR_IO_PENDING){
			return err;
		}
	}

	return socket_succeed;
}

void base_socket_win32::OnConnect(_error_code err) {
	tryconnectcount++;
	if (err){
		if (tryconnectcount < 3){
			do_async_connect();
		}
	}else{
		setsockopt(fd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
		s->isdisconnect = false;
	}

	if (!s->fn_onConnect.empty()){
		s->fn_onConnect(err);
	}
}

int base_socket_win32::disconnect(){
	CancelIo((HANDLE)fd);

	OverlappedEX * ovl = detail::OverlappedEXPool<OverlappedEX >::get();
	ovl->type = win32_tcp_disconnect_complete;
	
	return do_disconnect(&ovl->overlap);
}

void base_socket_win32::onDeconnect(_error_code err){
	if (!err){
		s->isrecv = false;
		s->isaccept = false;
	}
}

int base_socket_win32::do_disconnect(LPOVERLAPPED povld){
	DWORD dwBytes;
	GUID GuidDisconnectEx = WSAID_DISCONNECTEX;
	LPFN_DISCONNECTEX fn_DISCONNECTEX;
	
	WSAIoctl(
		fd, 
		SIO_GET_EXTENSION_FUNCTION_POINTER, 
		&GuidDisconnectEx, 
		sizeof(GuidDisconnectEx),
		&fn_DISCONNECTEX, 
		sizeof(fn_DISCONNECTEX), 
		&dwBytes, 
		NULL, 
		NULL);

	if (!fn_DISCONNECTEX(fd, povld, TF_REUSE_SOCKET, 0)){
		DWORD err = WSAGetLastError();
		if (err != ERROR_IO_PENDING){
			return err;
		}
	}

	_service->nConnect--;

	return socket_succeed;
}

int base_socket_win32::closesocket(){
	if (s->isdisconnect){
		onClose(fd);
	}else{
		CancelIo((HANDLE)fd);

		OverlappedEX_close * ovl = detail::OverlappedEXPool<OverlappedEX_close>::get();
		ovl->fd = fd;
		ovl->overlapex.type = win32_tcp_close_complete;
	
		return do_disconnect(&ovl->overlapex.overlap);
	}

	return socket_succeed;
}

void base_socket_win32::onClose(SOCKET fd){
	Releasefd(fd);
}


SOCKET base_socket_win32::Getfd(){
	SOCKET fd = (SOCKET)_sock_pool.pop();
	if (fd == 0){
		fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	}
	
	int nZeroSend = 0;
	if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&nZeroSend, sizeof(nZeroSend)) == SOCKET_ERROR){
		DWORD err = WSAGetLastError();
	}

	int nZeroRecv = 0;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&nZeroRecv, sizeof(nZeroRecv)) == SOCKET_ERROR){
		DWORD err = WSAGetLastError();
	}

	BOOL bNodelay = true;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&bNodelay, sizeof(BOOL)) == SOCKET_ERROR){
		DWORD err = WSAGetLastError();
	}

	unsigned long ul = 1;
	if (ioctlsocket(fd, FIONBIO, &ul) == SOCKET_ERROR){
		DWORD err = WSAGetLastError();
	}

	BOOL bSet = TRUE;
	if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&bSet, sizeof(bSet)) == 0){
		tcp_keepalive alive_in;
		tcp_keepalive alive_out;
		unsigned long ulBytesRet;

		alive_in.keepaliveinterval = 1000;
		alive_in.keepalivetime = 500;
		alive_in.onoff = true;

		WSAIoctl(fd, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in), &alive_out, sizeof(alive_out), &ulBytesRet, 0, 0);
	}

	return fd;
}

void base_socket_win32::Releasefd(SOCKET fd){
	if (_sock_pool.size() > 1024){
		::closesocket(fd);
	}else{
		_sock_pool.put((unsigned int *)fd);
	}
}

} //win32
} //async_net
} //angelica

#endif //_WIN32