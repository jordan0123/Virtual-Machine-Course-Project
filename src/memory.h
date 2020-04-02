#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"
#include <cstdlib>

class Memory
{
private:
	// memory contents
	types::Byte* data_;
	size_t size_;
	
public:
	Memory(size_t size);
	~Memory();

	// reads memory and stores it in the buffer
	// needs the base address, pointer to the buffer, and number of bytes to be read ( must not exceed sizeof(buffer) )
	template<typename T>
	void Read(unsigned int base_address, T* buffer, size_t size);
	
	// writes memory at the specified base address with the data given in the buffer
	// needs the base address, pointer to the buffer, and number of bytes to be written ( must not exceed sizeof(buffer) )
	template<typename T>
	void Write(unsigned int base_address, T* buffer, size_t size);
	
	unsigned int GetSize();
	
	/**DEBUG FUNCTIONS**/
	// print the contents of a memory block given a base address and size of block
	void PrintBlockPerByte(unsigned int base_address, unsigned int block_size);
	void PrintBlockPerWord(unsigned int base_address, unsigned int block_size);
};

#include "memory.template"

#endif // MEMORY_H