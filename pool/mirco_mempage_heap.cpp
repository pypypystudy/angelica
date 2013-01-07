/*
 * mirco_mempage_heap.c
 *  Created on: 2012-12-21
 *      Author: qianqians
 * mirco_mempage_heap 
 */
#include "chunk.h"
#include "mirco_mempage_heap.h"
#include "mempage_heap.h"

size_t chunk_size = 1024*64;

bool flag(mirco_mempage_heap * _heap){
	return _heap->_flag.test_and_set();
}

void clear(mirco_mempage_heap * _heap){
	_heap->_flag.clear();
}

void * _mirco_mempage_heap_alloc(struct mirco_mempage_heap * _heap, size_t size){
	void * ret = 0;

	if (size > chunk_size - 4096){
		size_t _chunksize = (size + chunk_size*2 -1)/chunk_size*chunk_size;

		if (_heap->chunk != 0){
			_heap->chunk->rec_flag.store(1);
			if (_heap->chunk->count.load() == 0 && _heap->chunk->rec_flag.load() != 2){
				_recover_chunk(_heap->_father_heap, _heap->chunk);
			}
		}
		_heap->chunk = _chunk(_heap->_father_heap, _chunksize);
	}

	if (_heap->chunk == 0){
		_heap->chunk = _chunk(_heap->_father_heap, chunk_size);
	}

	ret = _malloc(_heap->chunk, size);
	if (ret == 0){	
		if (_heap->chunk->count.load() != 0){
			_heap->chunk->rec_flag.store(1);
			if (_heap->chunk->count.load() == 0){
				char flag = 1;
				if (_heap->chunk->rec_flag.compare_exchange_weak(flag, 2)){
					_heap->chunk ->slide = sizeof(struct chunk);
				}
			}
		}else{
			_heap->chunk->slide = sizeof(struct chunk);
		}
		ret = _malloc(_heap->chunk, size);
	}

	return ret;
}