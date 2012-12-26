/*
 * virmem.cpp
 * Created on: 2012-10-24
 *	   Author: qianqians
 * ĞéÄâÄÚ´æ 
 */
#include <time.h>

#include "virmem.h"
#include "detail/c_list.h"

struct MappingFileNode{
#ifdef _WIN32
	HANDLE hFileMapping;
#endif

	unsigned int startaddr;
	unsigned int size;

	struct list_node node;
};
typedef list_head MappingFileList;

struct FreeVirAddrNode{
	void * addr;
	unsigned int offset;

	struct list_node node;
};
typedef list_head FreeVirAddrList;

MappingFileList * _pMappingFileList = 0;
FreeVirAddrList * _pFreeVirAddrList = 0;

void GetFileMap(HANDLE *hFileMapping, unsigned int * startaddr, unsigned int offset){
	struct list_node * _d = 0;
	struct MappingFileNode * _fnode = 0;

	for(_d = _pMappingFileList->_next; _d != 0; _d = _d->_next){
		_fnode = container_of(_d, struct MappingFileNode, node);

		if(_fnode->startaddr + offset <= _fnode->size){
			break;
		}else{
			_fnode = 0;
		}
	}

	if (_fnode == 0){
		_fnode = (struct MappingFileNode *)malloc(sizeof(struct MappingFileNode));

		_fnode->startaddr = 0;
		_fnode->size = 0x40000000;
		_fnode->hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,  PAGE_READWRITE, 0, 0x40000000, 0);

		_fnode->node._next = 0;

		list_insert(_pMappingFileList, 0, &_fnode->node);
	}

	*hFileMapping = _fnode->hFileMapping;
	*startaddr = _fnode->startaddr;
	_fnode->startaddr += offset;

	if(_fnode->startaddr == _fnode->size){
		list_erase(_pMappingFileList, &_fnode->node);
		free(_fnode);
	}
}

void GetVirMem(void ** addr, unsigned int * offset){
	struct list_node * _d = 0;
	struct FreeVirAddrNode * _addrnode = 0;

	*addr = 0;

	for(_d = _pFreeVirAddrList->_next; _d != 0; _d = _d->_next){
		_addrnode = container_of(_d, struct FreeVirAddrNode, node);

		if(_addrnode->offset >= *offset){
			break;
		}else{
			_addrnode = 0;
		}
	}

	if (_addrnode != 0){
		*addr = _addrnode->addr;
		*offset = _addrnode->offset;

		list_erase(_pFreeVirAddrList, &_addrnode->node);
		free(_addrnode);
	}
}

void vminit(){
	if (_pMappingFileList == 0){
		_pMappingFileList = (list_head *)malloc(sizeof(list_head));
		_pMappingFileList->_next = 0;
	}

	if (_pFreeVirAddrList == 0){
		_pFreeVirAddrList = (list_head *)malloc(sizeof(list_head));
		_pFreeVirAddrList->_next = 0;
	}
}

struct vmhandle * vmalloc(unsigned int size){
	struct list_node * _d = 0;
	struct MappingFileNode * _fnode = 0;
	struct FreeVirAddrNode * _addrnode = 0;
	struct vmhandle * _ret = 0;

	HANDLE hFileMapping;
	unsigned int startaddr = 0;

	void * addr = 0;
	unsigned int offset = 0;

	void * addr_ret = 0;

	static long flag = 0;
	while(InterlockedExchange(&flag, 1) != 0);

	size = (size + 4095)/4096*4096;
	GetFileMap(&hFileMapping, &startaddr, size);

	offset = size;
	GetVirMem(&addr, &offset);
	if (addr != 0){
		VirtualFree(addr, 0, MEM_RELEASE);
	}

	addr_ret = MapViewOfFileEx(hFileMapping, FILE_MAP_WRITE | FILE_MAP_READ, 0, startaddr, size, addr);
	if (addr_ret == 0){
		DWORD error_code = GetLastError();

		_fnode = (struct MappingFileNode *)malloc(sizeof(struct MappingFileNode));

		_fnode->hFileMapping = hFileMapping;
		_fnode->startaddr = startaddr;
		_fnode->size = size;

		_fnode->node._next = 0;

		list_insert(_pMappingFileList, 0, &_fnode->node);

		goto end;
	}

