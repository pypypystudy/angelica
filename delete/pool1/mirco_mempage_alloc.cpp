/*
 * mirco_mempage_alloc.cpp
 *  Created on: 2012-12-21
 *      Author: qianqians
 * mirco_mempage_alloc 
 */
#include "mirco_mempage_alloc.h"
#include <Windows.h>

namespace angelica{
namespace pool{

mirco_mempage_alloc::mirco_mempage_alloc(){
	std::size_t size = (sizeof(std::size_t) + 4096 + 4095)/4096*4096;
#ifdef _WIN32
	std::size_t * size_ = (std::size_t *)VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	*size_ = size;
	page_order_vector = (void**)++size_;
#endif
	max_size = (size - sizeof(std::size_t))/sizeof(void*);
	count = 0;
}

mirco_mempage_alloc::~mirco_mempage_alloc(){
#ifdef _WIN32
	//VirtualFree(page_order_vector, 0, MEM_RELEASE);
#endif
}

void * mirco_mempage_alloc::alloc(std::size_t size){
	size = (size + sizeof(std::size_t) + 4095)/4096*4096;
	void * ret = find_page(size);

	if (ret == 0){
		std::size_t * _memsize = (std::size_t *)VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		 *_memsize = size;
		 ret = (void *)++_memsize;
	}

	return ret;
}

void mirco_mempage_alloc::free(void * page){
	page = (void*)((char *)page + sizeof(std::size_t));
	insert(page);
	sweep();
}

void * mirco_mempage_alloc::find_page(std::size_t size){
	void * ret = 0;
	std::size_t oldsize = size;
	int slide = bisearch(size);
	
	if (slide >= 0){
		ret = (void*)((char *)page_order_vector[slide] + sizeof(std::size_t));
		erase(slide);

		if (size >= (oldsize + 8192)){
			std::size_t * tmp = (std::size_t *)((char*)ret + oldsize);
			*tmp = size - oldsize;
			insert((void*)tmp);
		}
	}

	return ret;
}

void mirco_mempage_alloc::insert(void * page){
	std::size_t * size = (std::size_t*)page;
	int slide = bisearch(*size);

	if (count >= max_size){
		unsigned int _oldsize = max_size*sizeof(void*);
		max_size *= 2;
		unsigned int _size = max_size*sizeof(void*);
		void ** _tmp = (void**)VirtualAlloc(0, _size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		memcpy(_tmp, page_order_vector, _oldsize);
		std::swap(_tmp, page_order_vector);
		free(_tmp);
	}

	memcpy(&page_order_vector[slide+1], &page_order_vector[slide], sizeof(void**)*(count-slide));
	page_order_vector[slide] = (void*)size;
}

void mirco_mempage_alloc::erase(unsigned int slide){
	unsigned int size = (count - slide - 1)*sizeof(void*);
	memcpy(&page_order_vector[slide], &page_order_vector[slide + 1], size);
}

int mirco_mempage_alloc::bisearch(std::size_t & size){
	int ret = -1;
	int slide = 0;
	while(1){
		std::size_t size_ = *((std::size_t*)page_order_vector[slide]);
		if (size_ == size){
			ret = slide;
			size = size_;
			break;
		}else if (size_ > size){
			if (slide > 0){
				std::size_t size__ =  *((std::size_t*)page_order_vector[slide-1]);
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
			if (slide < (count - 1)){
				slide = (slide + count)/2;
			}else {
				break;
			}
		}
	}

	return ret;
}

void mirco_mempage_alloc::sweep(){
	
}

bool mirco_mempage_alloc::flag(){
	return _flag.test_and_set();
}

void mirco_mempage_alloc::clear(){
	_flag.clear();
}

}//pool
}//angelica