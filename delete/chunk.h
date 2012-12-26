/*
 * chunk.h
 *  Created on: 2012-12-21
 *      Author: qianqians
 * chunk 
 */
#ifndef _CHUNK_H
#define _CHUNK_H

namespace angelica{
namespace pool{

struct chunk {
	unsigned int size;
	unsigned int slide;
	void * mem;
};

}//pool
}//angelica

#endif //_CHUNK_H