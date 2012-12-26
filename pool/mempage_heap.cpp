/*
 * mempage_heap.cpp
 *  Created on: 2012-12-21
 *      Author: qianqians
 * mempage_heap 
 */
#include "mempage_heap.h"
#include "mirco_mempage_heap.h"
#include "chunk.h"
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

	_heap->_recover = (struct chunk *)VirtualAlloc(0, 4096, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	::new (&_heap->_recover_mu) boost::shared_mutex();  
	_heap->_recover_slide.store(0);
	_heap->_recover_max = 4096/sizeof(struct chunk *);
	
	_heap->_old_recover = (struct chunk *)VirtualAlloc(0, 4096, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	::new (&_heap->_old_recover_mu) boost::shared_mutex();
	_heap->_old_recover_slide.store(0);
	_heap->_old_recover_max = 4096/sizeof(struct chunk *);

	_heap->_free = (struct chunk *)VirtualAlloc(0, 4096, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
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








