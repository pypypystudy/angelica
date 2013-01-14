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
	struct mempage_heap * _heap = 0;

	if (_chunk != 0){
		_heap = (struct mempage_heap *)_malloc(_chunk, sizeof(struct mempage_heap));
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
			_heap->_heap[i].chunk = _create_chunk(_heap, chunk_size*4);
			_heap->_heap[i].chunk_small = _create_chunk(_heap, chunk_size);
			_heap->_heap[i]._flag.clear();
		}

		_heap->_free = (boost::atomic<struct chunk *>*)_malloc(_chunk, sizeof(boost::atomic<struct chunk *>)*1024);
		::new (&_heap->_free_mu) boost::shared_mutex();
		_heap->_free_slide.store(0);
		_heap->_alloc_slide.store(0);
		_heap->_free_max = 1024;

		_heap->_bigfree = (struct chunk **)_malloc(_chunk, sizeof(struct chunk *)*1024);
		::new (&_heap->_bigfree_flag) boost::atomic_flag();
		_heap->_bigfree_slide = 0;
		_heap->_bigfree_max = 1024;

		_heap->_tmpchunk = (struct chunk **)_malloc(_heap->_chunk, sizeof(struct chunk *)*1024);
	}

	return _heap;
}

void * _mempage_heap_alloc(struct mempage_heap * _heap, size_t size){
	void * ret = 0;
	unsigned int slide = 0;

	for(int i = 0; i < _heap->concurrent_count; i++){
		if (!flag(&_heap->_heap[slide])){
			if((ret = _mirco_mempage_heap_alloc(&_heap->_heap[slide], size)) != 0){
				clear(&_heap->_heap[slide]);
				break;
			}
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
		if (tmp != 0){
			memcpy(tmp, mem, oldsize);
			_free(mem);
		}
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
			_chunk = _chunk_from_bigfree(_heap, chunk_size);
		}
	}

	if (_chunk == 0){
		_chunk = _create_chunk(_heap, size);
	}

	if (_chunk != 0){
		_chunk->count.store(0);
		_chunk->rec_flag = 0;
		_chunk->slide = sizeof(struct chunk);
	}

	return _chunk;
}

struct chunk * _chunk_from_free(struct mempage_heap * _heap){
	boost::shared_lock<boost::shared_mutex> lock(_heap->_free_mu);

