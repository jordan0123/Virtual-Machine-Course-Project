#include "memory.h"
#include "types.h"
#include <assert.h>
#include <iostream>

Memory::Memory(size_t size)
{
	data_ = new types::Byte[size];
	size_ = size;
}

Memory::~Memory()
{
	delete[] data_;
}

unsigned int Memory::GetSize()
{
	return size_;
}

//**DEBUG FUNCTIONS**//
void Memory::PrintBlockPerByte(unsigned int base_address, unsigned int block_size)
{
	
	for (unsigned int i = base_address; i < base_address + block_size; i++) 
	{
		std::cout << std::hex << "[" << (int)data_[i] << "], ";
	}
		
	std::cout << std::endl;
}

void Memory::PrintBlockPerWord(unsigned int base_address, unsigned int block_size)
{
	
	for (unsigned int i = base_address; i < base_address + block_size; i += sizeof(types::Word)) 
	{
		std::cout << "[";
		
		for (int j = 0; j < sizeof(types::Word); j++)
		{
			if (data_[i + j] > 0xF)
			{
				std::cout << std::hex << (int)data_[i + j];
			}
			else
			{
				std::cout << std::hex << "0" << (int)data_[i + j];
			}
		}
		
		std::cout << "], " << std::endl;
	}
		
	std::cout << std::endl;
}