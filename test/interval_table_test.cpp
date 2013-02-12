#include <angelica/container/concurrent_interval_table.h>
#include <string>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

angelica::container::concurrent_interval_table<int, std::string> table;

void insert(int n){
	for(int i = 0; i < 10000; i++){
		char buff[10];
		table.insert(i+(n*10000), "test"+(std::string)itoa(i+(n*10000), buff, 10));
	}
}

int main(){
	boost::thread th1(boost::bind(&insert, 0)), th2(boost::bind(&insert, 1));
	th1.join();
	th2.join();

	for(int i = 0; i < 2; i++){
		for(int j = 0; j < 10000; j++){
			std::string str;
			table.search(j+(i*10000), str);
			std::cout << str << std::endl;
		}
	}

	return 1;
}