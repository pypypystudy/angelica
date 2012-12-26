/*
 * mempage_heap.h
 *  Created on: 2012-12-21
 *      Author: qianqians
 * mempage_heap 
 */
#ifndef _MEMPAGE_HEAP_H
#define _MEMPAGE_HEAP_H

#include <boost/thread/shared_mutex.hpp>
#include <boost/atomic.hpp>

struct chunk;
struct mirco_mempage_heap;

struct mempage_heap{
	size_t concurrent_count;
	mirco_mempage_heap * _heap;

	boost::shared_mutex _recover_mu;
	struct chunk * _recover;
	boost::atomic_uint _recover_slide;
	unsigned int _recover_max;

	boost::shared_mutex _old_recover_mu;
	struct chunk * _old_recover;
	boost::atomic_uint _old_recover_slide;
	unsigned int _old_recover_max;

	boost::shared_mutex _free_mu;
	struct chunk * _free;
	boost::atomic_uint _free_slide;
	unsigned int _free_max;
};

struct mempage_heap * _create_mempage_heap();
void * _mempage_heap_alloc(struct mempage_heap * _heap, size_t size);
void * _mempage_heap_realloc(struct mempage_heap * _heap, void * mem, size_t size);

struct chunk * chunk(struct mempage_heap * _heap);
void _recover_chunk(struct mempage_heap * _heap, struct chunk * _chunk);
void sweep();

#endif //_MEMPAGE_HEAP_H