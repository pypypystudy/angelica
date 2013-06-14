/*
 * strtools.h
 *  Created on: 2013-4-3
 *      Author: qianqians
 * strtools
 */
#ifndef _STRTOOLS_H
#define _STRTOOLS_H

#include <vector>
#include "u_stdint.h"

namespace angelica{
namespace strtools{

String str2utf8(String str);

String utf82str(String utf8);

String int2str(Integer num);

Integer str2int(String num_str);

String uint2str(UInteger num);

UInteger str2uint(String num_str);

String float2str(Float num);

Float str2float(String num_str);

void spitle(char ch, String str, std::vector<String> & vectorStr, int spitlecount = 0);

} //strtools
} //angelica

#endif //_STRTOOLS_H