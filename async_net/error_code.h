/*
 * error.h
 * Created on: 2012-10-16
 *	   Author: qianqians
 * error code
 */

#ifndef _ERROR_H
#define _ERROR_H

namespace angelica {
namespace async_net {

typedef int _error_code;

enum err_code{
	socket_succeed = 0,
	invalid_addr = 100001,
	is_closed = 100002,
	is_connected = 100003,
	is_disconnected = 100004,
};

} //async_net
} //angelica

#endif //_ERROR_H