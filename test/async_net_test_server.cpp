#include <angelica/async_net/async_net.h>
#include <boost/thread/mutex.hpp>
#include <boost/atomic.hpp>
#include <iostream>
#include <fstream>
#include <map>
#include <ctime>

#include "msgh.h"

#pragma comment(lib, "async_net.lib")

typedef angelica::async_net::sock_addr sock_addr;
typedef angelica::async_net::socket Socket;

std::clock_t begin, end;

boost::mutex _mu;

int count = 0;

boost::atomic_ulong testcount;
boost::atomic_ulong Recvcount;
boost::atomic_ulong Sendcount;

class Session;
std::map<unsigned int, Session * > mapSocket;

class Session
{
public:
	Session(const Socket & _s) : s(_s), _buff(new char[sizeof(msg)]), buff_llen(0) {
		s.async_recv(boost::bind(&Session::onRecv, this, _1, _2, _3), true);
	}
	~Session(){};

	Socket s;

private:
	char * _buff;
	unsigned int buff_llen;
	
public:
	void onSend(int err){
		if (!err){
			Sendcount++; 
		}else{
			_mu.lock();
			std::cout << "error code: " << err << std::endl;
			_mu.unlock();
		}
	}

private:
	int doRecv(char * buff, unsigned int lenbuff){
		while(1){
			msg * msg_ = (msg*)buff;
			msg_->csid++;
			msg_->count++;

			int n_ = msg_->csid * msg_->count;
			n_ += n_;
			n_ += n_;
			n_ += n_;

			if (msg_->csid >= mapSocket.size()){
				msg_->csid = 0;
			}

			if(msg_->csid == msg_->sid){
				testcount--;
				
				if (testcount.load() == 0){
					_mu.lock();
					end = std::clock() - begin;
					std::cout << "testcount: " << msg_->count 
							  << " testid: " << testcount.load()
							  << " Sendcount: " << Sendcount.load() 
							  << " Recvcount: " << Recvcount.load() 
							  << " testend: " << end << std::endl;
					_mu.unlock();
				}
			}else{
				Session * s = 0;
				_mu.lock();
				std::map<unsigned int, Session * >::iterator iter = mapSocket.find(msg_->csid);
				if(iter != mapSocket.end()){
					s = iter->second;
				}
				_mu.unlock();
				
				if (s != 0){
					s->s.async_send((char*)msg_, sizeof(msg), boost::bind(&Session::onSend, s, _1));
				}
			}

			lenbuff -= sizeof(msg);
			if (lenbuff < sizeof(msg)){
				break;
			}else{
				buff += sizeof(msg);
			}
		}

		return lenbuff;
	}

public:
	void onRecv(char * buff, unsigned int lenbuff, int err){
		Recvcount++;
		if(!err){
			if (buff_llen > 0){
				unsigned int copy_llen =  sizeof(msg) - buff_llen;
				if (lenbuff < copy_llen){
					copy_llen = lenbuff;
				}
				memcpy(&_buff[buff_llen], buff, copy_llen);
				buff_llen += copy_llen;
				lenbuff -= copy_llen;
				if (buff_llen == sizeof(msg)){
					doRecv(_buff, sizeof(msg));
					buff_llen = 0;
					buff += copy_llen;
				}
			}

			if (lenbuff >= sizeof(msg)){
				buff_llen = doRecv(buff, lenbuff);
				if (buff_llen > 0){
					unsigned int slide = lenbuff - buff_llen;
					memcpy(_buff, &buff[slide], buff_llen);
				}
			}else if (lenbuff > 0){
				memcpy(_buff, buff, lenbuff);
				buff_llen += lenbuff;
			}
		}else{
			_mu.lock();
			std::cout << "error code: " << err << std::endl;
			_mu.unlock();
		}
	}
};

void onAccpet(Socket s, sock_addr & addr, int err){
	if (!err){
		_mu.lock();
		int nCount = count++;
		Session * _s = new Session(s);
		mapSocket.insert(std::make_pair(nCount, _s));
		std::cout << "connect" << count << std::endl;
		_mu.unlock();
	}else{
		_mu.lock();
		std::cout << "error code: " << err << std::endl;
		_mu.unlock();
	}
}

void dotest(){
	testcount.store(mapSocket.size());
	Recvcount.store(0);
	Sendcount.store(0);

	std::cout << "testbegin" << std::endl;
	begin = std::clock();

	for(int i = 0; i < testcount.load(); i++){
		msg msg;
		msg.msgid = 0;
		msg.sid = i;
		msg.csid = i;
		msg.msgllen = sizeof(msg);
		msg.count = 0;

		Session * s = 0;
		_mu.lock();
		std::map<unsigned int, Session * >::iterator iter = mapSocket.find(i);
		if(iter != mapSocket.end()){
			s = iter->second;
		}
		_mu.unlock();
				
		if (s != 0){
			s->s.async_send((char*)&msg, sizeof(msg), boost::bind(&Session::onSend, s, _1));
		}
	}
}

int main(){
	angelica::async_net::async_service service;
	service.start();

	angelica::async_net::socket s(service);
	//s.register_accpet(&onAccpet);

	char addr[32];
	memset(addr, 0, 32);
	std::ifstream fin("server.txt");
	fin.getline(addr, 33);
	std::cout << addr << std::endl;

	s.bind(sock_addr(addr, 3311));
	s.async_accpet(100000, boost::bind(&onAccpet, _1, _2, _3), true);
	
	std::cout << "angelica async_net 性能测试 server" << std::endl;
	std::cout << "输入r 重新测试" << std::endl;
	std::cout << "输入q 退出" << std::endl;
	std::cout << "输入h 帮助" << std::endl;
	
	while(1){
		char in;
		std::cin >> in;

		switch(in){
		case 'r':
			s.async_accpet(100000, boost::bind(&onAccpet, _1, _2, _3), false);
			dotest();
			break;
		case 'q':
			goto end;
			break;
		case 'h':
			std::cout << "angelica async_net 性能测试" << std::endl;
			std::cout << "输入r 重新测试" << std::endl;
			std::cout << "输入q 退出" << std::endl;
			std::cout << "输入h 帮助" << std::endl;
			break;

		default:
			break;//do nothing
		}
	}

end:
	s.async_accpet(100000, boost::bind(&onAccpet, _1, _2, _3), false);
	s.closesocket();

	service.stop();

	for(int i = 0; i < mapSocket.size(); i++){
		delete mapSocket[i];
	}

	return 1;
}