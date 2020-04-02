#include "cpu.h"
#include <iostream>
#include <math.h>
#include "metrics.h"

CPU::CPU(MemManager* mem_manager)
{
	mem_manager_ = mem_manager;
	current_process_ = NULL;
}

CPU::~CPU()
{
}

void CPU::SetCurrentProcess(PCB* process)
{
	current_process_ = process;
	current_process_->status = PCB::RUNNING;
}

PCB* CPU::GetCurrentProcess()
{
	return current_process_;
}

void CPU::Execute()
{
	
	// determine if current page is valid
	if (current_process_->page_table[current_process_->program_counter / mem_manager_->GetFrameSize()] == 0xFFFFFFFF)
	{
		current_process_->status = PCB::BLOCKED;
		current_process_->page_fault_index = current_process_->program_counter / mem_manager_->GetFrameSize();
		std::cout << "PAGE FAULT" << std::endl;
		return;
	}
	
	// the CPU gets the absolulte address of the current instruction
	program_counter_ = mem_manager_->GetEffectiveAddress(current_process_->program_counter, current_process_->page_table);
	
	// the CPU gets the instruction at the location of the program_counter_
	instruction_register_ = mem_manager_->FetchWord(program_counter_);

	// decode
	uint8_t format = instruction_register_ >> 30; // front 2 bits
	uint8_t opcode = (instruction_register_ >> 24) & 0b111111; // next 6 bits. cut off lower 24 bits and select the 6 remaining
	
	switch (opcode)
	{
		case 0x0: // RD | Reads content of I/P buffer into a accumulator
		{
			current_process_->io_ops++;
			
			uint8_t reg1 = (instruction_register_ >> 20) & 0xF;
			uint8_t reg2 = (instruction_register_ >> 16) & 0xF;
			uint16_t address = instruction_register_ & 0xFFFF;
			
			if (reg2 > 0)
			{
				address = current_process_->registers[reg2];
			}
			
			// read content
			uint32_t absolute_address = mem_manager_->GetEffectiveAddress(address, current_process_->page_table);// ip buffer absolute address
			
			if (absolute_address == 0xFFFFFFFF)
			{
				current_process_->status = PCB::BLOCKED;
				current_process_->page_fault_index = address / mem_manager_->GetFrameSize();
				std::cout << "PAGE FAULT" << std::endl;
				return;
			}
			
			types::Word content = mem_manager_->FetchWord(absolute_address);
			
			current_process_->registers[reg1] = content;
			
			break;
		}
		
		case 0x1: // WR | writes the content of accumulator into O/P buffer
		{
			current_process_->io_ops++;
			 
			uint8_t reg1 = (instruction_register_ >> 20) & 0xF;
			uint8_t reg2 = (instruction_register_ >> 16) & 0xF;
			uint16_t address = instruction_register_ & 0xFFFF;
			
			uint32_t absolute_address;
			
			if (address == 0)
			{
				absolute_address = mem_manager_->GetEffectiveAddress(current_process_->registers[reg2], current_process_->page_table);// op buffer absolute address
				
				if (absolute_address == 0xFFFFFFFF)
				{
					current_process_->status = PCB::BLOCKED;
					current_process_->page_fault_index = (current_process_->registers[reg2]) / mem_manager_->GetFrameSize();
					std::cout << "PAGE FAULT" << std::endl;
					return;
				}
			}
			else
			{
				absolute_address = mem_manager_->GetEffectiveAddress(address, current_process_->page_table);// op buffer absolute address
				
				if (absolute_address == 0xFFFFFFFF)
				{
					current_process_->status = PCB::BLOCKED;
					current_process_->page_fault_index = address / mem_manager_->GetFrameSize();
					std::cout << "PAGE FAULT" << std::endl;
					return;
				}
			}
			
			
			mem_manager_->GetMemory()->Write(absolute_address, &current_process_->registers[reg1], sizeof(types::Word));
			
			break;
		}
		
		case 0x2: // ST | stores content of a reg. into an addresss
		{
			uint8_t breg = (instruction_register_ >> 20) & 0xF;
			uint8_t dreg = (instruction_register_ >> 16) & 0xF;
			uint16_t address = instruction_register_ & 0xFFFF;
			
			types::Word breg_content = current_process_->registers[breg];
			types::Word dreg_content = current_process_->registers[dreg];
			
			address += dreg_content;
			
			// write content
			uint32_t absolute_address = mem_manager_->GetEffectiveAddress(address, current_process_->page_table);
			
			if (absolute_address == 0xFFFFFFFF)
			{
				current_process_->status = PCB::BLOCKED;
				current_process_->page_fault_index = address / mem_manager_->GetFrameSize();
				std::cout << "PAGE FAULT" << std::endl;
				return;
			}
			
			mem_manager_->GetMemory()->Write(absolute_address, &breg_content, sizeof(types::Word));
			
			break;
		}
		
		case 0x3: // LW | loads content of an address into a reg
		{
			uint8_t breg = (instruction_register_ >> 20) & 0xF;
			uint8_t dreg = (instruction_register_ >> 16) & 0xF;
			uint16_t address = instruction_register_ & 0xFFFF;
			
			uint32_t absolute_address = mem_manager_->GetEffectiveAddress(address + current_process_->registers[breg], current_process_->page_table);
			
			if (absolute_address == 0xFFFFFFFF)
			{
				current_process_->status = PCB::BLOCKED;
				current_process_->page_fault_index = (address + current_process_->registers[breg]) / mem_manager_->GetFrameSize();
				std::cout << "PAGE FAULT" << std::endl;
				return;
			}
			
			current_process_->registers[dreg] = mem_manager_->FetchWord(absolute_address);
			
			break;
		}
		
		case 0x4: // MOV | transfers the content of one register into another
		{
			uint8_t sreg1 = (instruction_register_ >> 20) & 0xF;
			uint8_t sreg2 = (instruction_register_ >> 16) & 0xF;
			uint8_t dreg = (instruction_register_ >> 12) & 0xF;
			
			current_process_->registers[sreg1] = current_process_->registers[sreg2]; 
			
			break;
		}
		
		case 0x5: // ADD | adds content of two s-regs into d-reg
		{
			uint8_t sreg1 = (instruction_register_ >> 20) & 0xF;
			uint8_t sreg2 = (instruction_register_ >> 16) & 0xF;
			uint8_t dreg = (instruction_register_ >> 12) & 0xF;
			
			current_process_->registers[dreg] = current_process_->registers[sreg1] + current_process_->registers[sreg2]; 
			
			break;
		}
		
		case 0x6: // SUB | subtracts content of two s-regs into d-reg
		{
			uint8_t sreg1 = (instruction_register_ >> 20) & 0xF;
			uint8_t sreg2 = (instruction_register_ >> 16) & 0xF;
			uint8_t dreg = (instruction_register_ >> 12) & 0xF;
			
			current_process_->registers[dreg] = current_process_->registers[sreg1] - current_process_->registers[sreg2]; 
			
			break;
		}
		
		case 0x7: // MUL | multiplies content of two s-regs into d-reg
		{
			uint8_t sreg1 = (instruction_register_ >> 20) & 0xF;
			uint8_t sreg2 = (instruction_register_ >> 16) & 0xF;
			uint8_t dreg = (instruction_register_ >> 12) & 0xF;
			
			current_process_->registers[dreg] = current_process_->registers[sreg1] * current_process_->registers[sreg2]; 
			
			break;
		}
		
		case 0x8: // DIV | divides content of two s-regs into d-reg
		{
			uint8_t sreg1 = (instruction_register_ >> 20) & 0xF;
			uint8_t sreg2 = (instruction_register_ >> 16) & 0xF;
			uint8_t dreg = (instruction_register_ >> 12) & 0xF;
			
			current_process_->registers[dreg] = current_process_->registers[sreg1] / current_process_->registers[sreg2]; 
			
			break;
		}
		
		case 0x9: // AND | logical AND of two s-regs into d-reg
		{
			uint8_t sreg1 = (instruction_register_ >> 20) & 0xF;
			uint8_t sreg2 = (instruction_register_ >> 16) & 0xF;
			uint8_t dreg = (instruction_register_ >> 12) & 0xF;
			
			current_process_->registers[dreg] = current_process_->registers[sreg1] & current_process_->registers[sreg2]; 
			
			break;
		}
		
		case 0xA: // OR | logical OR of two s-regs into d-reg
		{
			uint8_t sreg1 = (instruction_register_ >> 20) & 0xF;
			uint8_t sreg2 = (instruction_register_ >> 16) & 0xF;
			uint8_t dreg = (instruction_register_ >> 12) & 0xF;
			
			current_process_->registers[dreg] = current_process_->registers[sreg1] | current_process_->registers[sreg2]; 
			
			break;
		}
		
		case 0xB: // MOVI | transfers address/data directly into a register
		{
			uint8_t breg = (instruction_register_ >> 20) & 0xF;
			uint8_t dreg = (instruction_register_ >> 16) & 0xF;
			uint16_t address = instruction_register_ & 0xFFFF;
			
			current_process_->registers[dreg] = address;
			
			break;
		}
		
		case 0xC: // ADDI | Adds a data value directly into the content of a register
		{
			uint8_t breg = (instruction_register_ >> 20) & 0xF;
			uint8_t dreg = (instruction_register_ >> 16) & 0xF;
			uint16_t address = instruction_register_ & 0xFFFF;
			
			current_process_->registers[dreg] += address;
			
			break;
		}
		
		case 0xD: // MULI | Multiplies a data value directly into the content of a register
		{
			uint8_t breg = (instruction_register_ >> 20) & 0xF;
			uint8_t dreg = (instruction_register_ >> 16) & 0xF;
			uint16_t address = instruction_register_ & 0xFFFF;
			
			current_process_->registers[dreg] *= address;
			
			break;
		}
		
		case 0xE: // DIVI | Divides a data value directly into the content of a register
		{
			uint8_t breg = (instruction_register_ >> 20) & 0xF;
			uint8_t dreg = (instruction_register_ >> 16) & 0xF;
			uint16_t address = instruction_register_ & 0xFFFF;
			
			current_process_->registers[dreg] /= address;
			
			break;
		}
		
		case 0xF: // LDI | Loads a data/address directly into the content of a register
		{
			uint8_t breg = (instruction_register_ >> 20) & 0xF;
			uint8_t dreg = (instruction_register_ >> 16) & 0xF;
			uint16_t address = instruction_register_ & 0xFFFF;
			
			current_process_->registers[dreg] = address;
			
			break;
		}
		
		case 0x10: // SLT | Sets the D-reg to 1 if the first Sreg is less than the B-reg; 0 otherwise
		{
			uint8_t sreg1 = (instruction_register_ >> 20) & 0xF;
			uint8_t sreg2 = (instruction_register_ >> 16) & 0xF;
			uint8_t dreg = (instruction_register_ >> 12) & 0xF;
			
			current_process_->registers[dreg] = current_process_->registers[sreg1] < current_process_->registers[sreg2] ? 1 : 0; 
			
			break;
		}
		
		case 0x11: // SLTI | Sets the D-reg to 1 if the first S-reg is less than a data; 0 otherwise
		{
			uint8_t breg = (instruction_register_ >> 20) & 0xF;
			uint8_t dreg = (instruction_register_ >> 16) & 0xF;
			uint16_t address = instruction_register_ & 0xFFFF;
			
			current_process_->registers[dreg] = current_process_->registers[breg] < address ? 1 : 0;
			
			break;
		}
		
		case 0x12: // HLT | Logical end of program
		{
			// terminate program
			// mem_manager_->Release(current_process_->page_table, ceil(current_process_->program_size / (float)mem_manager_->GetFrameSize()));
			current_process_->status = PCB::TERMINATED;
			// current_process_ = NULL;
			return;
		}
		
		case 0x13: // NOP | Do nothing
		{
			break;
		}
		
		case 0x14: // JMP | Jumps to a specified location
		{
			uint16_t address = instruction_register_ & 0xFFFFFF;
			
			current_process_->program_counter = address - sizeof(types::Word); // subtract 4 cause program counter will always increment 4 a the end
			
			break;
		}
		
		case 0x15: // BEQ | Branches to an address when the content of B-reg = D-reg
		{
			uint8_t breg = (instruction_register_ >> 20) & 0xF;
			uint8_t dreg = (instruction_register_ >> 16) & 0xF;
			uint16_t address = instruction_register_ & 0xFFFF;
			
			if (current_process_->registers[breg] == current_process_->registers[dreg])
			{
				current_process_->program_counter = address - sizeof(types::Word);
			}
			
			break;
		}
		
		case 0x16: // BNE | Branches to an address when the content of B-reg != D-reg
		{
			uint8_t breg = (instruction_register_ >> 20) & 0xF;
			uint8_t dreg = (instruction_register_ >> 16) & 0xF;
			uint16_t address = instruction_register_ & 0xFFFF;
			
			if (current_process_->registers[breg] != current_process_->registers[dreg])
			{
				current_process_->program_counter = address - sizeof(types::Word);
			}
			
			break;
		}
		
		case 0x17: // BEZ | Branches to an address when the content of B-reg = 0
		{
			uint8_t breg = (instruction_register_ >> 20) & 0xF;
			uint8_t dreg = (instruction_register_ >> 16) & 0xF;
			uint16_t address = instruction_register_ & 0xFFFF;
			
			if (current_process_->registers[breg] == 0)
			{
				current_process_->program_counter = address - sizeof(types::Word);
			}
			
			break;
		}
		
		case 0x18: // BNZ | Branches to an address when the content of B-reg != 0
		{
			uint8_t breg = (instruction_register_ >> 20) & 0xF;
			uint8_t dreg = (instruction_register_ >> 16) & 0xF;
			uint16_t address = instruction_register_ & 0xFFFF;
			
			if (current_process_->registers[breg] != 0)
			{
				current_process_->program_counter = address - sizeof(types::Word);
			}
			
			break;
		}
		
		case 0x19: // BGZ | Branches to an address when the content of B-reg > 0
		{
			uint8_t breg = (instruction_register_ >> 20) & 0xF;
			uint8_t dreg = (instruction_register_ >> 16) & 0xF;
			uint16_t address = instruction_register_ & 0xFFFF;
			
			if (!(current_process_->registers[breg] & 0x80000000)) // not sure about this
			{
				current_process_->program_counter = address - sizeof(types::Word);
			}
			
			break;
		}
		
		case 0x1A: // BLZ | Branches to an address when the content of B-reg < 0
		{
			uint8_t breg = (instruction_register_ >> 20) & 0xF;
			uint8_t dreg = (instruction_register_ >> 16) & 0xF;
			uint16_t address = instruction_register_ & 0xFFFF;
			
			if (current_process_->registers[breg] & 0x80000000) // not sure about this
			{
				current_process_->program_counter = address - sizeof(types::Word);
			}
			
			break;
		}
	}
	
	//std::cout << std::hex << opcode << std::endl;
	
	current_process_->program_counter += sizeof(types::Word);
}