/*
 * mempage_alloc.h
 *  Created on: 2012-12-21
 *      Author: qianqians
 * mempage_alloc 
 */
#ifndef _MEMPAGE_ALLOC_H
#define _MEMPAGE_ALLOC_H

namespace angelica{
namespace pool{

class mirco_mempage_alloc;

class mempage_alloc{
public:
	mempage_alloc();
	~mempage_alloc();

public:
	void * alloc(unsigned int size);
	void free(void * mem);

private:	
	unsigned int count;
	mirco_mempage_alloc * _alloc;

};

}//pool
}//angelica

#endif //_MEMPAGE_ALLOC_H