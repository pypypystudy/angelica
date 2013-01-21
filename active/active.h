/*
 * active.h
 *
 *  Created on: 2013-1-21
 *      Author: qianqians
 */
#ifndef _ACTIVE_H
#define _ACTIVE_H

#include <boost/function.hpp>

namespace angelica {
namespace active {

class active_server;
class mirco_active;

class active {
private:
	typedef boost::function<void(void) > event_handle;

public:
	active(active_server & _active_server_);
	~active();

	void post(event_handle _handle);

private:
	active_server * _active_server;
	mirco_active * _mirco_active;

};

} /* namespace mirco_active */
} /* namespace angelica */

#endif //_ACTIVE_H