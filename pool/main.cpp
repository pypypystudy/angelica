#include "angmalloc.h"
#include <iostream>
#include <ctime>
#include <boost/thread.hpp>
#include <boost/atomic.hpp>

void test(){
	std::clock_t begin = clock();
	for(int j = 0; j < 5; j++){
		for(int i = 10; i < 100000; i++){
			int * ret = (int*)angmalloc(i*sizeof(int));
			for(int j = 0; j < i; j++){
				ret[j] = j;
			}
			angfree(ret);
		}
	}
	std::cout << "angmalloc" << std::clock() - begin << std::endl;
}

int main(){
	boost::thread th1(test), th2(test);
	th1.join();
	th2.join();

	int in;
	std::cin >> in;
}