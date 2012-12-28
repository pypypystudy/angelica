/*
 * angmalloc.cpp
 *  Created on: 2012-12-26
 *      Author: qianqians
 * angmalloc 
 */
#include "mempage_heap.h"
#include "mirco_mempage_heap.h"
#include "chunk.h"

static mempage_heap * _heap = _create_mempage_heap();

void * angmalloc(size_t size){
	return _mempage_heap_alloc(_heap, size);
}

void * angcalloc(size_t count, size_t size){
	void * ret = _mempage_heap_alloc(_heap, size);
	memset(ret, 0, size);
	return ret;
}

void * angrealloc(void * mem, size_t size){
	return _mempage_heap_realloc(_heap, mem, size);
}

void angfree(void * mem){
	_free(mem);
}