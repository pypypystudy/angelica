/*
 * eventmanager.h
 *
 *  Created on: 2012-4-15
 *      Author: qianqians
 */

#ifndef EVENTMANAGER_H_
#define EVENTMANAGER_H_

#include "eventqueue.h"

namespace angelica {
namespace active {
namespace event {

class event_manager {
public:
	event_manager();
	virtual ~event_manager();

	void do_idle_event();

private:
	//event_queue<boost::function<void(void)> > idle_event_queue;

};

} /* namespace event */
} /* namespace active */
} /* namespace angelica */
#endif /* EVENTMANAGER_H_ */
