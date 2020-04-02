#include "loader.h"
#include "types.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <assert.h>
#include <vector>
#include "utils.h"
#include <string>
#include "memory_manager.h"
#include <math.h>

namespace loader
{

void LoadFileToDisk(Disk& disk, std::vector<PCB>& jobs, std::string file_path)
{	
	std::string line; // current line being read
	std::ifstream data_file(file_path);
	
	if (data_file.is_open())
	{
		std::cout << "opened file" << std::endl;
		
		unsigned int cur_address = 0; // current address in Disk

		while (getline(data_file, line)) // read each line
		{
			if (line.substr(0, 2) == "//") // control card
			{
				std::vector<std::string> controls = utils::split(line, ' '); // includes the "//"
				
				if (controls[1] == "JOB") // new job
				{
					jobs.push_back(PCB());

					jobs.back().id = std::stoul(controls[2], 0, 16); // job id
					jobs.back().program_size += std::stoul(controls[3], 0, 16) * sizeof(types::Word); // code size
					jobs.back().priority = std::stoul(controls[4], 0, 16); // job priority
					jobs.back().disk_address = cur_address; // disk address
				}
				
				if (controls[1] == "Data")
				{
					jobs.back().input_buffer_offset = jobs.back().program_size;
					jobs.back().program_size += std::stoul(controls[2], 0, 16) * sizeof(types::Word);
					
					jobs.back().output_buffer_offset = jobs.back().program_size;
					jobs.back().program_size += std::stoul(controls[3], 0, 16) * sizeof(types::Word);
					
					jobs.back().temp_buffer_offset = jobs.back().program_size;
					jobs.back().program_size += std::stoul(controls[4], 0, 16) * sizeof(types::Word);
				}
			}
			else // instruction Word
			{
				types::Word w = std::stoul(line, 0, 16);
				disk.Write(cur_address, &w, sizeof(w));
				cur_address += sizeof(w);
			}
		}
		
		data_file.close(); // close file stream
	}
}

void LoadToMemory(Disk& disk, MemManager& mmu, PCB* job) // DEPRECATED. NOW USING DEMAND PAGING
{
	// set up page table
	job->page_table = mmu.Allocate(job->program_size);
	
	// write all pages to frames
	for (uint32_t logical_address = 0; logical_address < job->program_size; logical_address++)
	{
		uint32_t effective_address = mmu.GetEffectiveAddress(logical_address, job->page_table);
		
		types::Byte cur_byte;
		disk.Read(job->disk_address + logical_address, &cur_byte, sizeof(cur_byte));
		mmu.GetMemory()->Write(effective_address, &cur_byte, sizeof(cur_byte));
	}
}

// load single page to memory

void LoadPageToMemory(Disk& disk, MemManager& mmu, PCB* job, unsigned int page_num)
{
	uint32_t new_frame_index = mmu.AllocateFrame();
	job->page_table[page_num] = new_frame_index;
	
	uint32_t starting_absolute_address = new_frame_index * mmu.GetFrameSize(); // in memory
	uint32_t disk_address = job->disk_address + page_num * mmu.GetFrameSize();
	
	for (uint32_t absolute_address = starting_absolute_address; absolute_address < (starting_absolute_address + mmu.GetFrameSize()); absolute_address++, disk_address++)
	{
		types::Byte cur_byte;
		disk.Read(disk_address, &cur_byte, sizeof(cur_byte));
		mmu.GetMemory()->Write(absolute_address, &cur_byte, sizeof(cur_byte));
	}
}

}