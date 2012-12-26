/*
 * mirco_mempage_heap.c
 *  Created on: 2012-12-21
 *      Author: qianqians
 * mirco_mempage_heap 
 */
#include "chunk.h"
#include "mirco_mempage_heap.h"
#include "mempage_heap.h"

static size_t _64K = 1024*64;

bool flag(mirco_mempage_heap * _heap){
	return _heap->_flag.test_and_set();
}

void clear(mirco_mempage_heap * _heap){
	_heap->_flag.clear();
}

void * _mirco_mempage_heap_alloc(struct mirco_mempage_heap * _heap, size_t size){
	void * ret = 0;
	for(int i = 0; i < 8; i++){
		if (_heap->chunk[i] == 0){
			_heap->chunk[i] = _create_chunk(_64K);		
		}

		ret = _malloc(_heap->chunk[i], size);
		if (ret != 0){
			break;
		}
	}

	if (ret == 0){
		_recover_chunk(_heap->_father_heap, _heap->chunk[0]);
		_heap->chunk[0] = _create_chunk(_64K);

		ret = _malloc(_heap->chunk[0], size);
	}

	return ret;
}