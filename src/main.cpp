#include <iostream>
#include <vector>
#include <queue>
#include <math.h>
#include "pcb.h"
#include "disk.h"
#include "memory.h"
#include "memory_manager.h"
#include "loader.h"
#include "cpu.h"
#include "metrics.h"

Disk disk = Disk(2048 * 4);

Memory ram = Memory(1024 * 4);
MemManager mmu(&ram, 16); // 4 words per frame
	
const int CPU_COUNT = 4;
CPU* cpus[CPU_COUNT];

std::vector<PCB> programs;

std::queue<PCB*> ready_queue;
std::queue<PCB*> wait_queue;

int main()
{
	std::cout << "Start:" << std::endl;
	
	// initialize CPUs
	for (int i = 0; i < 4; i++)
	{
		cpus[i] = new CPU(&mmu);
	}
	
	// programs' data loaded into disk
	loader::LoadFileToDisk(disk, programs, "..\\DataFile.txt");
	
	// LONG-TERM SCHEDULER
	// determines order in which programs are loaded into ready_queue
	enum POLICIES {FCFS, PRIORITY, SJF};
	
	// get input for scheduling policy
	std::cout << "Enter scheduling policy [FCFS, PRIORITY, SJF] (0/1/2):" << std::endl;
	int p;
	std::cin >> p;
	
	POLICIES policy = static_cast<POLICIES>(p);
	
	switch (policy)
	{
		case FCFS:
		{
			for (int i = 0; i < programs.size(); i++)
			{
				ready_queue.push(&programs[i]);
				ready_queue.front()->status = PCB::READY;
			}
			
			break;
		}
		
		case PRIORITY:
		{
			PCB* temp[programs.size()];
			int tempsize = programs.size();
			
			for (int i = 0; i < programs.size(); i++)
			{
				temp[i] = &programs[i];
			}
			
			int highest_priority_index;
			
			while (ready_queue.size() < programs.size())
			{
				highest_priority_index = 0;
				
				for (int i = 0; i < tempsize; i++)
				{
					if (temp[i]->priority > temp[highest_priority_index]->priority)
					{
						highest_priority_index = i;
					}
				}
				
				ready_queue.push(temp[highest_priority_index]);
				temp[highest_priority_index] = temp[tempsize - 1];
				tempsize--;
			}
			
			break;
		}
		
		case SJF:
		{
			PCB* temp[programs.size()];
			int tempsize = programs.size();
			
			for (int i = 0; i < programs.size(); i++)
			{
				temp[i] = &programs[i];
			}
			
			int shortest_job_index;
			
			while (ready_queue.size() < programs.size())
			{
				shortest_job_index = 0;
				
				for (int i = 0; i < tempsize; i++)
				{
					if (temp[i]->input_buffer_offset < temp[shortest_job_index]->input_buffer_offset)
					{
						shortest_job_index = i;
					}
				}
				
				ready_queue.push(temp[shortest_job_index]);
				temp[shortest_job_index] = temp[tempsize - 1];
				tempsize--;
			}
			
			break;
		}
	}	
	
	// get input for number of CPUs to use
	std::cout << "Number of CPUs to use (1-4):" << std::endl;
	int c;
	std::cin >> c;
	
	std::cout << "Number of programs to execute (<= 30):" << std::endl;
	int n;
	std::cin >> n;
	
	float max_ram_usage = 0;
	
	int programs_to_execute = n;
	
	while (programs_to_execute > 0)
	{
		// SHORT-TERM SCHEDULER & M-DISPATCHER
		for (int cpu_index = 0; cpu_index < c; cpu_index++)
		{
			CPU*& cpu = cpus[cpu_index]; // cpu just an alias for current cpu
			
			// cpu idle
			if (cpu->GetCurrentProcess() == NULL || cpu->GetCurrentProcess()->status == PCB::TERMINATED || cpu->GetCurrentProcess()->status == PCB::WAITING)
			{
				// pick an available program/process
				if (!wait_queue.empty())
				{
					cpu->SetCurrentProcess(wait_queue.front());
					
					wait_queue.front()->cpu_id = cpu_index;
					wait_queue.pop();
				}
				else if (!ready_queue.empty())
				{
					cpu->SetCurrentProcess(ready_queue.front());
					
					// load first 4 frames of process into memory
					for (int i = 0; i < 4; i++)
					{
						loader::LoadPageToMemory(disk, mmu, ready_queue.front(), i);
					}
					
					ready_queue.front()->cpu_id = cpu_index;
					ready_queue.pop();
				}
			}
			
			if (cpu->GetCurrentProcess() != NULL && cpu->GetCurrentProcess()->status != PCB::TERMINATED)
			{
				PCB::STATUS& status = cpu->GetCurrentProcess()->status;
				
				if (status == PCB::RUNNING)
				{
					cpu->Execute();
					cpu->GetCurrentProcess()->completion_time++;
				}
				
				if (status == PCB::BLOCKED) // service page fault
				{
					status = PCB::WAITING;
					loader::LoadPageToMemory(disk, mmu, cpu->GetCurrentProcess(), cpu->GetCurrentProcess()->page_fault_index);
					cpu->GetCurrentProcess()->cpu_id = -1;
					wait_queue.push(cpu->GetCurrentProcess());
				}
				
				if (status == PCB::WAITING)
				{
					
				}
				
				if (status == PCB::TERMINATED)
				{
					mmu.PrintFrames(cpu->GetCurrentProcess());
					
					programs_to_execute--;
					cpu->GetCurrentProcess()->cpu_id = -1;
					mmu.Release(cpu->GetCurrentProcess()->page_table, ceil(cpu->GetCurrentProcess()->program_size / (float)mmu.GetFrameSize()));
				}
			}
		}
		
		// METRICS
		metrics::time++;
		
		for (int i = 0; i < programs.size(); i++)
		{
			if (programs[i].status != PCB::TERMINATED && programs[i].cpu_id == -1)
			{
				programs[i].wait_time++;
			}
		}
		
		if (mmu.PercentageUsed() > max_ram_usage)
		{
			max_ram_usage = mmu.PercentageUsed();
		}
	}
	
	std::cout << "EXECUTION COMPLETE" << std::endl << std::endl
			  << "Wait times for each job (ordered by job ID):" << std::endl;
			  
	for (int i = 0; i < programs.size(); i++)
	{
		std::cout << std::dec << programs[i].wait_time << ", ";
	}
	std::cout << std::endl << std::endl
			  << "Completion times for each job (ordered by job ID):" << std::endl;
			  
	for (int i = 0; i < programs.size(); i++)
	{
		std::cout << std::dec << programs[i].completion_time << ", ";
	}
	std::cout << std::endl << std::endl
			  << "Percentage of RAM space used (maximum): " << max_ram_usage << std::endl;
	
	// mmu.PrintFrames(&programs[3]);
/*
	for (int i = 0; i < programs.size(); i++)
	{
		std::cout << "Job " << i + 1 << ": " << std::endl
				  << "i/o ops: " << programs[i].io_ops << std::endl;
				  //<< "priority: " << programs[i].priority << std::endl << std::endl;
	}
	 */
	
	
	
	/*
	loader::LoadFileToDisk(disk, programs, "..\\DataFile.txt");
	loader::LoadToMemory(disk, mmu, &programs[1]);
	
	//ram.PrintBlock(0, 100);
	CPU*& cpu = cpus[0];
	
	cpu->SetCurrentProcess(&programs[1]);
	
	while (cpu->GetCurrentProcess()->status != PCB::TERMINATED)
	{
		cpu->Execute();
	}
	
	std::cout << std::endl;
	std::cout << std::hex << programs[1].program_counter << std::endl;
	std::cout << std::endl;
	for (int i = 0; i < 16; i++)
	{
		std::cout << i << " [" << programs[1].registers[i] << "] " << std::endl; 
	}
	std::cout << std::endl;
	
	//ram.PrintBlockPerWord(0, 500);
	mmu.PrintFrames(&programs[1]);
	*/
	//std::cout << programs[2].program_size << std::endl;
	 
	 
}