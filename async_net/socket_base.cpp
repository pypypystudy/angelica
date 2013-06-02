/*
 * socket_base.cpp
 * Created on: 2012-10-18
 *	   Author: qianqians
 * socket �ӿ�
 */
#include "sock_addr.h"

#include "sock_buff.h"
#include "socket.h"
#include "socket_base.h"
#include "read_bufff_pool.h"
#include "write_buff_pool.h"

namespace angelica {
namespace async_net {

socket_base::socket_base(async_service * _service_) :
	isclosed(false),
	isrecv(false),
	isaccept(false),
	isdisconnect(true),
	_service(_service_),
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

void socket_base::register_accpet_handle(AcceptHandle onAccpet){
	if (!flagAcceptHandle.test_and_set()){
		onAcceptHandle = onAccpet;
	}
}

void socket_base::register_recv_handle(RecvHandle onRecv){
	if(!flagRecvHandle.test_and_set()){
		onRecvHandle = onRecv;
	}
}
	
void socket_base::register_connect_handle(ConnectHandle onConnect){
	if (!flagConnectHandle.test_and_set()){
		onConnectHandle = onConnect;
	}
}
	
void socket_base::register_send_handle(SendHandle onSend){
	if(!flagSendHandle.test_and_set()){
		onSendHandle = onSend;
	}
}

} //async_net
} //angelica
