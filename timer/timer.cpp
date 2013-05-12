/*
 * timer.cpp
 *  Created on: 2013-5-12
 *      Author: qianqians
 * timer
 */
#include "timer.h"

namespace angelica{
namespace timer{

timer::timer(){
	time(&datumtime);
	datumtime *= 1000;
}
	
timer::~timer(){
}

time_t timer::nowtime(){
	return datumtime + clock();
}

void timer::add_timer(std::string & key, time_t time, timerHandle handle){
	timermap * ptimerhandle = tss_timer_map.get();
	if (ptimerhandle == 0){
		ptimerhandle = new timermap();
		tss_timer_map.reset(ptimerhandle);
	}

	time_t runtime = nowtime() + time;
	timermap::iterator iterfind = ptimerhandle->find(runtime);
	if (iterfind == ptimerhandle->end()){
		ptimerhandle->insert(std::make_pair(runtime, std::map<std::string, timerHandle>()));
		iterfind = ptimerhandle->find(runtime);
	}
	iterfind->second.insert(std::make_pair(key, handle));

	indexmap * pindexhandle = tss_index.get();
	if (pindexhandle == 0){
		pindexhandle = new indexmap;
		tss_index.reset(pindexhandle);
	}
	pindexhandle->insert(std::make_pair(key, runtime));
}

void timer::del_timer(std::string & key){
	indexmap * pindexhandle = tss_index.get();
	if (pindexhandle == 0){
		return;
	}
	
	indexmap::iterator iterfind = pindexhandle->find(key);
	if (iterfind == pindexhandle->end()){
		return;
	}
	time_t keytime = iterfind->second;
	pindexhandle->erase(iterfind);

	timermap * ptimerhandle = tss_timer_map.get();
	if (ptimerhandle == 0){
		return;
	}

	timermap::iterator _iterfind = ptimerhandle->find(keytime);
	if (_iterfind == ptimerhandle->end()){
		return;
	}
	
	std::map<std::string, timerHandle >::iterator _iterfind_ = _iterfind->second.find(key);
	if (_iterfind_ == _iterfind->second.end()){
		return;
	}
	_iterfind->second.erase(_iterfind_);
}

void timer::do_timer(){
	timermap * ptimerhandle = tss_timer_map.get();
	if (ptimerhandle == 0){
		return;
	}

	timermap::iterator iter = ptimerhandle->upper_bound(nowtime());
	if (ptimerhandle->end() == iter){
		return;
	}
	
	indexmap * pindexmap = tss_index.get();
	if (pindexmap == 0){
		return;
	}

	timermap::iterator run = ptimerhandle->begin(); 
	do{
		for(std::map<std::string, timerHandle>::iterator _iter = run->second.begin(); _iter != run->second.end(); _iter++){
			_iter->second();
			pindexmap->erase(_iter->first);
		}
		run = ptimerhandle->erase(run);
	}while(run != iter);

	for(std::map<std::string, timerHandle>::iterator _iter = iter->second.begin(); _iter != iter->second.end(); _iter++){
		_iter->second();
		pindexmap->erase(_iter->first);
	}
	ptimerhandle->erase(iter);
}

}// timer
}// angelica