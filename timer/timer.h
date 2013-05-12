/*
 * timer.h
 *  Created on: 2013-5-12
 *      Author: qianqians
 * timer
 */
#ifndef _TIMER_H
#define _TIMER_H

#include <ctime>
#include <map>

#include <boost/bimap.hpp>
#include <boost/thread/tss.hpp>
#include <boost/function.hpp>

namespace angelica{
namespace timer{

typedef boost::function<void() > timerHandle;

class timer{
private:
	typedef std::multimap<time_t, std::map<std::string, timerHandle> > timermap;
	typedef std::map<std::string, time_t> indexmap;

public:
	timer();
	~timer();

	void add_timer(std::string & key, time_t time, timerHandle handle);

	void del_timer(std::string & key);

	void do_timer();

private:
	time_t nowtime();

private:
	time_t datumtime;
	boost::thread_specific_ptr<indexmap > tss_index;
	boost::thread_specific_ptr<timermap > tss_timer_map;

};

}// timer
}// angelica

#endif //_TIMER_H