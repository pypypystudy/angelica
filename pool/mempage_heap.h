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
	struct chunk * _chunk;

	boost::shared_mutex _free_mu;
	struct chunk ** _free;
	boost::atomic_uint _free_slide;
	unsigned int _free_max;

	boost::atomic_flag _bigfree_flag;
	struct chunk ** _bigfree;
	unsigned int _bigfree_slide;
	unsigned int _bigfree_max;

	struct chunk ** _tmpchunk;

};	
	
struct mempage_heap * _create_mempage_heap();
void * _mempage_heap_alloc(struct mempage_heap * _heap, size_t size);
void * _mempage_heap_realloc(struct mempage_heap * _heap, void * mem, size_t size);

void _recover_chunk(struct mempage_heap * _heap, struct chunk * _chunk);
struct chunk * _chunk(struct mempage_heap * _heap, size_t size);

#endif //_MEMPAGE_HEAP_H