#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <ctime>
#include "Status.h"
#include "Config.h"
#include "Globals.h"
#include "MemoryManager.h"

class Process
{
public:
    std::string name;
    int pid;
    std::vector<std::string> commands;
    std::unordered_map<std::string, uint16_t> variables;
    std::unordered_map<std::string, uint16_t> variable_table;
    uint16_t next_offset;
    int totalCommands;
    Status status;
    bool has_page_fault = false; 
    int faulting_page_num = -1; 
    std::string creation_timestamp;
    int assigned_core_id;
    int quantum_remaining;
    int quantum_max;
    int commandCounter;
    // size_t memory_start_address;
    // size_t memory_size;

    std::vector<PageTableEntry> page_table;

    Process(std::string n, size_t mem_size, int p, const Config& config, bool generate_inst); 
        
    // Process(std::string n, int p, const Config& config, const std::vector<std::string>& user_commands); 
    // Process(std::string n, int p, const Config& config); 

    void initialize_virtual_memory(size_t virtual_memory_size, size_t page_size);

    Process(const Process &) = default;
    Process() = default;

private:
    void generate_instructions(const Config &config);
};