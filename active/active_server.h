/*
 * active_server.h
 *
 *  Created on: 2012-4-8
 *      Author: qianqians
 */

#ifndef ACTIVE_SERVER_H_
#define ACTIVE_SERVER_H_

#include <boost/thread.hpp>
#include <boost/atomic.hpp>

#include <angelica/container/no_blocking_pool.h>

namespace angelica {
namespace active {

class mirco_active;

class active_server {
public:
	active_server();
	~active_server();

	bool do_one();

	void start(unsigned int  nCurrentNum = 0);
	void stop();

private:
	void run();

private:
	void post(mirco_active * _mirco_active);

	mirco_active * get_mirco_active();
	void release_mirco_active(mirco_active * _mirco_active);

private:
	friend class mirco_active;
	friend class active;

	boost::condition_variable _cond;
	boost::mutex _mu;
	boost::atomic_bool _empty_flag;

	boost::atomic_bool _run_flag;

	unsigned int current_num;
	boost::thread_group th_group;

	angelica::container::no_blocking_pool<mirco_active> _active_pool, _free_active_pool;

};

} /* namespace active */
} /* namespace angelica */
#endif /* ACTIVESERVER_H_ */
