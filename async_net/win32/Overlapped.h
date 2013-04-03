/*
 * Overlapped.h
 *  Created on: 2012-10-16
 *		Author: qianqians
 * 扩展Overlapped结构
 * 定义Overlapped对象池
 */
#ifndef _OVERLAPPED_H
#define _OVERLAPPED_H

#ifdef _WIN32

#include "winhdef.h"
#include "../error_code.h"

#include <angelica/container/no_blocking_pool.h>

#include <boost/function.hpp>

namespace angelica {
namespace async_net {

class socket_base;

namespace win32 {

//扩展Overlapped结构
struct OverlappedEX {
	int type;
	OVERLAPPED overlap;
};

struct OverlappedEX_Accept{
	OverlappedEX overlapex;
	socket_base * socket_;
};

struct OverlappedEX_close{
	OverlappedEX overlapex;
	SOCKET fd;
};

namespace detail {

template<typename OverlappedEX_Type>
class OverlappedEXPool{
public:
	static void Init(){
		m_pOverlappedEXPool = new OverlappedEXPool();
	}

	static OverlappedEX_Type * get(){
		OverlappedEX_Type * _OverlappedEX = m_pOverlappedEXPool->_OverlappedEX_pool.pop();
		if (_OverlappedEX == 0){
			_OverlappedEX = new OverlappedEX_Type();
		}else{
			new (_OverlappedEX) OverlappedEX_Type();
		}

		return _OverlappedEX;
	}

	static void release(OverlappedEX_Type * _OverlappedEX){
		_OverlappedEX->~OverlappedEX_Type();
		m_pOverlappedEXPool->_OverlappedEX_pool.put(_OverlappedEX);
	}

private:
	static OverlappedEXPool * m_pOverlappedEXPool;

	angelica::container::no_blocking_pool<typename OverlappedEX_Type > _OverlappedEX_pool;

};

template<typename OverlappedEX_Type>
OverlappedEXPool<OverlappedEX_Type > * OverlappedEXPool<OverlappedEX_Type >::m_pOverlappedEXPool = 0;

}// detail

} //win32
} //async_net
} //angelica

#endif //_WIN32
#endif //_OVERLAPPED_H