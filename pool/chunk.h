/*
 * chunk.h
 *  Created on: 2012-12-21
 *      Author: qianqians
 * chunk 
 */
#ifndef _CHUNK_H
#define _CHUNK_H

#include <boost/atomic.hpp>

static unsigned int _flag = 0x12345678;

struct chunk {
	unsigned int flag; 
	struct mempage_heap * _heap;
	size_t size;
	size_t slide;
	boost::atomic_char rec_flag; //0 action, 1 recv, 2 recved
	boost::atomic_uint count;
};

struct chunk * _create_chunk(struct mempage_heap * _heap, size_t size);
void * _brk(struct chunk * _chunk, size_t size);

struct chunk * _merge_chunk(struct chunk * _c1, struct chunk * _c2);

void * _malloc(struct chunk * _chunk, size_t size);
void * _realloc(void * mem, size_t size);
void _free(void * mem);

#endif //_CHUNK_H