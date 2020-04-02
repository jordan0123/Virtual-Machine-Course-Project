#include "memory_manager.h"
#include <algorithm>
#include <iostream>
#include <math.h>

MemManager::MemManager(Memory* memory, unsigned int frame_size)
{
	memory_ = memory;
	frame_size_ = frame_size;
	
	num_frames_ = memory_->GetSize() / frame_size_;
}

MemManager::~MemManager()
{
}

Memory* MemManager::GetMemory()
{
	return memory_;
}

unsigned int MemManager::GetFrameSize()
{
	return frame_size_;
}

unsigned int MemManager::GetNumFrames()
{
	return num_frames_;
}

uint32_t MemManager::GetEffectiveAddress(uint32_t logical_address, uint32_t* page_table)
{
	return page_table[logical_address / frame_size_] == 0xFFFFFFFF ? 0xFFFFFFFF : page_table[logical_address / frame_size_] * frame_size_ + (logical_address % frame_size_);
}

uint32_t MemManager::FetchWord(uint32_t absolute_address)
{
	uint32_t buff;
	memory_->Read(absolute_address, &buff, sizeof(buff));
	return buff;
}

uint32_t* MemManager::Allocate(unsigned int num_bytes)
{
	int frames_to_allocate = ceil(num_bytes / (float)frame_size_);
	//std::cout << std::dec << num_bytes << std::endl;
	uint32_t* frames = new unsigned int[frames_to_allocate];
	
	unsigned int index = 0;
	unsigned int frames_allocated = 0;
	
	while (index <= num_frames_ && frames_allocated < frames_to_allocate)
	{
		if (std::find(used_frame_indexes_.begin(), used_frame_indexes_.end(), index) == used_frame_indexes_.end()) // is the current frame unoccupied?
		{
			frames[frames_allocated] = index;
			used_frame_indexes_.push_back(index);
			
			frames_allocated++;
		}
		
		index++;
	}
	
	if (index > num_frames_)
	{
		std::cout << "Cannot allocate memory" << std::endl;
	}

	return frames;
}

uint32_t MemManager::AllocateFrame() // allocate one frame
{
	int index = 0;
	
	while (index <= num_frames_)
	{
		if (std::find(used_frame_indexes_.begin(), used_frame_indexes_.end(), index) == used_frame_indexes_.end()) // is the current frame unoccupied?
		{
			used_frame_indexes_.push_back(index);
			return index;
		}
		
		index++;
	}

	std::cout << "Cannot allocate memory" << std::endl;

}

void MemManager::Release(uint32_t* page_table, size_t size)
{
	for (int i = 0; i < size; i++)
	{
		used_frame_indexes_.erase(std::remove(used_frame_indexes_.begin(), used_frame_indexes_.end(), page_table[i]), used_frame_indexes_.end());
	}
}

void MemManager::PrintFrames(PCB* process)
{
	std::cout << "Program " << process->id << " frames: " << std::endl;
	
	for (int frame = 0; frame < ceil(process->program_size / (float)frame_size_); frame++)
	{
		std::cout << "Frame " << frame << ": " << std::endl;
		
		uint32_t frame_base_addr = process->page_table[frame] * frame_size_;
		
		for (uint32_t byte_addr = frame_base_addr; byte_addr < (frame_base_addr + frame_size_); byte_addr += 4)
		{
			types::Word buff;
			GetMemory()->Read(byte_addr, &buff, sizeof(buff));
			std::cout << std::hex << "[" << (int)buff << "], " << std::endl;
		}
		
		std::cout << std::endl << std::endl;
	}
}

float MemManager::PercentageUsed()
{
	return used_frame_indexes_.size() / (float)num_frames_;
}