#include "disk.h"
#include "types.h"
#include <iostream>

Disk::Disk(size_t size)
{
	data_ = new types::Byte[size];
}

Disk::~Disk()
{
	delete[] data_;
}
//**DEBUG FUNCTIONS**//
void Disk::PrintBlock(unsigned int base_address, unsigned int end_address)
{
	assert(end_address > base_address);
	
	for (unsigned int i = base_address; i <= end_address; i++) 
	{
		std::cout << std::hex << "[" << (int)data_[i] << "], ";
	}
		
	std::cout << std::endl;
}
