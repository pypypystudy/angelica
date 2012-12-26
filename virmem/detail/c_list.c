/*
 * c_list.c
 * Created on: 2012-11-5
 *     Author: qianqians
 * c_list
 */
#include "c_list.h"

//将_s插入到_list中_d结点之前, if _d == null insert to list end
bool list_insert(list_head * _list, struct list_node *_d, struct list_node *_s){
	struct list_node *_d1 = (struct list_node*)_list;
	for( ; _d1->_next != _d; _d1 = _d1->_next);

	if(_d1->_next != _d)
		return false;

	_d1->_next = _s;
	_s->_next = _d;

	return true;
}

//将_list中的_d结点删除
bool list_erase(list_head * _list, struct list_node *_d){
	struct list_node *_d1 = (struct list_node*)_list;
	for( ; _d1 != _d; _d1 = _d1->_next);
	
	if(_d1 == 0 || _d1->_next != _d)
		return false;

	_d1->_next = _d->_next;

	return true;
}

//判断_list是否为空
bool list_empty(list_head * _list){
	if (((struct list_node*)_list)->_next == 0)
		return true;

	return false;
}
