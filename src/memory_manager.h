#ifndef MMU_H
#define MMU_H

#include "memory.h"
#include "pcb.h"
#include <vector>

class MemManager
{
private:
	Memory* memory_;
	std::vector<unsigned int> used_frame_indexes_;
	
	unsigned int frame_size_;
	unsigned int num_frames_;
	
public:
	MemManager(Memory* memory, unsigned int frame_size);
	~MemManager();
	
	Memory* GetMemory();
	unsigned int GetFrameSize();
	unsigned int GetNumFrames();
	uint32_t GetEffectiveAddress(uint32_t logical_address, uint32_t* page_table);
	uint32_t FetchWord(uint32_t absolute_address);
	
	// returns table of unused frame indexes
	// finds first available
	uint32_t* Allocate(unsigned int num_bytes);
	
	// returns single empty frame index and adds it to list of used frames
	uint32_t AllocateFrame();
	
	// releases the given page table's frames
	void Release(uint32_t* page_table, size_t size);
	
	// prints out all frames used by a profess
	void PrintFrames(PCB* process);
	
	float PercentageUsed();
};

#endif // MMU_H
