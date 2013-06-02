/*
 * socket_base_linux.cpp
 * Created on: 2013-5-19
 *	   Author: qianqians
 * socket_base_linux �ӿ�
 */
#ifdef __linux__

#include <netinet/in.h>

#include "../socket.h"
#include "socket_base_linux.h"

namespace angelica{
namespace async_net{
namespace _linux{

socket_base_linux::socket_base_linux(boost::async_service * _impl) : socket_base(_impl), _th(0){
	_sendflag.store(false);

	_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	if (_socket != -1){
		InitSocket(_socket);
	}else{
		throw angelica::excepiton("INVALID_SOCKET");
	}
}

socket_base_linux::~socket_base_linux(){
	if (_th != 0){
		_th->join();
		delete _th;
	}

	close(_socket);
}

socket_base_linux::socket_base_linux(boost::async_service * _impl, int fd) : socket_base(_impl), _th(0){
	_sendflag.store(false);

	InitSocket(fd);
}

void socket_base_linux::InitSocket(int fd){
	int nZeroSend = 0;
	if (setsockopt(_socket, SOL_SOCKET, SO_SNDBUF, (char *)&nZeroSend, sizeof(nZeroSend)) == SOCKET_ERROR){
		int err = errno;
	}

	int nZeroRecv = 0;
	if (setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, (char *)&nZeroRecv, sizeof(nZeroRecv)) == SOCKET_ERROR){
		int err = errno;
	}

	BOOL bNodelay = true;
	if (setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, (char *)&bNodelay, sizeof(BOOL)) == SOCKET_ERROR){
		int err = errno;
	}

	unsigned long ul = 1;
	if (ioctl(_socket, FIONBIO, &ul) == SOCKET_ERROR){
		int err = errno;
	}

	int bSet = 1;
	if (setsockopt(_socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&bSet, sizeof(bSet)) == 0){
		int keepidel = 60;
		setsockopt(_socket, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepidel));

		int keepInterval = 5;
		setsockopt(_socket, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));

		int keepCount = 3;
		setsockopt(_socket, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));
	}
}

int socket_base_linux::bind(sock_addr addr){
	sockaddr_in service;
	memset(&service, 0, sizeof(service));
	service.sin_family = AF_INET;
	service.sin_addr = addr.sin_addr;
	service.sin_port = addr._port;

	if (::bind(_socket, (sockaddr*)&service, sizeof(service)) != 0){
		return errno;
	}

	return socket_succeed;
}

int socket_base_linux::opensocket(sock_addr addr){
	int reuseaddr = 1;
	if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseaddr, sizeof(reuseaddr)) == 0){
		return errno;
	}

	return socket_succeed;
}

int socket_base_linux::closesocket(){
	shutdown(_socket, 2);

	return socket_succeed;
}

int socket_base_linux::async_accpet(int num, bool bflag){
	if (bflag == true){
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
					return errno;
				}
			}

			struct run_accept{
				void operator()(socket_base_linux * _sockethandle){
					while(isaccept){
						if (_service->nConnect < _service->nMaxConnect){
							int acceptsocket = accept(_socket, 0, 0);

							if (acceptsocket != -1){
								_service->nConnect++;

								setnonblocking(acceptsocket);
								_sockethandle->InitSocket(acceptsocket);
								_sockethandle->OnAccept(acceptsocket);
							}
						}else{
							boost::this_thread::sleep(boost::posix_time::microseconds(15));
						}
					}
				}
			};

			_th = new boost::thread(boost::bind(run_accept(), this));
		}
	}

	return socket_succeed;
}

int socket_base_linux::async_accpet(bool bflag){
	if (bflag == true){
		isaccept = false;

		_th->join();
		delete _th;
		_th = 0;
	}

	return socket_succeed;
}

void socket_base_linux::OnAccept(int fd){
	angelica::async_net::socket s;
	s._socket = new socket_base_linux(*_service, fd);
	s._ref = new boost::atomic_uint(1);

	this->onAcceptHandle(s, sock_addr(), 0);
}

int socket_base_linux::async_recv(bool bflag){
	if (bflag == true){
		isrecv = true;

		epoll_event ev;
		ev.events = EPOLLIN | EPOLLET;
		ev.data.ptr = this;
		epoll_ctl(_service->epollfd_read, EPOLL_CTL_ADD, _socket, &ev);
	}else{
		isrecv = false;
	}

	return socket_succeed;
}

