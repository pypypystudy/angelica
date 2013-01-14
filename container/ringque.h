/*
 * ringque.h
 *  Created on: 2012-1-13
 *      Author: qianqians
 * ringque
 */
#ifndef _RINGQUE_H
#define _RINGQUE_H

#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <boost/pool/pool_alloc.hpp>

namespace angelica{
namespace container{

template<typename T, typename _Allocator = boost::pool_allocator<T>>
class ringque{ 
public:
	ringque(){
		_que = (boost::atomic<typename T *>*)malloc(sizeof(boost::atomic<typename T *>)*1024);
		_push_slide.store(0);
		_que_max = 1024;
		_pop_slide.store(_que_max);
	}

	~ringque(){
		free(_que);
	}

	void push(T data){
		boost::upgrade_lock<boost::shared_mutex> lock(_mu);

		unsigned int slide = _push_slide.load();
		unsigned int newslide = 0;
		while(1){
			newslide = slide+1; 
			if (newslide == _que_max){
				newslide = 0;
			}

			if (newslide == _pop_slide.load()){
				boost::unique_lock<boost::shared_mutex> uniquelock(boost::move(lock));
				slide = _push_slide.load();
				if (newslide == _que_max){
					newslide = 0;
				}
				if (newslide == _pop_slide.load()){
					resize();
				}
			}

			if (_push_slide.compare_exchange_strong(slide, newslide)){		
				T * _tmp = _T_alloc.allocate(1);
				::new (_tmp) T(data);
				_que[slide].store(_tmp);
				break;
			}
		}
	}

	bool pop(T & data){
		boost::shared_lock<boost::shared_mutex> lock(_mu);

		T * _tmp = 0;
		unsigned int slide = _pop_slide.load();
		unsigned int newslide = 0;
		while(1){
			if (slide == _push_slide.load()){
				break;
			}

			newslide = slide+1;
			if (newslide == _que_max){
				newslide = 0;
			}

			if (_pop_slide.compare_exchange_strong(slide, newslide)){
				while((_tmp = _que[newslide].exchange(0)) == 0);
				data = *_tmp;
				_tmp->~T();
				_T_alloc.deallocate(_tmp, 1);
				return true;
			}
		}

		return false;
	}

private:
	void resize(){
		size_t size = 0, size1 = 0, size2 = 0;
		unsigned int pushslide = _push_slide.load();
		unsigned int popslide = _pop_slide.load();
		boost::atomic<typename T *> * _tmp = 0;
		unsigned int newslide = pushslide+1;

		if (newslide == popslide){
			size = _que_max*sizeof(boost::atomic<typename T *>)*2;
			while((_tmp = (boost::atomic<typename T *>*)malloc(size)) == 0);
			size1 = pushslide*sizeof(boost::atomic<typename T *>);
			size2 = _que_max*sizeof(boost::atomic<typename T *>)-size1;
			memcpy(_tmp, _que, size1);
			memcpy((char*)_tmp+size-size2, (char*)_que+size1, size2); 
			_pop_slide += _que_max;
			_que_max *= 2;
			free(_que);
			_que = _tmp;
		}
	}

private:
	boost::shared_mutex _mu;
	boost::atomic<typename T *> * _que;
	boost::atomic_uint _push_slide, _pop_slide;
	unsigned int _que_max;
	
	_Allocator _T_alloc;
};

}//container
}//angelica

#endif //_RINGQUE_H