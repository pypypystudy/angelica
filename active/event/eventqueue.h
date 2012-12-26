/*
 * eventqueue.h
 *
 *  Created on: 2012-5-4
 *      Author: Administrator
 */

#ifndef EVENTQUEUE_H_
#define EVENTQUEUE_H_

#include "container/tslist.h"

namespace angelica {
namespace active {
namespace event {

template<typename T>
class event_queue {
public:
	event_queue() {};
	virtual ~event_queue() {};

	bool empty() {
		return __event_queue.empty();
	};

	void clear() {
		__event_queue.clear();
	};

	std::size_t size() {
		return __event_queue.size();
	};

	void push(T event) {
		__event_queue.push(event);
	};

	bool pop(T &event) {
		return __event_queue.pop(event);
	};

private:
	angelica::container::ts_list<T> __event_queue;

};

} /* namespace event */
} /* namespace active */
} /* namespace angelica */
#endif /* EVENTQUEUE_H_ */
