#pragma once

#include <vector>
#include <mutex>
#include <string>
#include <sstream>
#include <algorithm> // for std::sort

class Process;

struct MemoryBlock {
    size_t start_address;
    size_t size;
    bool is_free;
    int process_id; // -1 if free
    std::string process_name;
};

class MemoryManager {
private:
    std::vector<MemoryBlock> memory_map;
    std::mutex mtx;
    size_t total_memory_size;

    void merge_free_blocks();

public:
    MemoryManager() : total_memory_size(0) {} // Default constructor
    void initialize(size_t total_size);

    // First-Fit allocation
    bool allocate(Process& process, size_t required_size);

    void deallocate(int process_id);

    size_t calculate_external_fragmentation();
    int get_process_count_in_memory();
    std::string generate_memory_snapshot(const std::vector<Process>& running_processes);
};