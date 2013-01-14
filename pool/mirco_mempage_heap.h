/*
 * mirco_mempage_heap.h
 *  Created on: 2012-12-21
 *      Author: qianqians
 * mirco_mempage_heap
 */
#ifndef _MIRCO_MEMPAGE_HEAP_H
#define _MIRCO_MEMPAGE_HEAP_H

#include <boost/atomic.hpp>

struct chunk;
struct mempage_heap;

extern size_t chunk_size;

struct mirco_mempage_heap {
	struct mempage_heap * _father_heap;
	struct chunk * chunk, * chunk_small;
	boost::atomic_flag _flag;
};

bool flag(mirco_mempage_heap * _heap);
void clear(mirco_mempage_heap * _heap);

void * _mirco_mempage_heap_alloc(struct mirco_mempage_heap * _heap, size_t size);

#endif //_MIRCO_MEMPAGE_HEAP_H