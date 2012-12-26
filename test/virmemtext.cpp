#include <angelica/virmem/virmem.h>
#include <iostream>

#pragma comment (lib, "virtual_memory.lib")

int main() {
	vminit();
	vmhandle * pvm = vmalloc(4096);

	for(int i = 0; i < 4096; i++){
		((char*)pvm->mem)[i] = i;
	}

	for (int i = 0; i < 4096; i++){
		std::cout << ((char *)pvm->mem)[i] << std::endl;
	}

	swap_out(pvm);

	swap_in(pvm);

	for (int i = 0; i < 4096; i++){
		std::cout << ((char *)pvm->mem)[i] << std::endl;
	}

	return 1;
}