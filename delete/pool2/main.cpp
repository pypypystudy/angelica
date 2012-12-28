#include "angmalloc.h"
#include <iostream>
#include <ctime>

int main(){
	std::clock_t begin = clock();
	for(int i = 10; i < 10000; i++){
		int * ret = 0;
		if (i == 8191){
			ret = (int*)angmalloc(i*sizeof(int));
		}else{
			ret = (int*)angmalloc(i*sizeof(int));
		}
		for(int j = 0; j < i; j++){
			ret[j] = j;
		}
		angfree(ret);
	}
	std::cout << std::clock() - begin;
}