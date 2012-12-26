/*
 * activeserver.h
 *
 *  Created on: 2012-4-8
 *      Author: qianqians
 */

#ifndef ACTIVESERVER_H_
#define ACTIVESERVER_H_

#include "event/eventloop.h"
#include <list>

namespace angelica {
namespace active {

class active_server {
private:
	typedef angelica::active::event::event_loop event_loop;

public:
	active_server();
	virtual ~active_server();

private:
	std::list<event_loop> event_loop_list;

};

} /* namespace active */
} /* namespace angelica */
#endif /* ACTIVESERVER_H_ */
