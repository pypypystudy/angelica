/*
 * mempage_alloc.cpp
 *  Created on: 2012-12-21
 *      Author: qianqians
 * mempage_alloc 
 */
#include "mempage_alloc.h"
#include "mirco_mempage_alloc.h"
#include <Windows.h>
#include <xmemory>

namespace angelica{
namespace pool{
	
mempage_alloc::mempage_alloc(){
#ifdef _WIN32
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	count = info.dwNumberOfProcessors;
#endif //_WIN32
	
	unsigned int size = (sizeof(mirco_mempage_alloc)*count + 4095)/4096*4096;

#ifdef _WIN32
	_alloc = (mirco_mempage_alloc *)VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#endif //_WIN32

	for(unsigned int i = 0; i < count; i++){
		std::_Construct(&_alloc[i]);
	}
}

mempage_alloc::~mempage_alloc(){
	for(unsigned int i = 0; i < count; i++){
		std::_Destroy(&_alloc[i]);
	}

#ifdef _WIN32
	VirtualFree(_alloc, 0, MEM_RELEASE);
#endif //_WIN32
}

void * mempage_alloc::alloc(unsigned int size){
	void * ret = 0;
	for(unsigned int i = 0; i < count; i++){
		if(!_alloc[i].flag()){
			ret = _alloc[i].alloc(size);
			_alloc[i].clear();
			if (ret != 0)
			{	
				break;
			}
		}
	}

	if (ret == 0){
		size = (size + sizeof(unsigned int) + 4095)/4096*4096;
		unsigned int * _memsize = (unsigned int *)VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		 *_memsize = size;
		 ret = (void *)++_memsize;
	}

	return ret;
}

void mempage_alloc::free(void * mem)
{
	unsigned int slide = 0;
	while(1){
		if (!_alloc[slide].flag()){
			_alloc[slide].free(mem);
			_alloc[slide].clear();
			break;
		}

		if (slide == count){
			slide = 0;
		}
	}
}

}//pool
}//angelica