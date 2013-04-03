/*
 * socke_poolt.h
 * Created on: 2013-2-24
 *	   Author: qianqians
 * socket ½Ó¿Ú
 */
#ifndef _SOCKE_POOLT_H
#define _SOCKE_POOLT_H

#include "socket_base.h"
#include <angelica/container/no_blocking_pool.h>

namespace angelica {
namespace async_net {

namespace detail {

class SocketPool{
public:
	static void Init(){
		m_pSocketPool = new SocketPool();
	}

	static socket_base * get(async_service & _impl){
		socket_base * _socket = m_pSocketPool->_socket_pool.pop();
		if (_socket == 0){
			_socket = new socket_base(_impl);
		}
		return _socket;
	}

	static void release(socket_base * _socket){
		if (m_pSocketPool->_socket_pool.size() > 1024){
			delete _socket;
		}else{
			m_pSocketPool->_socket_pool.put(_socket);
		}
	}

private:	
	static SocketPool * m_pSocketPool;

	angelica::container::no_blocking_pool<socket_base > _socket_pool;

};

} //detail

} //async_net
} //angelica

#endif //_SOCKE_POOLT_H