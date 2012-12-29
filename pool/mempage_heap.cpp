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

struct chunk * _chunk_from_free(struct mempage_heap * _heap);
struct chunk * _chunk_from_bigfree(struct mempage_heap * _heap, size_t size);
int _bisearch(struct mempage_heap * _heap, size_t size);	
void _erase(struct mempage_heap * _heap, unsigned int silde);
	
void _recover_to_free(struct mempage_heap * _heap, struct chunk * _chunk);
void _recover_to_bigfree(struct mempage_heap * _heap, struct chunk * _chunk);
void _merge(struct mempage_heap * _heap);
void _merge_bigfree(struct mempage_heap * _heap);
void _resize_freelist(struct mempage_heap * _heap);
void _resize_bigfreelist(struct mempage_heap * _heap);

struct mempage_heap * _create_mempage_heap(){
	struct chunk * _chunk = _create_chunk(0, chunk_size);
	struct mempage_heap * _heap = (struct mempage_heap *)_malloc(_chunk, sizeof(struct mempage_heap));
	_heap->_chunk = _chunk;
	_chunk->_heap = _heap;
	
#ifdef _WIN32
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	_heap->concurrent_count = info.dwNumberOfProcessors;
#endif //_WIN32
	_heap->_heap = (struct mirco_mempage_heap*)_malloc(_chunk, sizeof(mirco_mempage_heap)*_heap->concurrent_count);
	for(unsigned int i = 0; i < _heap->concurrent_count; i++){
		_heap->_heap[i]._father_heap = _heap;
		_heap->_heap[i].chunk = 0;
		_heap->_heap[i]._flag.clear();
	}

	_heap->_free = (struct chunk **)_malloc(_chunk, 4096);
	::new (&_heap->_free_mu) boost::shared_mutex();
	_heap->_free_slide.store(0);
	_heap->_free_max = 4096/sizeof(struct chunk *);

	_heap->_bigfree = (struct chunk **)_malloc(_chunk, 4096);
	::new (&_heap->_bigfree_flag) boost::atomic_flag();
	_heap->_bigfree_slide = 0;
	_heap->_bigfree_max = 4096/sizeof(struct chunk *);

	return _heap;
}

