/*
 * eventloop.h
 *
 *  Created on: 2012-3-27
 *      Author: qianqians
 */

#ifndef EVENTLOOP_H_
#define EVENTLOOP_H_

#include "eventmanager.h"
#include "eventqueue.h"

#include <boost/thread/condition.hpp>

namespace angelica {
namespace active {
namespace event {

class event_loop {
private:
	//typedef angelica::active::event::event_queue<boost::function<void(void)> > event_queue;

public:
	event_loop(event_manager*);
	virtual ~event_loop();

	void loop();
	void quit();

	bool idle();

private:
	void do_event();
	void do_idle_event();

	bool bloop;
	bool bidle;

	//event_queue *pt_event_queue;
	//event_queue *pt_idle_event_queue;

};

} /* namespace event */
} /* namespace active */
} /* namespace angelica */
#endif /* EVENTLOOP_H_ */
