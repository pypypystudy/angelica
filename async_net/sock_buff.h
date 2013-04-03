/*
 * sock_buff.h
 *  Created on: 2012-5-11
 *      Author: qianqians
 * 2012-10-19 move to async_net
 * no-blocking buff 
 */
#ifndef _SOCK_BUFF_H
#define _SOCK_BUFF_H

#ifdef _WIN32
#include "win32/winhdef.h"
#endif

#include <angelica/container/msque.h>

#include <vector>
#include <cstddef>
#include <boost/atomic.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/pool/pool_alloc.hpp>

namespace angelica {
namespace async_net {
namespace detail {

class read_buff{
public:	
#ifdef _WIN32
	WSABUF _wsabuf;
#endif	

	read_buff();
	~read_buff();
	
	void init();

	char * buff;
	std::size_t buff_size;
	std::size_t slide;

};

class write_buff {
public:
	write_buff();
	~write_buff();
	
	void init();

private:
	struct buffex{
		buffex();
		~buffex();

		void clear();

		void put_buff();

		char * buff;
		std::size_t buff_size;
		boost::atomic_uint32_t slide;

		boost::shared_mutex _mu;
		
#ifdef _WIN32
		WSABUF * _wsabuf;
		unsigned int _wsabuf_count;
		boost::atomic_uint32_t _wsabuf_slide;
		boost::shared_mutex _wsabuf_mu;
#endif	//_WIN32
	};

private:
	boost::atomic<buffex * > write_buff_;
	
public:
	boost::atomic<buffex * > send_buff_;

public:	
	void write(char * data, std::size_t llen);

	bool send_buff();
	void clear();

private:
	boost::atomic_flag _send_flag;

};	

} //detail
} /* namespace async_net */
} /* namespace angelica */

#endif /* _SOCK_BUFF_H */
