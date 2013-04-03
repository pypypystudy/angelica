/*
 * socke_poolt.cpp
 * Created on: 2013-2-24
 *	   Author: qianqians
 * socket ½Ó¿Ú
 */
#include "socket_pool.h"

namespace angelica {
namespace async_net {

namespace detail {

SocketPool * SocketPool::m_pSocketPool = 0;

} //detail

} //async_net
} //angelica