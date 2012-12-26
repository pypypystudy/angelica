#include <boost/thread.hpp>
#include <angelica/container/concurrent_queue.h>
#include <boost/timer.hpp>

const int count_per_thread_push = 1000000;

angelica::container::concurrent_queue<std::clock_t> queue;

void fn_push()
{
	for(auto i = 1; i <= count_per_thread_push/4; i++)
	{
		//std::cout << "th_id" << th_id << "num" << i << std::endl;
		std::clock_t now = std::clock();
		queue.push(now);
		queue.push(now);
		queue.push(now);
		queue.push(now);
	}
}

void fn_pop()
{
	std::clock_t t;
	for(int i = 0; i < count_per_thread_push/4; i++){
		while(queue.pop(t));
	}
}

int main()
{
	std::clock_t begin = std::clock();

	std::cout << queue.size() << std::endl;

	//boost::thread * th_push[10];
	//for(int i = 0; i <10; i++)
	//{
	//	th_push[i] = new boost::thread(boost::bind(fn_push));
	//}

	//for(int i = 0; i <10; i++)
	//{
	//	th_push[i]->join();
	//}

	std::cout << queue.size() << std::endl;

	boost::thread * th_pop[10];
	for(int i = 0; i < 10; i++)
	{
		th_pop[i] = new boost::thread(boost::bind(fn_pop));
	}

	for(int i = 0; i < 10; i++)
	{
		th_pop[i]->join();
	}

	std::cout << queue.size() << std::endl;

	for(int i = 0; i < 10; i++)
	{
		//delete th_push[i];
		delete th_pop[i];
	}

	std::cout << std::clock() - begin;

	int n;
	std::cin >> n;

	return 0;
}