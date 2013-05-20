#include "../container/optimistic_queue.h"
#include <boost/thread.hpp>

angelica::container::optimistic_queue<int> queue;

boost::atomic_int flag;

void test1(){
	flag++;
	for(int i = 0; i < 10000; i++){
		queue.push_front(i);
	}
	flag--;
}

void test2(){
	flag++;
	for(int i = 0; i < 10000; i++){
		queue.push_back(i);
	}
	flag--;
}

void test3(){
	int data;
	while(flag != 0){
		while(queue.pop_back(data));
	}
}

void test4(){
	int data;
	while(flag != 0){
		while(queue.pop_front(data));
	}
}

int main(){
	boost::thread_group th_g;

	for(int i = 0; i < 5; i ++){
		th_g.create_thread(test1);
	}
	for(int i = 0; i < 5; i ++){
		th_g.create_thread(test2);
	}
	for(int i = 0; i < 5; i ++){
		th_g.create_thread(test3);
	}
	for(int i = 0; i < 5; i ++){
		th_g.create_thread(test4);
	}

	th_g.join_all();

	printf("test end");

	return 1;
}