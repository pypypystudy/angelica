/*
 * random.cpp
 *
 *  Created on: 2012-2-9
 *      Author: Administrator
 */

#include "random.h"

#include <boost/random.hpp>

namespace angelica { namespace tools {

int rand_unsigned_int(int low, int height)
{
	static boost::mt19937 re;
	static boost::uniform_int<int> dist(low, height);

	return dist(re);
}
}}