	struct chunk * _chunk = 0;
	unsigned int slide = _heap->_alloc_slide.load();
	unsigned int newslide = 0;
	while(1){
		if (slide == _heap->_free_slide.load()){
			break;
		}

		newslide = slide+1;
		if (newslide == _heap->_free_max){
			newslide = 0;
		}

		if (_heap->_alloc_slide.compare_exchange_strong(slide, newslide)){
			while((_chunk = _heap->_free[newslide].exchange(0)) == 0);
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

	if (_heap->_free_slide.load() == _heap->_alloc_slide.load()){
		boost::unique_lock<boost::shared_mutex> uniquelock(_heap->_free_mu);
		while(_heap->_bigfree_flag.test_and_set());
		_merge(_heap);
		_heap->_bigfree_flag.clear();
	}
}

void _recover_to_free(struct mempage_heap * _heap, struct chunk * _chunk){
	boost::upgrade_lock<boost::shared_mutex> lock(_heap->_free_mu);

	unsigned int slide = _heap->_free_slide.load();
	unsigned int newslide = 0;
	while(1){
		newslide = slide+1; 
		if (newslide == _heap->_free_max){
			newslide = 0;
		}

		if (newslide == _heap->_alloc_slide.load()){
			boost::unique_lock<boost::shared_mutex> uniquelock(boost::move(lock));
			slide = _heap->_free_slide.load();
			if (newslide == _heap->_free_max){
				newslide = 0;
			}
			if (newslide == _heap->_alloc_slide.load()){
				_resize_freelist(_heap);
			}
		}

		if (_heap->_free_slide.compare_exchange_strong(slide, newslide)){
			_heap->_free[slide].store(_chunk);
			break;
		}
	}
}

void _recover_to_bigfree(struct mempage_heap * _heap, struct chunk * _chunk){
	size_t size = _chunk->size;
	
	if (_heap->_bigfree_slide >= _heap->_bigfree_max){
		_merge_bigfree(_heap);
	}

	if (_heap->_bigfree_slide >= _heap->_bigfree_max){
		_resize_bigfreelist(_heap);
	}

	int slide = _bisearch(_heap, size);
	if (slide == -1){
		slide = _heap->_bigfree_slide;
	}
	
	for(int i = _heap->_bigfree_slide; i > slide; i--){
		_heap->_bigfree[i]= _heap->_bigfree[i-1];
	}
	_heap->_bigfree[slide] = _chunk;
	_heap->_bigfree_slide++;
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
	_heap->_bigfree_slide--;
	for(unsigned int i = slide; i < _heap->_bigfree_slide; i++){
		_heap->_bigfree[i] = _heap->_bigfree[i+1];
	}
}

void _merge(struct mempage_heap * _heap){
	struct chunk * _ret = 0, * _tmp = 0;

	for(unsigned int i = _heap->_alloc_slide; i < _heap->_free_slide && _heap->_free_slide > 0; ){
		_tmp = _heap->_free[i];
		for(unsigned int j = 0; j < _heap->_bigfree_slide; ){
			_ret = _merge_chunk(_tmp, _heap->_bigfree[j]);
			if (_ret != 0){
				_erase(_heap, j);
				_recover_to_bigfree(_heap, _ret);
				break;
			}else{
				j++;
			}
		}

		if (_ret == 0){
			for(unsigned int j = i+1; j < _heap->_free_slide && _heap->_free_slide > 0; ){
				_ret = _merge_chunk(_tmp, _heap->_free[j]);
				if (_ret != 0){
					_heap->_free[j].store(_heap->_free[--_heap->_free_slide].exchange(0));
					_recover_to_bigfree(_heap, _ret);
					break;
				}else{
					j++;
				}
			}
		}

		if (_ret != 0){
			_heap->_free[i].store(_heap->_free[--_heap->_free_slide].exchange(0));
		}else{
			i++;
		}

		if (_heap->_free_slide < _heap->_free_max/2){
			break;
		}
	}
}	

void _merge_bigfree(struct mempage_heap * _heap){
	if (_heap->_bigfree_slide > _heap->concurrent_count){
		struct chunk * _ret = 0, * _tmp = 0;
		unsigned int _tmpslide = 0;
		unsigned int _old_slide = _heap->_bigfree_slide;

		_heap->_tmpchunk[_tmpslide++] = _heap->_bigfree[--_heap->_bigfree_slide]; 

		for(unsigned int i = 0; i < _heap->_bigfree_slide; ){
			_tmp = _heap->_bigfree[i];

			for(unsigned int j = 0; j < _tmpslide; j++){
				_ret = _merge_chunk(_tmp, _heap->_tmpchunk[j]);
				if (_ret != 0){
					_heap->_tmpchunk[j] = _ret;
					break;
				}
			}

			_heap->_bigfree[i] = _heap->_bigfree[--_heap->_bigfree_slide];
			if (_ret == 0){
				if (_tmpslide < 1024){
					_heap->_tmpchunk[_tmpslide++] = _tmp;
				}
			}
		}
		for(unsigned int n = 0; n < _tmpslide; n++){
			_recover_to_bigfree(_heap, _heap->_tmpchunk[n]);
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
		_heap->_bigfree = _tmp;
	}
}

void _resize_freelist(struct mempage_heap * _heap){
	size_t size = 0, size1 = 0, size2 = 0;
	unsigned int slide = _heap->_free_slide.load();
	unsigned int allocslide = _heap->_alloc_slide.load();
	boost::atomic<struct chunk *>* _tmp = 0;

	if (slide == allocslide){
		size = _heap->_free_max*sizeof(struct chunk *)*2;
		_heap->_free_max *= 2;
		_tmp = (boost::atomic<struct chunk *>*)_malloc(_heap->_chunk, size);
		if (_tmp == 0){
			size_t tmpsize = (size + chunk_size - 1)/chunk_size*chunk_size;
			_heap->_chunk = _chunk(_heap, tmpsize);
			_tmp = (boost::atomic<struct chunk *>*)_malloc(_heap->_chunk, size);
		}
		size1 = slide*sizeof(struct chunk *);
		size2 = allocslide*sizeof(struct chunk *);
		memcpy(_tmp, _heap->_free, size1);
		memcpy((char*)_tmp+size-size2, (char*)_heap->_free+size1, size2); 
		_free(_heap->_free);
		_heap->_free = _tmp;
	}
}