/*
 * mirco_active.h
 *
 *  Created on: 2012-3-27
 *      Author: qianqians
 */

#ifndef MIRCO_ACTIVE_H_
#define MIRCO_ACTIVE_H_

#include <boost/atomic.hpp>
#include <boost/function.hpp>

#include <angelica/container/msque.h>

namespace angelica {
namespace active {

class active_server;

class mirco_active {
private:
	typedef boost::function<void(void) > event_handle;

public:
	mirco_active();
	~mirco_active();

	void post(event_handle _handle, active_server * _active_server);
	bool do_one();
	void run();

private:
	friend class active_server;

	boost::atomic_bool _run_flag;
	angelica::container::msque<event_handle > event_que;

};

} /* namespace active */
} /* namespace angelica */
#endif /* MIRCO_ACTIVE_H_ */
