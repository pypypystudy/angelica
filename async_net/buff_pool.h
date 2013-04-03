/*
 * buff_pool.h
 * Created on: 2013-2-24
 *	   Author: qianqians
 * buff_pool ½Ó¿Ú
 */
#ifndef _BUFF_POOL_H
#define _BUFF_POOL_H

#include <angelica/container/no_blocking_pool.h>

namespace angelica {
namespace async_net {

namespace detail {

class BuffPool{
public:
	static void Init(size_t _page_size){
		m_pBuffPool = new BuffPool();
		m_pBuffPool->_size = _page_size;
	}

	static char * get(size_t _size){
		char * buff = 0;
		if (_size <= m_pBuffPool->_size){
			if ((buff = m_pBuffPool->_buff_pool.pop()) == 0){
				buff = new char[m_pBuffPool->_size];
			}
		}else{
			buff = new char[_size];
		}

#ifdef _DEBUG
		memset(buff, 0, _size);
#endif //_DEBUG

		return buff;
	}

	static void release(char * buff, size_t _size){
		if (_size <= m_pBuffPool->_size){
			m_pBuffPool->_buff_pool.put(buff);
		}else{
			free(buff);
		}
	}

private:
	static BuffPool * m_pBuffPool;

	size_t _size;
	angelica::container::no_blocking_pool<char > _buff_pool;

};

extern unsigned int page_size;

} //detail

} //async_net
} //angelica

#endif //_BUFF_POOL_H
