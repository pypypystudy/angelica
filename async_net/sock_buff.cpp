/*
 * simplebuff.cpp
 *
 *  Created on: 2012-5-11
 *      Author: qianqians
 */
#include "sock_buff.h"
#include "simple_pool.h"
#include "no_blocking_pool.h"

namespace angelica {
namespace async_net {
namespace detail {

static unsigned int page_size = 8192;

typedef angelica::async_net::detail::no_blocking_pool<void> mempagepl;
static mempagepl _mempagepl;

//mem page pool
void InitMemPagePool(){
	char * mempage = new char[page_size];
	ReleaseMemPage(mempage);
}

char * GetMemPage(){
	char * mempage = (char*)_mempagepl.get();
	if(mempage == 0){
		while((mempage = (char*)malloc(page_size)) == 0);
	}
#ifdef _DEBUG
	memset(mempage, 0, page_size);
#endif //_DEBUG

	return mempage;
}

void ReleaseMemPage(char * page){
	_mempagepl.release(page);
}

void DestryMemPagePool(){
	while(1){
		char * p = (char*)_mempagepl.get();
		if(p == 0){
			break;
		}
		delete p;
	}
}

//read buff
read_buff::read_buff(){
	buff = GetMemPage();
	buff_size = page_size;
	slide = 0;

#ifdef _WIN32
	_wsabuf.buf = 0;
	_wsabuf.len = 0;
#endif
}

read_buff::~read_buff() {
	ReleaseMemPage(buff);
}

void read_buff::init() {
	slide = 0;
	_wsabuf.buf = 0;
	_wsabuf.len = 0;
}

typedef angelica::async_net::detail::no_blocking_pool<read_buff> readbuffpl;
static readbuffpl _readbuffpl;

void InitReadBuffPool(){
	read_buff * p = new read_buff();
	p->fn_Release = boost::bind(&readbuffpl::release, &_readbuffpl, p);

	_readbuffpl.release(p);
}

read_buff * GetReadBuff(){
	read_buff * p = _readbuffpl.get();
	if (p == 0){
		p = new read_buff();
		p->fn_Release = boost::bind(&readbuffpl::release, &_readbuffpl, p);
	}else{
		p->init();
	}

	return p;
}

void DestryReadBuffPool(){
	while(1){
		read_buff * p = _readbuffpl.get();
		if(p == 0){
			break;
		}
		delete p;
	}
}

//write buff buffex
write_buff::buffex::buffex() : slide(0) {
	buff = GetMemPage();
	buff_size = page_size;
	slide.store(0);

#ifdef _DEBUG
	memset(buff, 0, page_size);	
#endif //_DEBUG

#ifdef _WIN32
	_wsabuf = new WSABUF[8]; 
	_wsabuf_count = 8; 
	_wsabuf_slide.store(0);
#endif	//_WIN32
}
	
write_buff::buffex::~buffex() { 
#ifdef _WIN32
	for(unsigned int i = 0; i < _wsabuf_slide.load(); i++){
		if(buff != _wsabuf[i].buf){
			if (_wsabuf[i].len > page_size){
				free(_wsabuf[i].buf);
			}else{
				ReleaseMemPage(_wsabuf[i].buf);
			}
		}
	}

	delete[] _wsabuf; 
#endif	//_WIN32

	if (buff_size > page_size){
		free(buff);
	}else{
		ReleaseMemPage(buff);
	}
}

void write_buff::buffex::clear() {
#ifdef _DEBUG
	memset(buff, 0, buff_size);
#endif //_DEBUG
	slide.store(0);

#ifdef _WIN32
	for(unsigned int i = 0; i < _wsabuf_slide.load(); i++){
		if(buff != _wsabuf[i].buf){
			if (_wsabuf[i].len > page_size){
				free(_wsabuf[i].buf);
			}else{
				ReleaseMemPage(_wsabuf[i].buf);
			}
		}
		_wsabuf[i].len = 0;
	}
	_wsabuf_slide.store(0);
#endif	//_WIN32
}

void write_buff::buffex::put_buff() {
	boost::upgrade_lock<boost::shared_mutex> lock(_wsabuf_mu);

	unsigned int nSlide = _wsabuf_slide++;
	if (nSlide >= _wsabuf_count){
		boost::unique_lock<boost::shared_mutex > uniquelock(boost::move(lock));		

		unsigned int _new_wsabuf_count = _wsabuf_count * 2;
		WSABUF * _new_wsabuf = new WSABUF[_new_wsabuf_count];
		for(unsigned int i = 0; i < _wsabuf_count; i++){
			_new_wsabuf[i] = _wsabuf[i];
		}
		delete[] _wsabuf;
		
		_wsabuf = _new_wsabuf; 
		_wsabuf_count = _new_wsabuf_count;
	}

	_wsabuf[nSlide].buf = buff;
	_wsabuf[nSlide].len = slide.load();
}

//write buff
write_buff::write_buff(){
	send_buff_.store(new buffex());
	write_buff_.store(new buffex());	
}

write_buff::~write_buff(){
	delete send_buff_;
	delete write_buff_;
}

void write_buff::init(){
	send_buff_.load()->clear();
	write_buff_.load()->clear();
	
	_send_flag.clear();
}

void write_buff::write(char * data, std::size_t llen){
	buffex * _write_buff = 0;
	while(1){
		_write_buff = write_buff_.load();
		
		if(_write_buff->_mu.try_lock_upgrade()) {
			if(_write_buff != write_buff_.load()) {
				_write_buff->_mu.unlock_upgrade();
				continue;
			}
			break;
		}
	}

	uint32_t _old_slide = _write_buff->slide.load();
	while(1){
		uint32_t _new_slide = _old_slide + llen;

		if (_new_slide > _write_buff->buff_size){
			_write_buff->_mu.unlock_upgrade_and_lock();
			
			_old_slide = _write_buff->slide.load();
			_new_slide = _old_slide + llen;

			if (_new_slide > _write_buff->buff_size){
				if (_old_slide > 0){
					_write_buff->put_buff();
				}else{
					detail::ReleaseMemPage(_write_buff->buff);
				}

				_old_slide = 0;
				_write_buff->slide.store(0);
				if (llen > page_size){
					_write_buff->buff_size = (llen + 4095)/4096*4096;
					while((_write_buff->buff = (char*)malloc(_write_buff->buff_size)) == 0);
				}else{
					_write_buff->buff_size = page_size;
					_write_buff->buff = detail::GetMemPage();
				}
			}
				
			memcpy(&_write_buff->buff[_old_slide], data, llen);
			_write_buff->slide += llen;
			
			_write_buff->_mu.unlock_and_lock_upgrade();
			break;
		}else{
			if(_write_buff->slide.compare_exchange_strong(_old_slide, _new_slide)){
				memcpy(&_write_buff->buff[_old_slide], data, llen);
				break;
			}
		}
	}
	
	_write_buff->_mu.unlock_upgrade();
}

bool write_buff::send_buff(){
	buffex * _buff = write_buff_.load();
	boost::unique_lock<boost::shared_mutex> lock(_buff->_mu, boost::try_to_lock);
	while (!lock.owns_lock()){
#ifdef _WIN32
		if(_buff->_wsabuf_slide > 8){
			if(_buff == write_buff_.load()){
				lock.lock();
				break;
			}
		}
#endif //_WIN32
		return false;
	}

	if (_send_flag.test_and_set()){
		return false;
	}

	if (_buff != write_buff_.load()){
		_send_flag.clear();
		return false;
	}

	if (_buff->_wsabuf_slide.load() <= 0 && _buff->slide.load() <= 0){
		_send_flag.clear();
		return false;
	}
		
	write_buff_.store(send_buff_.load());

	if (_buff->slide.load() > 0){
		_buff->put_buff();
	}

	send_buff_.store(_buff);
	
	return true;
}

void write_buff::clear() {
	send_buff_.load()->clear();
	_send_flag.clear();
}

typedef angelica::async_net::detail::no_blocking_pool<write_buff> writebuffpl;
static writebuffpl _writebuffpl;

void InitWriteBuffPool(){
	write_buff * p = new write_buff();
	p->fn_Release = boost::bind(&writebuffpl::release, &_writebuffpl, p);

	_writebuffpl.release(p);
}

write_buff * GetWriteBuff(){
	write_buff * p = _writebuffpl.get();
	if (p == 0){
		p = new write_buff();
		p->fn_Release = boost::bind(&writebuffpl::release, &_writebuffpl, p);
	}else{
		p->init();
	}

	return p;
}

void DestryWriteBuffPool(){
	while(1){
		write_buff * p = _writebuffpl.get();
		if(p == 0){
			break;
		}
		delete p;
	}
}

} //detail
} /* namespace async_net */
} /* namespace angelica */
