/*
 * virmem.h
 * Created on: 2012-10-24
 *	   Author: qianqians
 * 虚拟内存 
 */
#ifndef _VIRMEM_H
#define _VIRMEM_H

#ifdef _WIN32
#include <Windows.h>
#endif

#include <angelica/detail/tools.h>

struct vmhandle {
	unsigned int cache_count;
	unsigned int cache_clock;
	void * mem;
	unsigned int size;

#ifdef _WIN32
	HANDLE hFileMapping;
#endif
	unsigned int startaddr;
	
	unsigned int offset;
};

#ifdef __cplusplus
extern "C"{
#endif //__cplusplus

//初始化
void vminit();

//分配一块虚拟内存
struct vmhandle * vmalloc(unsigned int size);

//访问虚拟内存 必须通过此函数访问 否则不能准确记录访问信息
void * mem_access(struct vmhandle * handle);

//换入页
bool swap_in(struct vmhandle * handle);
//换出页
bool swap_out(struct vmhandle * handle);

//释放虚拟内存
void vfree(struct vmhandle * handle);

//析构
void vmdestructor();

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_VIRMEM_H