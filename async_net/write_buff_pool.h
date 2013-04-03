/*
 * write_buff_pool.h
 * Created on: 2013-2-24
 *	   Author: qianqians
 * write_buff_pool ½Ó¿Ú
 */
#ifndef _WRITE_BUFF_POOL_H
#define _WRITE_BUFF_POOL_H

#include "sock_buff.h"
#include <angelica/container/no_blocking_pool.h>

namespace angelica {
namespace async_net {

namespace detail {

class WriteBuffPool{
public:
	static void Init(){
		m_pWriteBuffPool = new WriteBuffPool();
	}

	static write_buff * get(){
		write_buff * _write_buff = m_pWriteBuffPool->_write_buff_pool.pop();
		if (_write_buff == 0){
			_write_buff = new write_buff();
		}
		return _write_buff;
	}

	static void release(write_buff * _write_buff){
		m_pWriteBuffPool->_write_buff_pool.put(_write_buff);
	}

private:
	static WriteBuffPool * m_pWriteBuffPool;

	angelica::container::no_blocking_pool<write_buff > _write_buff_pool;

};

} //detail

} //async_net
} //angelica

#endif // _write_buff_pool_h