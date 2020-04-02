#ifndef LOADER_H
#define LOADER_H

#include <vector>
#include "disk.h"
#include "pcb.h"
#include "memory_manager.h"
#include <string>

namespace loader
{
void LoadFileToDisk(Disk& disk, std::vector<PCB>& jobs, std::string file_path);
void LoadToMemory(Disk& disk, MemManager& mmu, PCB* job);
void LoadPageToMemory(Disk& disk, MemManager& mmu, PCB* job, unsigned int page_num);
}

#endif // LOADER_H
