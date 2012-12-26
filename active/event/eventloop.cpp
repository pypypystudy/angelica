/*
 * eventloop.cpp
 *
 *  Created on: 2012-3-27
 *      Author: qianqians
 */

#include "eventloop.h"

namespace angelica {
namespace active {
namespace event {

event_loop::event_loop(event_manager* pt_event_manager_)
	: bloop(false),
	  bidle(false)
	  //pt_event_manager(pt_event_manager_)
{
}

event_loop::~event_loop() {
	// TODO Auto-generated destructor stub
}

void event_loop::do_event()
{
	bidle = false;

	/*while(!pt_event_queue->empty())
	{
		while(!pt_event_queue->empty())
		{
			boost::function<void(void)> fn = pt_event_queue->pop();

			fn();
		}
	}*/
}

void event_loop::do_idle_event()
{
	bidle = true;

	//pt_event_manager->do_idle_event();
	//if (!pt_idle_event_queue->empty())
	//{
	//	boost::function<void(void)> fn = pt_idle_event_queue->pop();

	//	fn();
	//}
}

void event_loop::loop()
{
	bloop = true;

	while(bloop)
	{
		do_event();

		do_idle_event();
	}
}

void event_loop::quit()
{
	bloop = false;
}

bool event_loop::idle()
{
	return bidle;
}

} /* namespace event */
} /* namespace active */
} /* namespace angelica */
