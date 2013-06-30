/*
 * exception.h
 *
 *  Created on: May 22, 2013
 *      Author: qianqians
 * angelica exception
 */
#ifndef EXCEPTION_H_
#define EXCEPTION_H_

#include <stdexcept>
#include <string>
#include <sstream>

namespace angelica{
namespace exception{

class exception : public std::exception{
public:
	exception(const char * info){
		std::stringstream strstream;
		strstream << info << " file:" << __FILE__ << " line:" << __LINE__;
		err = strstream.str();
	}

	exception(const std::string & info){
		std::stringstream strstream;
		strstream << info << " file:" << __FILE__ << " line:" << __LINE__;
		err = strstream.str();
	}

	exception(const char * info, int errcode){
		std::stringstream strstream;
		strstream << info << errcode << " file:" << __FILE__ << " line:" << __LINE__;
		err = strstream.str();
	}

	exception(const std::string & info, int errcode){
		std::stringstream strstream;
		strstream << info << errcode << " file:" << __FILE__ << " line:" << __LINE__;
		err = strstream.str();
	}

	virtual ~exception(){}

	virtual const char *what( ) const{
		return err.c_str();
	}

private:
	exception(){}

private:
	std::string err;
};

}// exception
}// angelica

#endif /* EXCEPTION_H_ */
