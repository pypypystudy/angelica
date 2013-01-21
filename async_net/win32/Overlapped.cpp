/*
 * Overlapped.cpp
 *    Created on: 2012-10-16
 *        Author: qianqians
 * Overlapped对象池接口实现
 */
#ifdef _WIN32

#include "Overlapped.h"
#include <angelica/pool/angmalloc.h>

namespace angelica {
namespace async_net {
namespace win32 {

namespace detail {

OverlappedEX * CreateOverlapped(){
	OverlappedEX * ptr = (OverlappedEX *)angmalloc(sizeof(OverlappedEX));
	::new (ptr) OverlappedEX();
	ZeroMemory(&ptr->overlap, sizeof(OVERLAPPED));
	ptr->isstop = 0;
	ptr->fn_onHandle = 0;
	
	return ptr;
}

void DestroyOverlapped(OverlappedEX * ptr){
	ptr->~OverlappedEX();
	angfree(ptr);
}

}// detail

} //win32
} //async_net
} //angelica

#endif //_WIN32