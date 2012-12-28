/*
 * mempage_heap.cpp
 *  Created on: 2012-12-21
 *      Author: qianqians
 * mempage_heap 
 */
#include "mempage_heap.h"
#include "mirco_mempage_heap.h"
#include "chunk.h"
#include <boost/thread/locks.hpp>
#ifdef _WIN32
#include <Windows.h>
#endif //_WIN32

#ifdef _WIN32
struct mempage_heap * _create_mempage_heap_win32(){
	size_t size = (sizeof(mempage_heap) + 4095)/4096*4096;
	struct mempage_heap * _heap = (struct mempage_heap*)VirtualAlloc(0, size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	_heap->concurrent_count = info.dwNumberOfProcessors;
	size_t size_mirco_heap = (sizeof(mirco_mempage_heap)*_heap->concurrent_count + 4095)/4096*4096;
	_heap->_heap = (struct mirco_mempage_heap*)VirtualAlloc(0, size_mirco_heap, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	for(unsigned int i = 0; i < _heap->concurrent_count; i++){
		_heap->_heap[i]._father_heap = _heap;
		for(int j = 0; j < 8; j++){
			_heap->_heap[i].chunk[i] = 0;
		}
		_heap->_heap[i]._flag.clear();
	}

	_heap->_recover = (struct chunk **)VirtualAlloc(0, 4096, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	::new (&_heap->_recover_mu) boost::shared_mutex();  
	_heap->_recover_slide.store(0);
	_heap->_recover_max = 4096/sizeof(struct chunk *);
	
	_heap->_old_recover = (struct chunk **)VirtualAlloc(0, 4096, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	::new (&_heap->_old_recover_mu) boost::shared_mutex();
	_heap->_old_recover_slide.store(0);
	_heap->_old_recover_max = 4096/sizeof(struct chunk *);

	_heap->_free = (struct chunk **)VirtualAlloc(0, 4096, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	::new (&_heap->_free_mu) boost::shared_mutex();
	_heap->_free_slide.store(0);
	_heap->_free_max = 4096/sizeof(struct chunk *);

	return _heap;
}
#endif //_WIN32

struct mempage_heap * _create_mempage_heap(){
#ifdef _WIN32
	return _create_mempage_heap_win32();
#endif //_WIN32
}

void * _mempage_heap_alloc(struct mempage_heap * _heap, size_t size){
	void * ret = 0;

	while(1){
		unsigned int slide = 0;

		if (!flag(&_heap->_heap[slide])){
			ret = _mirco_mempage_heap_alloc(&_heap->_heap[slide], size);
			clear(&_heap->_heap[slide]);
			break;
		}

		if (++slide >= _heap->concurrent_count){
			slide = 0;
		}
	}

	return ret;
}

void * _mempage_heap_realloc(struct mempage_heap * _heap, void * mem, size_t size){
	if (size == 0){
		_free(mem);
		return 0;
	}

	size_t oldsize = *(size_t *)((char*)mem - sizeof(size_t));
	if (oldsize > size){
		return mem;
	}
	
	void * tmp = _realloc(mem, size);
	if (tmp == 0){
		tmp = _mempage_heap_alloc(_heap, size);
		memcpy(tmp, mem, oldsize);
		_free(mem);
	}

	return tmp;
}

struct chunk * _chunk(struct mempage_heap * _heap, size_t size){
	boost::shared_lock<boost::shared_mutex> lock(_heap->_free_mu);

	struct chunk * _chunk = 0;
	unsigned int slide = _heap->_free_slide.load();
	while(1){
		if (slide <= 0){
			break;
		}

		unsigned int newslide = slide - 1;
		if (_heap->_free_slide.compare_exchange_strong(slide, newslide)){
			_chunk = _heap->_free[newslide];
			break;
		}
	}

	if (_chunk == 0){
		_chunk = _create_chunk(size);
	}

	return _chunk;
}

void _recover_chunk(struct mempage_heap * _heap, struct chunk * _chunk){
	boost::upgrade_lock<boost::shared_mutex> lock(_heap->_recover_mu);

	unsigned int slide = _heap->_recover_slide++;
	if (slide >= _heap->_recover_max){
		boost::unique_lock<boost::shared_mutex> uniquelock(boost::move(lock));

		unsigned int newslide = _heap->_recover_slide.load();
		if (newslide >= _heap->_recover_max){
			_resize_recvlist(_heap);
		}
	} 
	_heap->_recover[slide] = _chunk;

	slide = _heap->_recover_slide.load();
	if (slide >= _heap->_recover_max){
		_sweep(_heap);
	}
}

void _sweep(struct mempage_heap * _heap){
	boost::upgrade_lock<boost::shared_mutex> lock(_heap->_free_mu);
	boost::upgrade_lock<boost::shared_mutex> uplock(_heap->_old_recover_mu);
	
	for(unsigned int i = 0; i < _heap->_recover_slide; ){
		if (_isfree(_heap->_recover[i])){
			unsigned int slide = _heap->_free_slide++;
			if (slide >= _heap->_free_max){
				boost::unique_lock<boost::shared_mutex> uniquelock(boost::move(lock));
				_resize_freelist(_heap);
			}
			_heap->_free[slide] = _heap->_recover[i];
			_heap->_free[slide]->slide = sizeof(struct chunk);
			_heap->_free[slide]->rec_count = 0;
			
			_heap->_recover[i] = _heap->_recover[--_heap->_recover_slide];

		}else{
			if (_isoldchunk(_heap->_recover[i])){
				unsigned int slide = _heap->_old_recover_slide++;
				if (slide >= _heap->_old_recover_max){
					boost::unique_lock<boost::shared_mutex> uniquelock(boost::move(uplock));
					_resize_oldrecvlist(_heap);
				}
				_heap->_old_recover[slide] = _heap->_recover[i];

				_heap->_recover[i] = _heap->_recover[--_heap->_recover_slide];

			}else{
				i++;
			}
		}
	}

	if (_heap->_old_recover_slide.load() > 1024){
		boost::unique_lock<boost::shared_mutex> uniquelock(boost::move(uplock));
		for(unsigned int i = 0; i < _heap->_old_recover_slide; ){
			if (_isfree(_heap->_old_recover[i])){
				unsigned int slide = _heap->_free_slide++;
				if (slide >= _heap->_free_max){
					boost::unique_lock<boost::shared_mutex> uniquelock(boost::move(lock));
					_resize_freelist(_heap);
				}
				_heap->_free[slide] = _heap->_recover[i];

				_heap->_old_recover[i] = _heap->_old_recover[--_heap->_old_recover_slide];

			}else{
				i++;
			}
		}
	}
}

void _resize_recvlist(struct mempage_heap * _heap){
	unsigned int slide = _heap->_recover_slide.load();
	if (slide >= _heap->_recover_max){
		size_t size = _heap->_recover_max*sizeof(struct chunk *);
		_heap->_recover_max *= 2;
#ifdef _WIN32
		struct chunk ** _tmp = (struct chunk **)VirtualAlloc(0, size*2, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
		memcpy(_tmp, _heap->_recover, size);
		VirtualFree(_heap->_recover, 0, MEM_RELEASE);	
#endif
		_heap->_recover = _tmp;
	}
}

void _resize_freelist(struct mempage_heap * _heap){
	unsigned int slide = _heap->_free_slide.load();
	if (slide >= _heap->_free_max){
		size_t size = _heap->_free_max*sizeof(struct chunk *);
		_heap->_free_max *= 2;
	#ifdef _WIN32
		struct chunk ** _tmp = (struct chunk **)VirtualAlloc(0, size*2, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
		memcpy(_tmp, _heap->_free, size);
		VirtualFree(_heap->_free, 0, MEM_RELEASE);	
	#endif
		_heap->_free = _tmp;
	}
}

void _resize_oldrecvlist(struct mempage_heap * _heap){
	unsigned int slide = _heap->_old_recover_slide.load();
	if (slide >= _heap->_old_recover_max){
		size_t size = _heap->_old_recover_max*sizeof(struct chunk *);
		_heap->_old_recover_max *= 2;
	#ifdef _WIN32
		struct chunk ** _tmp = (struct chunk **)VirtualAlloc(0, size*2, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
		memcpy(_tmp, _heap->_old_recover, size);
		VirtualFree(_heap->_old_recover, 0, MEM_RELEASE);	
	#endif
		_heap->_old_recover = _tmp;
	};
}