void socket_base_linux::OnRecv(){
	int error = socket_succeed;

	while(1){
		std::size_t size = _read_buff->buff_size - _read_buff->slide;
		char * buff = _read_buff->buff + _read_buff->slide;
		if ((llen = recv(fd, buff, size, 0)) == -1){
			int err = errno;
			if (err != EAGAIN){
				error = err;
			}
			break;
		}

		if (llen < size){
			break;
		}

		if (llen == 0) {
			error = is_disconnected;
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

		epoll_event ev;
		ev.events = EPOLLIN | EPOLLET;
		ev.data.ptr = this;
		epoll_ctl(_service->epollfd_read, EPOLL_CTL_ADD, _socket, &ev);
	}

	if (error){
		if (!onRecvHandle.empty()){
			onRecvHandle(0, 0, err);
		}
	}
}

int socket_base_linux::async_connect(const sock_addr & addr){
	ret = socket_succeed;

	sockaddr_in service;
	memset(&service, 0, sizeof(service));
	service.sin_family = AF_INET;
	service.sin_addr = addr.sin_addr;
	service.sin_port = addr._port;
	if (connect(_socket, (sockaddr*)&service, sizeof(sockaddr_in)) == -1){
		ret = errno;
		OnConnect(ret);
	}else{
		OnConnect(0);
	}

	return ret;
}

void socket_base_linux::OnConnect(int err){
	if (!err){
		setsockopt(fd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
		isdisconnect = false;
	}

	if (!onConnectHandle.empty()){
		onConnectHandle(err);
	}
}

int socket_base_linux::async_send(char * buff, unsigned int lenbuff){
	int err = socket_succeed;

	if (isclosed){
		return is_closed;
	}else if (isdisconnect){
		return is_disconnected;
	}

	boost::shared_lock<boost::shared_mutex> lock(_send_shared_mutex);
	if (_sendflag.test_and_set()){
		_write_buff->write(buff, lenbuff);

		boost::unique_lock<boost::shared_mutex> unique_lock(boost::move(lock), boost::try_to_lock);
		if (unique_lock.owns_lock()){
			do_send();
		}
	}else{
		int sendlen = 0, llen = 0;
		while(1){
			if ((sendlen = send(_socket, buff+llen, lenbuff, 0)) == -1){
				int error = errno;
				if (error != EAGAIN){
					err = error;
					break;
				}

				if (llen < lenbuff){
					boost::this_thread::sleep(boost::posix_time::milliseconds(15));
					continue;
				}

				break;
			}

			llen += sendlen;
			if (llen < lenbuff){
				continue;
			}else{
				break;
			}
		}

		if (err == 0){
			epoll_event ev;
			ev.events = EPOLLOUT | EPOLLET;
			ev.data.ptr = this;
			epoll_ctl(_service->epollfd_write, EPOLL_CTL_ADD, _socket, &ev);
		}
	}

	return err;
}

void socket_base_linux::OnSend(){
	boost::unique_lock<boost::shared_mutex> unique_lock(_send_shared_mutex, boost::try_to_lock);
	if (unique_lock.owns_lock()){
		if (do_send() == 0){
			_sendflag.clear();
		}
	}
}

int socket_base_linux::do_send(){
	int llen = 0;
	if (_write_buff->send_buff()){
		int sendlen = 0, err = 0;
		char * buff = _write_buff->send_buff_.load()->buff;
		int llenbuf = _write_buff->send_buff_.load()->slide.load();
		if (llenbuf > 0){
			while(1){
				if ((sendlen = send(_socket, buff +llen, llenbuf, 0)) == -1){
					int error = errno;
					if (error != EAGAIN){
						err = error;
						break;
					}

					if (llen < _write_buff->send_buff_.load()->_wsabuf_slide.load()){
						boost::this_thread::sleep(boost::posix_time::milliseconds(15));
						continue;
					}

					break;
				}

				llen += sendlen;
				if (llen < lenbuff){
					continue;
				}else{
					break;
				}
			}

			if (err == 0){
				epoll_event ev;
				ev.events = EPOLLOUT | EPOLLET;
				ev.data.ptr = this;
				epoll_ctl(_service->epollfd_write, EPOLL_CTL_ADD, _socket, &ev);
			}
		}
	}

	return llen;
}

}// linux
}// async_net
}// angelica

#endif //__linux__
