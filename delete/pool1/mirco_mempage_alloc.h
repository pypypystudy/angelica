/*
 * mirco_mempage_alloc.h
 *  Created on: 2012-12-21
 *      Author: qianqians
 * mirco_mempage_alloc 
 */
#ifndef _MIRCO_MEMPAGE_ALLOC_H
#define _MIRCO_MEMPAGE_ALLOC_H

#include <boost/atomic.hpp>

namespace angelica{
namespace pool{

class mirco_mempage_alloc {
public:
	mirco_mempage_alloc();
	~mirco_mempage_alloc();

public:
	void * alloc(std::size_t size);
	void free(void * page);

private:
	void * find_page(std::size_t size);
	void insert(void * page);
	int bisearch (std::size_t & size);
	void erase(unsigned int slide);
	void sweep();

public:
	bool flag();
	void clear();

private:
	unsigned int count;
	unsigned int max_size;
	void ** page_order_vector;
	
	boost::atomic_flag _flag;

};

}//pool
}//angelica

#endif //_MIRCO_MEMPAGE_ALLOC_H