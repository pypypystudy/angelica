/*
 * Overlapped.cpp
 *    Created on: 2012-10-16
 *        Author: qianqians
 * Overlapped对象池接口实现
 */
#ifdef _WIN32

#include "../no_blocking_pool.h"
#include "../simple_pool.h"
#include "Overlapped.h"

namespace angelica {
namespace async_net {
namespace win32 {

namespace detail {

typedef angelica::async_net::detail::no_blocking_pool<OverlappedEX> OverlappedEXPool;
static OverlappedEXPool _OverlappedEXPool;

void InitOverlappedPool(){
	OverlappedEX * ptr = new OverlappedEX;
	ptr->fn_Destory = boost::bind(&OverlappedEXPool::release, &_OverlappedEXPool, ptr);
	
	_OverlappedEXPool.release(ptr);
}

OverlappedEX * GetOverlapped(){
	OverlappedEX * ptr = _OverlappedEXPool.get();
	if (ptr == 0) {
		ptr = new OverlappedEX;
		ptr->fn_Destory = boost::bind(&OverlappedEXPool::release, &_OverlappedEXPool, ptr);
	}
	ZeroMemory(&ptr->overlap, sizeof(OVERLAPPED));
	ptr->isstop = 0;
	ptr->fn_onHandle = 0;

	return ptr;
}

void DestroyOverlappedPool(){
	while(1){
		OverlappedEX * ptr = _OverlappedEXPool.get();
		if(ptr == 0){
			break;
		}
		delete ptr;
	}
}

}// detail

} //win32
} //async_net
} //angelica

#endif //_WIN32