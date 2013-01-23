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

	detail::InitWin32SOCKETPool();
}

iocp_impl::~iocp_impl() {
	detail::DestroyWin32SOCKETPool();

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
		void* pHandle = 0;
		LPOVERLAPPED pOverlapped = 0;
		_error_code err = 0;

		BOOL bret = GetQueuedCompletionStatus(_impl->hIOCP, &nBytesTransferred, (PULONG_PTR)&pHandle, &pOverlapped, INFINITE);
		if(!bret) {
			err = GetLastError();
		}
			
		OverlappedEX * pOverlappedEX = container_of(pOverlapped, OverlappedEX, overlap);
		if(pOverlappedEX->isstop == 1){
			detail::DestroyOverlapped(pOverlappedEX);
			break;
		}else{
			pOverlappedEX->fn_onHandle(nBytesTransferred, err);
			detail::DestroyOverlapped(pOverlappedEX);
		}
	}
	
	_impl->thread_count--;

	return 0;
}

void iocp_impl::stop(){
	for(unsigned int i = 0; i < current_num; i++) {
		OverlappedEX * olp = detail::CreateOverlapped();
		olp->isstop = 1;
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