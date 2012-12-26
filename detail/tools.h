/*
 * tools.h
 * Created on: 2012-10-16
 *     Author: qianqians
 * tools
 */
#ifndef _TOOLS_H
#define _TOOLS_H

#ifndef __cplusplus
#define bool char 
#define true 1
#define false 0
#endif

#ifndef container_of
// copy from include/linux/kernel.h
#define container_of(ptr, type, member) (type *)( (char *)ptr - offsetof(type, member) )
#endif //container_of

#ifndef offsetof
// copy from include/linux/stddef.h
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif //offsetof

#ifdef __cplusplus
extern "C"{
#endif //__cplusplus

void _trace(char * format, ...);

#ifdef __cplusplus
} //"C"
#endif //__cplusplus

#endif //_tools_h