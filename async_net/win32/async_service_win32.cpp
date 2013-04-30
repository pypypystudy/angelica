/*
 * async_service_win32.cpp
 *   Created on: 2012-11-14
 *       Author: qianqians
 * async_service win32 й╣ож
 */
#ifdef _WIN32

#include "winhdef.h"

#include <boost/exception/all.hpp>

#include "../async_service.h"
#include "../socket.h"
#include "../socket_pool.h"
#include "../buff_pool.h"
#include "../read_bufff_pool.h"
#include "../write_buff_pool.h"

#include "Overlapped.h"
#include "socket_base_win32.h"

namespace angelica { 
namespace async_net { 

async_service::async_service() : nConnect(0), nMaxConnect(0) {
	SYSTEM_INFO info;
	GetSystemInfo(&info);
		
	current_num = info.dwNumberOfProcessors;

	WSADATA data;
	WSAStartup(MAKEWORD(2,2), &data);

	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if(hIOCP == 0) {
		BOOST_THROW_EXCEPTION(std::logic_error("Error: CreateIoCompletionPort failed."));
	}

	detail::SocketPool::Init();
	detail::BuffPool::Init(detail::page_size);
	detail::ReadBuffPool::Init();
	detail::WriteBuffPool::Init();

	win32::detail::OverlappedEXPool<win32::OverlappedEX >::Init();
	win32::detail::OverlappedEXPool<win32::OverlappedEX_close>::Init();
	win32::detail::OverlappedEXPool<win32::OverlappedEX_Accept >::Init();
}

async_service::~async_service(){
	CloseHandle(hIOCP);
	WSACleanup();
}

void async_service::stop(){
	for(unsigned int i = 0; i < current_num; i++) {
		win32::OverlappedEX * olp = win32::detail::OverlappedEXPool<win32::OverlappedEX >::get();
		olp->type = win32_stop_;
		PostQueuedCompletionStatus(this->hIOCP, 0, 0, &olp->overlap);
	}

	_th_group.join_all();
}

void async_service::serverwork() {
	thread_count++;

	while(1) {
		while(do_one());

		DWORD nBytesTransferred = 0;
		socket_base * pHandle = 0;
		LPOVERLAPPED pOverlapped = 0;
		_error_code err = 0;

		BOOL bret = GetQueuedCompletionStatus(hIOCP, &nBytesTransferred, (PULONG_PTR)&pHandle, &pOverlapped, INFINITE);
		if(!bret) {
			err = GetLastError();
		}
			
		win32::OverlappedEX * pOverlappedEX = container_of(pOverlapped, win32::OverlappedEX, overlap);
		if (pOverlappedEX->type == win32_tcp_send_complete){
			((win32::socket_base_win32*)pHandle)->OnSend(err);
			win32::detail::OverlappedEXPool<win32::OverlappedEX >::release(pOverlappedEX);
		}else if (pOverlappedEX->type == win32_tcp_recv_complete){
			((win32::socket_base_win32*)pHandle)->OnRecv(nBytesTransferred, err);
			win32::detail::OverlappedEXPool<win32::OverlappedEX >::release(pOverlappedEX);
		}else if (pOverlappedEX->type == win32_tcp_connect_complete){
			((win32::socket_base_win32*)pHandle)->OnConnect(err);
			win32::detail::OverlappedEXPool<win32::OverlappedEX >::release(pOverlappedEX);
		}else if (pOverlappedEX->type == win32_tcp_disconnect_complete){
			((win32::socket_base_win32*)pHandle)->onDeconnect(err);
			win32::detail::OverlappedEXPool<win32::OverlappedEX >::release(pOverlappedEX);
		}else if (pOverlappedEX->type == win32_tcp_accept_complete){
			win32::OverlappedEX_Accept * _OverlappedEXAccept = container_of(pOverlappedEX, win32::OverlappedEX_Accept, overlapex);
			((win32::socket_base_win32*)pHandle)->OnAccept(_OverlappedEXAccept->socket_, nBytesTransferred, err);
			win32::detail::OverlappedEXPool<win32::OverlappedEX_Accept >::release(_OverlappedEXAccept);
		}else if (pOverlappedEX->type == win32_tcp_close_complete){
			win32::OverlappedEX_close * _OverlappedEXClose = container_of(pOverlappedEX, win32::OverlappedEX_close, overlapex);
			((win32::socket_base_win32*)pHandle)->onClose();
			win32::detail::OverlappedEXPool<win32::OverlappedEX_close >::release(_OverlappedEXClose);
		}else if (pOverlappedEX->type == win32_stop_){
			win32::detail::OverlappedEXPool<win32::OverlappedEX >::release(pOverlappedEX);
			break;
		}
	}
	
	thread_count--;
}

} //async_net
} //angelica

#endif //_WIN32