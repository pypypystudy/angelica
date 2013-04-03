/*
 * buff_pool.cpp
 * Created on: 2013-2-24
 *	   Author: qianqians
 * buff_pool ½Ó¿Ú
 */
#include "buff_pool.h"

namespace angelica {
namespace async_net {

namespace detail {

unsigned int page_size = 8192;

BuffPool * BuffPool::m_pBuffPool = 0;

} //detail

} //async_net
} //angelica