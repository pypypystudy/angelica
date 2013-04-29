/*
 * read_buff_pool.h
 * Created on: 2013-2-24
 *	   Author: qianqians
 * read_buff_pool ½Ó¿Ú
 */
#ifndef _READ_BUFF_POOL_H
#define _READ_BUFF_POOL_H

#include "sock_buff.h"

#include <angelica/container/no_blocking_pool.h>

namespace angelica {
namespace async_net {

namespace detail {

class ReadBuffPool{
public:
	static void Init(){
		m_pReadBuffPool = new ReadBuffPool();
	}

	static read_buff * get(){
		read_buff * _read_buff = m_pReadBuffPool->_read_buff_pool.pop();
		if (_read_buff == 0){
			_read_buff = new read_buff();
		}
		return _read_buff;
	}

	static void release(read_buff * _read_buff){
		m_pReadBuffPool->_read_buff_pool.put(_read_buff);
	}

private:
	static ReadBuffPool * m_pReadBuffPool;

	angelica::container::no_blocking_pool<read_buff > _read_buff_pool;

};

} //detail

} //async_net
} //angelica

#endif //_READ_BUFF_POOL_H