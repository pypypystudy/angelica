#include <angelica/container/ringque.h>
#include <boost/thread.hpp>
#include <boost/atomic.hpp>

struct data{
	int threadid;
	int num;
};

angelica::container::ringque<data> _que;
boost::atomic_uint count(0);

void push(int threadid){
	count++;
	for(int i = 0; i < 1000; i++){
		data data;
		data.threadid = threadid;
		data.num = i;
		_que.push(data);
	}
}

void pop(){
	while(count.load()){
		data data;
		if(_que.pop(data)){
			data.num;
		}
	}
}

int main(){
	boost::thread th1(boost::bind(&push, 0)), th2(pop);
	th1.join();
	th2.join();

	return 1;
}

