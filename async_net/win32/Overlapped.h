/*
 * Overlapped.h
 *  Created on: 2012-10-16
 *		Author: qianqians
 * ��չOverlapped�ṹ
 * ����Overlapped�����
 */
#ifndef _OVERLAPPED_H
#define _OVERLAPPED_H

#ifdef _WIN32

#include "winhdef.h"
#include "../error_code.h"

#include <boost/function.hpp>

namespace angelica {
namespace async_net {
namespace win32 {

//��չOverlapped�ṹ
struct OverlappedEX {
	int isstop;

	boost::function<void() > fn_Destory;
	boost::function<void(std::size_t, _error_code ) > fn_onHandle;

	OVERLAPPED overlap;
};

namespace detail {

//Overlapped����ؽӿ�
void InitOverlappedPool();
OverlappedEX * GetOverlapped();
void DestroyOverlappedPool();

}// detail

} //win32
} //async_net
} //angelica

#endif //_WIN32
#endif //_OVERLAPPED_H