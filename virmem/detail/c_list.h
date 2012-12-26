/*
 * tools.h
 * Created on: 2012-10-16
 *     Author: qianqians
 * c_list
 */
#ifndef _C_LIST_H
#define _C_LIST_H

#include <angelica/detail/tools.h>

typedef struct list_node{
	struct list_node * _next;
}list_head;

#ifdef __cplusplus
extern "C"{
#endif //__cplusplus

//将_s插入到_list中_d结点之前, if _d == null insert to list end
bool list_insert(list_head * _list, struct list_node *_d, struct list_node *_s);

//将_list中的_d结点删除
bool list_erase(list_head * _list, struct list_node *_d);

//判断_list是否为空
bool list_empty(list_head * _list);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_C_LIST_H