/*
 * async_service_linux.h
 *   Created on: 2012-10-16
 *       Author: qianqians
 * async_service linux
 */
#ifdef __linux__

#include <angelica/exception/exception.hpp>

#include "../async_service.h"
#include "socket_base_linux.h"

static int MAX_EVENTS = 128;

async_service::async_service(){
	epollfd = epoll_create(0);
	if (epollfd == -1){
		throw angelica::exception("Error: epoll_create failed.");
	}

	Init();
}

async_service::~async_service(){
	close(epollfd);
}

bool async_service::network(){
	epoll_event events[MAX_EVENTS];

	int ndfs = epoll_wait(epollfd_read, events, MAX_EVENTS, 15);
	for(int i = 0; i < ndfs; i++){
		if (events[i].events == EPOLLIN){
			((socket_base_linux*)events[i].ptr)->OnRecv();
		}
	}

	int ndfs = epoll_wait(epollfd_write, events, MAX_EVENTS, 15);
	for(int i = 0; i < ndfs; i++){
		if(events[i].events == EPOLLOUT){
			((socket_base_linux*)events[i].ptr)->OnSend();
		}
	}

	return true;
}

#endif //__linux__
