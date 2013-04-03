/*
 * iocp_impl.cpp
 *  Created on: 2012-10-1
 *      Author: qianqians
 * IOCP
 */
#ifdef _WIN32

#include <sstream>
#include <string>
#include <boost/exception/exception.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <angelica/detail\tools.h>

#include "iocp_impl.h"
#include "Overlapped.h"
#include "base_socket_win32.h"
#include "../async_service.h"

namespace angelica {
namespace async_net {
namespace win32 {

iocp_impl::iocp_impl() : thread_count(0) {
	SYSTEM_INFO info;
	GetSystemInfo(&info);
		
	current_num = info.dwNumberOfProcessors;

	WSADATA data;
	WSAStartup(MAKEWORD(2,2), &data);

	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if(hIOCP == 0) {
		std::stringstream error_buff;  
		error_buff << "CreateIoCompletionPort failed. Error" << GetLastError();
		std::string error_info;
		error_buff >> error_info;
		throw boost::throw_function(error_info.c_str());
	}

	detail::OverlappedEXPool<OverlappedEX >::Init();
	detail::OverlappedEXPool<OverlappedEX_close>::Init();
	detail::OverlappedEXPool<OverlappedEX_Accept >::Init();
}

iocp_impl::~iocp_impl() {
	CloseHandle(hIOCP);
	WSACleanup();
}

void iocp_impl::start(unsigned int nCurrentNum) {
	if (nCurrentNum == 0){
		nCurrentNum = current_num;
	}

	for (unsigned int i = 0; i < nCurrentNum; i++) {
		DWORD threadid;
		HANDLE threadHandle = CreateThread(NULL, 0, &iocp_impl::serverwork, (void *)this, 0, &threadid);
		if (threadHandle == NULL) {
			std::stringstream error_buff;  
			error_buff << "CreateThread failed. Error" << GetLastError();
			std::string error_info;
			error_buff >> error_info;
			throw boost::throw_function(error_info.c_str());
		}
		
		CloseHandle(threadHandle);
	}
}

DWORD WINAPI iocp_impl::serverwork(void * impl) {
	iocp_impl * _impl = (iocp_impl * )impl;
	_impl->thread_count++;
	async_service * _service = container_of(_impl, async_service, _impl);

	while(1) {
		while(_service->do_one());

		DWORD nBytesTransferred = 0;
		base_socket_win32 * pHandle = 0;
		LPOVERLAPPED pOverlapped = 0;
		_error_code err = 0;

		BOOL bret = GetQueuedCompletionStatus(_impl->hIOCP, &nBytesTransferred, (PULONG_PTR)&pHandle, &pOverlapped, INFINITE);
		if(!bret) {
			err = GetLastError();
		}
			
		OverlappedEX * pOverlappedEX = container_of(pOverlapped, OverlappedEX, overlap);
		if (pOverlappedEX->type == win32_tcp_send_complete){
			pHandle->OnSend(err);
			detail::OverlappedEXPool<OverlappedEX >::release(pOverlappedEX);
		}else if (pOverlappedEX->type == win32_tcp_recv_complete){
			pHandle->OnRecv(nBytesTransferred, err);
			detail::OverlappedEXPool<OverlappedEX >::release(pOverlappedEX);
		}else if (pOverlappedEX->type == win32_tcp_connect_complete){
			pHandle->OnConnect(err);
			detail::OverlappedEXPool<OverlappedEX >::release(pOverlappedEX);
		}else if (pOverlappedEX->type == win32_tcp_accept_complete){
			OverlappedEX_Accept * _OverlappedEXAccept = container_of(pOverlappedEX, OverlappedEX_Accept, overlapex);
			pHandle->OnAccept(_OverlappedEXAccept->socket_, nBytesTransferred, err);
			detail::OverlappedEXPool<OverlappedEX_Accept >::release(_OverlappedEXAccept);
		}else if (pOverlappedEX->type == win32_tcp_disconnect_complete){
			pHandle->onDeconnect(err);
			detail::OverlappedEXPool<OverlappedEX >::release(pOverlappedEX);
		}else if (pOverlappedEX->type == win32_tcp_close_complete){
			OverlappedEX_close * _OverlappedEXClose = container_of(pOverlappedEX, OverlappedEX_close, overlapex);
			pHandle->onClose(_OverlappedEXClose->fd);
			detail::OverlappedEXPool<OverlappedEX >::release(pOverlappedEX);
		}else if (pOverlappedEX->type == win32_stop_){
			detail::OverlappedEXPool<OverlappedEX >::release(pOverlappedEX);
			break;
		}
	}
	
	_impl->thread_count--;

	return 0;
}

void iocp_impl::stop(){
	for(unsigned int i = 0; i < current_num; i++) {
		OverlappedEX * olp = detail::OverlappedEXPool<OverlappedEX >::get();
		olp->type = win32_stop_;
		PostQueuedCompletionStatus(this->hIOCP, 0, 0, &olp->overlap);
	}

	while (thread_count.load() != 0){
		SwitchToThread();
	}
}

} //win32
} //async_net
} //angelica

#endif //_WIN32