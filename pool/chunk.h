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
	size_t size;
	size_t slide;
	boost::atomic_uint count;
};

struct chunk * _create_chunk(size_t size);
void * _brk(struct chunk * _chunk, size_t size);
bool _isfree(struct chunk * _chunk);

void * _malloc(struct chunk * _chunk, size_t size);
void * _realloc(void * mem, size_t size);
void _free(void * mem);

#endif //_CHUNK_H