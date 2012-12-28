/*
 * angmalloc.h
 *  Created on: 2012-12-26
 *      Author: qianqians
 * angmalloc 
 */
#ifndef _ANGMALLOC_H
#define _ANGMALLOC_H

#ifndef __cplusplus
extern "C"{
#endif //__cplusplus

void * angmalloc(size_t size);
void * angcalloc(size_t count, size_t size);
void * angrealloc(void * mem, size_t size);
void angfree(void * mem);

#ifndef __cplusplus
}
#endif __cplusplus

#endif //_ANGMALLOC_H