/*
 * socket_base.cpp
 * Created on: 2012-10-18
 *	   Author: qianqians
 * socket ½Ó¿Ú
 */
#include "sock_addr.h"
#include "sock_buff.h"
#include "socket_base.h"
#include "socket.h"
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
	_write_buff(detail::WriteBuffPool::get()),
#ifdef _WIN32
	fd(_service_._impl)
#endif
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

int socket_base::bind(sock_addr addr){
	int ret = socket_succeed;

	if(isclosed){
		ret = is_closed;
	}else if(!isdisconnect){
		ret = is_connected;
	}else{
#ifdef _WIN32
		ret = fd.bind(addr);
#endif
	}

	return ret;
}

int socket_base::closesocket(){
	if(isclosed){
		return is_closed;
	}

#ifdef _WIN32
	return fd.closesocket();
#endif
}

int socket_base::disconnect(){
	if(isclosed){
		return is_closed;
	}else if(isdisconnect){
		return is_disconnected;
	}

	isdisconnect = true;
#ifdef _WIN32
	return fd.disconnect();
#endif
}

int socket_base::async_accpet(int num, boost::function<void(socket s, sock_addr & addr, _error_code err)> onAccpet, bool bflag){
	if (bflag == true) {
		if (isclosed){
			return is_closed;
		}else if(!isdisconnect){
			return is_connected;
		}

		if (num != 0){
			_service->nMaxConnect = num;
		}

		if (!isaccept){
			isaccept = true;

			fn_onAccpet = onAccpet;

#ifdef _WIN32
			return fd.start_accpet();
#endif
		}
	}else{
		if(isclosed){
			return is_closed;
		}else if(!isdisconnect){
			return is_connected;
		}else{
			isaccept = false;
		}

		return socket_succeed;
	}

	return socket_succeed;
}

int socket_base::async_recv(boost::function<void(char * buff, unsigned int lenbuff, _error_code err) > onRecv, bool bflag){
	if (bflag == true){
		if(isclosed){
			return is_closed;
		}else if(isdisconnect){
			return is_disconnected;
		}
		
		if (!isrecv){
			isrecv = true;

			fn_onRecv = onRecv;

#ifdef _WIN32
			return fd.start_recv();
#endif
		}
	}else{
		if(isclosed){
			return is_closed;
		}else if(isdisconnect){
			return is_disconnected;
		}else{
			isrecv = false;
		}

		return socket_succeed;
	}

	return socket_succeed;
}

int socket_base::async_connect(sock_addr addr, boost::function<void(_error_code err)> onConnect){
	if(isclosed){
		return is_closed;
	}else if(!isdisconnect){
		return is_connected;
	}

	fn_onConnect = onConnect;

#ifdef _WIN32
	return fd.async_connect(addr);
#endif
}

int socket_base::async_send(char * buff, unsigned int lenbuff, boost::function<void(_error_code err) > onSend){
	if(isclosed){
		return is_closed;
	}else if(isdisconnect){
		return is_disconnected;
	}

	if (fn_onSend.empty()){
		fn_onSend = onSend;
	}

	_write_buff->write(buff, lenbuff);
#ifdef _WIN32
	return fd.async_send();
#endif
}

} //async_net
} //angelica