void * _mempage_heap_alloc(struct mempage_heap * _heap, size_t size){
	void * ret = 0;
	unsigned int slide = 0;

	while(1){
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
	struct chunk * _chunk = 0;

	if (size > chunk_size){
		_chunk = _chunk_from_bigfree(_heap, size);
	}else{
		_chunk = _chunk_from_free(_heap);
		if (_chunk == 0){
			_chunk = _chunk_from_bigfree(_heap, size);
		}
	}

	if (_chunk == 0){
		_chunk = _create_chunk(_heap, size);
	}
	_chunk->_heap = _heap;
	_chunk->rec_count = 0;
	_chunk->rec_flag = 0;
	_chunk->slide = sizeof(struct chunk);

	return _chunk;
}

struct chunk * _chunk_from_free(struct mempage_heap * _heap){
	boost::shared_lock<boost::shared_mutex> lock(_heap->_free_mu);

	struct chunk * _chunk = 0;
	unsigned int slide = _heap->_free_slide.load();
	unsigned int newslide = 0;
	while(1){
		if (slide <= 0){
			break;
		}

		newslide = slide - 1;
		if (_heap->_free_slide.compare_exchange_strong(slide, newslide)){
			_chunk = _heap->_free[newslide];
			break;
		}
	}

	return _chunk;
}

struct chunk * _chunk_from_bigfree(struct mempage_heap * _heap, size_t size){
	while(_heap->_bigfree_flag.test_and_set());
	
	struct chunk * _chunk = 0;
	int slide = _bisearch(_heap, size);
	unsigned int _oldslide = 0;

	if (slide >= 0){
		_chunk = _heap->_bigfree[slide];
		_erase(_heap, slide);
	}else{
		_oldslide = _heap->_bigfree_slide;
		if (_oldslide > _heap->concurrent_count){
			_merge_bigfree(_heap);
			if (_heap->_bigfree_slide < _oldslide){
				slide = _bisearch(_heap, size);
				if (slide >= 0){
					_chunk = _heap->_bigfree[slide];
					_erase(_heap, slide);
				}
			}
		}
	}

	_heap->_bigfree_flag.clear();

	return _chunk;
}

void _recover_chunk(struct mempage_heap * _heap, struct chunk * _chunk){
	if (_chunk->size > chunk_size){
		while(_heap->_bigfree_flag.test_and_set());
		_recover_to_bigfree(_heap, _chunk);
		_heap->_bigfree_flag.clear();
	}else{
		_recover_to_free(_heap, _chunk);
	}

	if (_heap->_free_slide.load() >= _heap->_free_max){
		boost::unique_lock<boost::shared_mutex> uniquelock(_heap->_free_mu);
		while(_heap->_bigfree_flag.test_and_set());
		_merge(_heap);
		_heap->_bigfree_flag.clear();
	}
}

void _recover_to_free(struct mempage_heap * _heap, struct chunk * _chunk){
	boost::upgrade_lock<boost::shared_mutex> lock(_heap->_free_mu);

	unsigned int slide = _heap->_free_slide++;
	if (slide >= _heap->_free_max){
		boost::unique_lock<boost::shared_mutex> uniquelock(boost::move(lock));

		if (_heap->_free_slide.load() >= _heap->_free_max){
			_resize_freelist(_heap);
		}
	} 
	_heap->_free[slide] = _chunk;
}

void _recover_to_bigfree(struct mempage_heap * _heap, struct chunk * _chunk){
	size_t size = _chunk->size;
	int slide = _bisearch(_heap, size);

	if (_heap->_bigfree_slide >= _heap->_bigfree_max){
		_merge_bigfree(_heap);
	}

	if (_heap->_bigfree_slide >= _heap->_bigfree_max){
		_resize_bigfreelist(_heap);
	}

	if (slide < 0){
		_heap->_bigfree[_heap->_bigfree_slide++] = _chunk;
	}else{
		for(unsigned int i = slide; i < _heap->_bigfree_slide; i++){
			_heap->_bigfree[i+1]= _heap->_bigfree[i];
		}
		_heap->_bigfree_slide++;
		_heap->_bigfree[slide] = _chunk;
	}
}

int _bisearch(struct mempage_heap * _heap, size_t size){
	int ret = -1;

	if (_heap->_bigfree_slide > 0){
		unsigned int slide = 0;
		std::size_t size_ = 0;
		std::size_t size__  = 0;
		while(1){
			size_ = _heap->_bigfree[slide]->size;
			if (size_ == size){
				ret = slide;
				size = size_;
				break;
			}else if (size_ > size){
				if (slide > 0){
					size__ = _heap->_bigfree[slide-1]->size;
					if (size__ > size){
						slide = slide/2;
					}else{
						ret = slide;
						size = size_;
						break;
					}
				}else{
					ret = slide;
					size = size_;
					break;
				}
			}else{
				if (slide < (_heap->_bigfree_slide - 1)){
					slide = (slide + _heap->_bigfree_slide)/2;
				}else {
					break;
				}
			}
		}
	}

	return ret;
}

void _erase(struct mempage_heap * _heap, unsigned int slide){
	if (slide > 0){
		for(unsigned int i = slide; i < _heap->_bigfree_slide - 1; i++){
			_heap->_bigfree[slide] = _heap->_bigfree[slide+1];
		}
	}
	_heap->_bigfree_slide--;
}

void _merge(struct mempage_heap * _heap){
	struct chunk * _ret = 0, * _tmp = 0, * _tmpret = 0;

	for(unsigned int i = 0; i < _heap->_free_slide; ){
		_ret = 0;
		_tmp = _heap->_free[i];
		for(unsigned int j = 0; j < _heap->_bigfree_slide; ){
			_tmpret = _merge_chunk(_tmp, _heap->_bigfree[j]);
			if (_tmpret != 0){
				_tmp = _ret = _tmpret;
				_erase(_heap, j);
			}else{
				j++;
			}
		}
		if (_ret != 0){
			_recover_to_bigfree(_heap, _ret);
		}

		if (_ret == 0){
			for(unsigned int j = i+1; j < _heap->_free_slide; ){
				_tmpret = _merge_chunk(_tmp, _heap->_free[j]);
				if (_tmp != 0){
					_ret = _tmp = _tmpret;
					_heap->_free[j] = _heap->_free[--_heap->_free_slide];
				}else{
					j++;
				}
			}
			if (_ret != 0){
				_recover_to_bigfree(_heap, _ret);
			}
		}

		if (_ret != 0){
			_heap->_free[i] = _heap->_free[--_heap->_free_slide];
		}else{
			i++;
		}

		if (_heap->_free_slide < _heap->_free_max/2){
			break;
		}
	}
}	

void _merge_bigfree(struct mempage_heap * _heap){
	struct chunk * _ret = 0, * _tmp = 0, * _tmpret = 0;

	for(unsigned int i = 0; i < _heap->_bigfree_slide; ){
		_ret = 0;
		_tmp = _heap->_bigfree[i];
		for(unsigned int j = i+1; j < _heap->_bigfree_slide; ){
			_tmpret = _merge_chunk(_tmp, _heap->_bigfree[j]);
			if (_tmpret != 0){
				_ret = _tmp = _tmpret;
				_erase(_heap, j);
			}else{
				j++;
			}
		}
		if (_ret != 0){
			_erase(_heap, i);
			_recover_to_bigfree(_heap, _ret);
		}else{
			i++;
		}
	
		if(_heap->_bigfree_slide <= _heap->concurrent_count){
			break;
		}
	}
}

void _resize_bigfreelist(struct mempage_heap * _heap){
	size_t size = 0;
	unsigned int slide = _heap->_bigfree_slide;
	struct chunk ** _tmp = 0;
	
	if (slide >= _heap->_bigfree_max){
		size = _heap->_bigfree_max*sizeof(struct chunk *);
		_heap->_bigfree_max *= 2;
		_tmp = (struct chunk **)_malloc(_heap->_chunk, size*2);
		if (_tmp == 0){
			size_t tmpsize = (size + chunk_size - 1)/chunk_size*chunk_size;
			_heap->_chunk = _chunk(_heap, tmpsize);
			_tmp = (struct chunk **)_malloc(_heap->_chunk, size*2);
		}
		memcpy(_tmp, _heap->_bigfree, size);
		_free(_heap->_bigfree);
	}
}

void _resize_freelist(struct mempage_heap * _heap){
	size_t size = 0;
	unsigned int slide = _heap->_free_slide.load();
	struct chunk ** _tmp = 0;

	if (slide >= _heap->_free_max){
		size = _heap->_free_max*sizeof(struct chunk *);
		_heap->_free_max *= 2;
		_tmp = (struct chunk **)_malloc(_heap->_chunk, size*2);
		if (_tmp == 0){
			size_t tmpsize = (size + chunk_size - 1)/chunk_size*chunk_size;
			_heap->_chunk = _chunk(_heap, tmpsize);
			_tmp = (struct chunk **)_malloc(_heap->_chunk, size*2);
		}
		memcpy(_tmp, _heap->_free, size);
		_free(_heap->_free);
	}
}