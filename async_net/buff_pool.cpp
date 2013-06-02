/*
 * buff_pool.cpp
 * Created on: 2013-2-24
 *	   Author: qianqians
 * buff_pool �ӿ�
 */
#include "buff_pool.h"

namespace angelica {
namespace async_net {

namespace detail {

#ifdef _WIN32
unsigned int page_size = 8192;
#elif __linux__
unsigned int page_size = 32768;
#endif

BuffPool * BuffPool::m_pBuffPool = 0;

} //detail

} //async_net
} //angelica
