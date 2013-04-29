/*
 * socket_base.cpp
 * Created on: 2012-10-18
 *	   Author: qianqians
 * socket ½Ó¿Ú
 */
#ifdef _WIN32

#include "winhdef.h"

#include "../socket.h"
#include "../sock_addr.h"
#include "../sock_buff.h"
#include "../socket_pool.h"
#include "../read_bufff_pool.h"
#include "../write_buff_pool.h"
#include "../buff_pool.h"
#include "socket_base_win32.h"
#include "Overlapped.h"

namespace angelica {
namespace async_net {
namespace win32 {
 
static angelica::container::no_blocking_pool<unsigned int> _sock_pool;

SOCKET Getfd(){
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

void Releasefd(SOCKET fd){
	if (_sock_pool.size() > 1024){
		::closesocket(fd);
	}else{
		_sock_pool.put((unsigned int *)fd);
	}
}



socket_base_win32::socket_base_win32(async_service & _impl) : 
	socket_base(_impl),
	isListen(false)
{
	fd = win32::Getfd();

	if (CreateIoCompletionPort((HANDLE)fd, _service->hIOCP, (ULONG_PTR)this, 0) != _service->hIOCP){
		DWORD err = WSAGetLastError();
		BOOST_THROW_EXCEPTION(std::logic_error("Error: CreateIoCompletionPort failed."));
	}
}

socket_base_win32::~socket_base_win32(){
}

int socket_base_win32::bind(sock_addr addr){
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

int socket_base_win32::do_disconnect(LPOVERLAPPED povld){
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

int socket_base_win32::disconnect(){
	CancelIo((HANDLE)fd);

	OverlappedEX * ovl = detail::OverlappedEXPool<OverlappedEX >::get();
	ovl->type = win32_tcp_disconnect_complete;
	
	return do_disconnect(&ovl->overlap);
}

void socket_base_win32::onDeconnect(_error_code err){
	if (!err){
		isrecv = false;
		isaccept = false;
	}
}

int socket_base_win32::closesocket(){
	if (isdisconnect){
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

void socket_base_win32::onClose(SOCKET fd){
	Releasefd(fd);
}

int socket_base_win32::do_async_accpet(){
	DWORD dwBytes;
	socket_base_win32 * _client_socket = (socket_base_win32 *)async_net::detail::SocketPool::get(*_service);
	OverlappedEX_Accept * olp = detail::OverlappedEXPool<OverlappedEX_Accept >::get();
	olp->socket_ = _client_socket;
	olp->overlapex.type = win32_tcp_accept_complete;
			
	if (!AcceptEx(fd, 
				  _client_socket->fd, 
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

int socket_base_win32::async_accpet(int num, AcceptHandle _onAccpet, bool bflag){
	int ret = socket_succeed;

	if (bflag == true) {
		if (isclosed){
			return is_closed;
		}else if(!isdisconnect){
			return is_connected;
		}

		if (num != 0){
			_service->nMaxConnect = num;
		}

		if (!isaccept){
			isaccept = true;

			if (!isListen){
				if (listen(fd, 100) == SOCKET_ERROR) {
					return WSAGetLastError();
				} 
			}

			if (!flagAcceptHandle.test_and_set()){
				onAcceptHandle = _onAccpet;
			}
	
			_error_code ret = socket_succeed;

			SYSTEM_INFO info;
			GetSystemInfo(&info);
			for(DWORD i = 0; i < info.dwNumberOfProcessors * 64; i++){
				if ((ret = do_async_accpet()) == socket_succeed){
					break;
				}
			}
		}
	}else{
		if(isclosed){
			return is_closed;
		}else if(!isdisconnect){
			return is_connected;
		}else{
			isaccept = false;
		}
	}

	return ret;
}

void socket_base_win32::OnAccept(socket_base * sClient, DWORD llen, _error_code err) {
	sock_addr addr;

	if ((_service->nConnect++ < _service->nMaxConnect) && isaccept){
		do_async_accpet();
	}
	
	if(isclosed){
		err = is_closed;
		Releasefd(((socket_base_win32*)sClient)->fd);
	}

	if (err){
		Releasefd(((socket_base_win32*)sClient)->fd);
	}else{
		LPSOCKADDR local_addr = 0;
		int local_addr_length = 0;
		LPSOCKADDR remote_addr = 0;
		int remote_addr_length = 0;
		GetAcceptExSockaddrs(
			((socket_base_win32*)sClient)->_read_buff->buff, 
			llen, 
			sizeof(sockaddr_in) + 16, 
			sizeof(sockaddr_in) + 16, 
			&local_addr,
			&local_addr_length,
			&remote_addr,
			&remote_addr_length);
	
		if (remote_addr_length > sizeof(sockaddr_in) + 16) {
			Releasefd(((socket_base_win32*)sClient)->fd);
		}else{
			if (llen > 0){
				((socket_base_win32*)sClient)->_read_buff->slide += llen;
			}

			setsockopt(((socket_base_win32*)sClient)->fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&fd, sizeof(fd));
			if (CreateIoCompletionPort((HANDLE)((socket_base_win32*)sClient)->fd, _service->hIOCP, 0, 0) != _service->hIOCP){
				DWORD err = WSAGetLastError();
			}

			addr = remote_addr;

			((socket_base_win32*)sClient)->isdisconnect = false;
		}
	}

	socket socket_;
	socket_._socket = sClient;
	if (!onAcceptHandle.empty()){
		onAcceptHandle(socket_, addr, err);
	}
}

int socket_base_win32::async_recv(RecvHandle _onRecv, bool bflag){
	if (bflag == true){
		if(isclosed){
			return is_closed;
		}else if(isdisconnect){
			return is_disconnected;
		}
		
		if (!isrecv){
			isrecv = true;

			if(!flagRecvHandle.test_and_set()){
				onRecvHandle = _onRecv;
			}

			if (_read_buff->slide > 0){
				if (!onRecvHandle.empty()){
					onRecvHandle(_read_buff->buff, _read_buff->slide, socket_succeed);
				}
				_read_buff->slide = 0;
			}

			return do_async_recv();
		}
	}else{
		if(isclosed){
			return is_closed;
		}else if(isdisconnect){
			return is_disconnected;
		}else{
			isrecv = false;
		}

		return socket_succeed;
	}

	return socket_succeed;
}

int socket_base_win32::do_async_recv(){
	DWORD flag = 0;
	DWORD llen = 0;
	
	OverlappedEX * ovl = detail::OverlappedEXPool<OverlappedEX >::get();
	ovl->type = win32_tcp_recv_complete;
	
	_read_buff->_wsabuf.buf = 0;
	_read_buff->_wsabuf.len = 0;
	if (WSARecv(fd, &_read_buff->_wsabuf, 1, &llen, &flag, &ovl->overlap, 0) == SOCKET_ERROR){
		DWORD err = WSAGetLastError();

		if (err != WSA_IO_PENDING){
			return err;
		}
	}

	return socket_succeed;
}	

void socket_base_win32::OnRecv(DWORD llen, _error_code err) { 
	if (!err){
		while(1){	
			std::size_t size = _read_buff->buff_size - _read_buff->slide;
			char * buff = _read_buff->buff + _read_buff->slide;
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
				unsigned int newllen = _read_buff->buff_size*2;
				char * tmp = async_net::detail::BuffPool::get(newllen);

				memcpy(tmp, _read_buff->buff, _read_buff->buff_size);
				async_net::detail::BuffPool::release(_read_buff->buff, _read_buff->buff_size);
				_read_buff->buff = tmp;
				_read_buff->buff_size = newllen;
			}

			_read_buff->slide += llen;
		}

		if (isrecv){
			if (!onRecvHandle.empty()){
				onRecvHandle(_read_buff->buff, _read_buff->slide, 0);
			}
			_read_buff->slide = 0;
			
			if (!err){
				err = do_async_recv();
			}
		}
	}
	
	if (err){
		if (!onRecvHandle.empty()){
			onRecvHandle(0, 0, err);
		}
	}
}	

int socket_base_win32::async_send(char * buff, unsigned int lenbuff, SendHandle onSend){
	if(isclosed){
		return is_closed;
	}else if(isdisconnect){
		return is_disconnected;
	}

	_write_buff->write(buff, lenbuff, onSend);

	return do_async_send();
}

int socket_base_win32::do_async_send(){
	if (_write_buff->send_buff()){
		DWORD flag = 0;
		DWORD llen = 0;

		OverlappedEX * ovl = detail::OverlappedEXPool<OverlappedEX >::get();
		ovl->type = win32_tcp_send_complete;
		
		if (WSASend(fd, 
					_write_buff->send_buff_.load()->_wsabuf, 
					_write_buff->send_buff_.load()->_wsabuf_slide.load(), 
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

void socket_base_win32::OnSend(_error_code err){
	if (!err){
		_write_buff->clear();

		SendHandleEx onSendHandle;
		while(_write_buff->send_buff_.load()->queSendHandle.pop(onSendHandle)){
			if (!onSendHandle.empty()){
				onSendHandle(err);
			}
		}

		err = do_async_send();
	}

	if (err){
		SendHandleEx onSendHandle;
		if(_write_buff->send_buff_.load()->queSendHandle.pop(onSendHandle)){
			if (!onSendHandle.empty()){
				onSendHandle(err);
			}
		}
	}
}	

int socket_base_win32::async_connect(sock_addr addr, ConnectHandle onConnect){
	if(isclosed){
		return is_closed;
	}else if(!isdisconnect){
		return is_connected;
	}

	if (!flagConnectHandle.test_and_set()){
		onConnectHandle = onConnect;
	}

	_remote_addr = addr;
	tryconnectcount = 0;

	return do_async_connect();
}

int socket_base_win32::do_async_connect() {
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

void socket_base_win32::OnConnect(_error_code err) {
	tryconnectcount++;
	if (err){
		if (tryconnectcount < 3){
			do_async_connect();
		}
	}else{
		setsockopt(fd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
		isdisconnect = false;
	}

	if (!onConnectHandle.empty()){
		onConnectHandle(err);
	}
}

} //win32
} //async_net
} //angelica

#endif //_WIN32