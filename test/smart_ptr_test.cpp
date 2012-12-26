#include <angelica/smart_ptr/smart_prt.h>

struct test{
	~test() {std::cout << "destroy" << std::endl;}

public:
	int a, b, c;
};

int main() {
	typedef angelica::smart_ptr::smart_prt<test> smart_ptr_test;

	for(int i = 0; i < 10000; i++){
		smart_ptr_test ptr(new test);
		ptr->a = 0;
		ptr->b = 1;
		ptr->c = 2;

		std::cout << "ptr" << ptr->a << ptr->b << ptr->c << std::endl;
	}

	return 0;
}