#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include "pcb.h"
#include "types.h"
#include "memory_manager.h"

class CPU
{
private:
	PCB* current_process_;
	MemManager* mem_manager_;
	uint32_t program_counter_; // absolute address
	types::Word instruction_register_;
	
public:
	CPU(MemManager* mem_manager); // needs a pointer to the memory manager to fetch instructions
	~CPU();
	
	void SetCurrentProcess(PCB* process);
	PCB* GetCurrentProcess();
	
	void Execute();

};

#endif // CPU_H
