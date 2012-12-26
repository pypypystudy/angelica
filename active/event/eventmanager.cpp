/*
 * eventmanager.cpp
 *
 *  Created on: 2012-4-15
 *      Author: qianqians
 */

#include "eventmanager.h"

namespace angelica {
namespace active {
namespace event {

event_manager::event_manager() {
	// TODO Auto-generated constructor stub

}

event_manager::~event_manager() {
	// TODO Auto-generated destructor stub
}

void event_manager::do_idle_event()
{
	/*if (!idle_event_queue.empty())
	{
		boost::function<void(void)> fn = idle_event_queue.pop();

		fn();
	}
	else
	{
		sleep(5);
	}*/
}

} /* namespace event */
} /* namespace active */
} /* namespace angelica */
