/*
 * socket_base.cpp
 * Created on: 2012-10-18
 *	   Author: qianqians
 * socket ½Ó¿Ú
 */
#include "sock_addr.h"

#include "sock_buff.h"
#include "socket.h"
#include "socket_base.h"
#include "read_bufff_pool.h"
#include "write_buff_pool.h"

namespace angelica {
namespace async_net {

socket_base::socket_base(async_service & _service_) :
	isclosed(false),
	isrecv(false),
	isaccept(false),
	isdisconnect(true),
	_service(&_service_),
	_read_buff(detail::ReadBuffPool::get()),
	_write_buff(detail::WriteBuffPool::get())
{
}

socket_base::~socket_base(){
	if (_read_buff != 0){
		detail::ReadBuffPool::release(_read_buff);
	}
	if (_write_buff != 0){
		detail::WriteBuffPool::release(_write_buff);
	}
}

} //async_net
} //angelica