	_ret = (struct vmhandle *)malloc(sizeof(struct vmhandle));
	_ret->cache_count = 1;
	_ret->cache_clock = clock();
	_ret->hFileMapping = hFileMapping;
	_ret->mem = addr_ret;
	_ret->offset = offset;
	_ret->size = size;
	_ret->startaddr = startaddr;

end:
	InterlockedExchange(&flag, 0);

	return _ret;
}	

void * mem_access(struct vmhandle * handle){
	handle->cache_count++;
	handle->cache_count = clock();

	return handle->mem;
}

bool swap_in(struct vmhandle * handle){
	struct FreeVirAddrNode * _addrnode = 0;

	void * addr = 0;
	unsigned int offset = handle->size;
	
	bool ret = true;

	static long flag = 0;

	if(handle == 0){
		abort();
	}

	if(handle->mem != 0){
		abort();
	}

	while(InterlockedExchange(&flag, 1) != 0);

	GetVirMem(&addr, &offset);
	if (addr != 0){
		VirtualFree(addr, 0, MEM_RELEASE);
	}
	handle->mem = MapViewOfFileEx(handle->hFileMapping, FILE_MAP_WRITE, 0, handle->startaddr, offset, addr);
	if (handle->mem == 0){
		DWORD error_code = GetLastError();

		_addrnode = (struct FreeVirAddrNode *)malloc(sizeof(struct FreeVirAddrNode));

		_addrnode->addr = addr;
		_addrnode->offset = offset;
		
		_addrnode->node._next = 0;

		list_insert(_pFreeVirAddrList, 0, &_addrnode->node);

		ret = false;
		goto end;
	}

end:
	InterlockedExchange(&flag, 0);
	return ret;
}

bool swap_out(struct vmhandle * handle){
	struct FreeVirAddrNode * _addrnode = 0;
	bool ret = true;
	void * addr = 0;

	static long flag = 0;

	if(handle == 0){
		abort();
	}

	if (handle->mem == 0){
		abort();
	}

	while(InterlockedExchange(&flag, 1) != 0);

	if (!FlushViewOfFile(handle->mem, handle->size)){
		ret = false;
		goto end;
	}

	if (!UnmapViewOfFile(handle->mem)){
		ret = false;
		goto end;
	}

	addr = VirtualAlloc(handle->mem, handle->offset, MEM_RESERVE, PAGE_READWRITE);
	if (addr != 0){
		_addrnode = (struct FreeVirAddrNode *)malloc(sizeof(struct FreeVirAddrNode));

		_addrnode->addr = addr;
		_addrnode->offset = handle->offset;
		
		_addrnode->node._next = 0;

		list_insert(_pFreeVirAddrList, 0, &_addrnode->node);

		handle->mem = 0;
	}

end:
	InterlockedExchange(&flag, 0);

	return ret;
}

void vfree(struct vmhandle * handle){
	struct MappingFileNode * _fnode = 0;

	static long flag = 0;

	if (handle == 0){
		abort();
	}

	while(InterlockedExchange(&flag, 1) != 0);

	if (handle->mem != 0){
		swap_out(handle);
	}

	_fnode = (struct MappingFileNode *)malloc(sizeof(struct MappingFileNode));
	_fnode->hFileMapping = handle->hFileMapping;
	_fnode->startaddr = handle->startaddr;
	_fnode->size = handle->size;

	InterlockedExchange(&flag, 0);
}

void vmdestructor(){
	struct list_node * _d = 0;
	struct FreeVirAddrNode * _addrnode = 0;
	struct MappingFileNode * _fnode = 0;
	HANDLE hFileMapping = 0;

	for(_d = _pFreeVirAddrList->_next; _d != 0; _d = _d->_next){
		_addrnode = container_of(_d, struct FreeVirAddrNode, node);

		VirtualFree(_addrnode->addr, 0, MEM_RELEASE);
	}

	for(_d = _pMappingFileList->_next; _d != 0; _d = _d->_next){
		_fnode = container_of(_d, struct MappingFileNode, node);

		if (_fnode->hFileMapping != hFileMapping){
			hFileMapping = _fnode->hFileMapping;

			CloseHandle(_fnode->hFileMapping);
		}
	}
}