#ifndef PCB_H
#define PCB_H

#include "iostream"

// process control block
struct PCB
{
	unsigned int id;
	unsigned int priority;
	
	int cpu_id;
	uint32_t program_counter; // logical address
	uint32_t* page_table;

	unsigned int program_size; // in bytes
	unsigned int input_buffer_offset; // relative to base address
	unsigned int output_buffer_offset;
	unsigned int temp_buffer_offset;
	
	uint32_t disk_address; // base address of program in disk
	
	uint32_t registers[16];
	
	enum STATUS {READY, RUNNING, WAITING, BLOCKED, TERMINATED};
	STATUS status;
	
	uint32_t page_fault_index;
	
	// METRICS
	int io_ops;
	int wait_time;
	int completion_time;
	
	PCB()
	{
		cpu_id = -1;
		program_size = 0;
		program_counter = 0;
		
		registers[1] = 0; // the Zero register
		
		page_table = new uint32_t[0x40];
		
		for (int i = 0; i < 0x40; i++)
		{
			page_table[i] = 0xFFFFFFFF; // invalid page
		}
		
		// METRICS
		io_ops = 0;
		wait_time = 0;
		completion_time = 0;
	}
};

#endif // PCB_H