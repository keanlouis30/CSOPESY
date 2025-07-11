// MemoryManager.cpp
#include "MemoryManager.h"
#include "Process.h"
#include <iostream>
#include <iomanip>
#include <ctime>

void MemoryManager::initialize(size_t total_size) {
    std::lock_guard<std::mutex> lock(mtx);
    total_memory_size = total_size;
    memory_map.clear();
    // Start with one large free block
    memory_map.push_back({0, total_memory_size, true, -1, ""});
}

// Merges adjacent free blocks to reduce fragmentation
void MemoryManager::merge_free_blocks() {
    // No lock here, should be called from a locked context
    if (memory_map.size() <= 1) return;

    // Sort by address to ensure adjacency check works
    std::sort(memory_map.begin(), memory_map.end(), [](const MemoryBlock& a, const MemoryBlock& b) {
        return a.start_address < b.start_address;
    });

    for (size_t i = 0; i < memory_map.size() - 1; ) {
        if (memory_map[i].is_free && memory_map[i + 1].is_free) {
            memory_map[i].size += memory_map[i + 1].size;
            memory_map.erase(memory_map.begin() + i + 1);
            // Don't increment i, check the new next block
        } else {
            i++;
        }
    }
}

// First-Fit Allocation Logic
bool MemoryManager::allocate(Process& process, size_t required_size) {
    std::lock_guard<std::mutex> lock(mtx);

    for (auto& block : memory_map) {
        if (block.is_free && block.size >= required_size) {
            // Found a fit!
            size_t original_block_size = block.size;
            size_t original_start_address = block.start_address;

            // Update the found block to be the new allocated block
            block.is_free = false;
            block.size = required_size;
            block.process_id = process.pid;
            block.process_name = process.name;
            
            // Store memory info in the process object
            process.memory_start_address = block.start_address;
            process.memory_size = required_size;

            // If there's leftover space, create a new free block (split the block)
            if (original_block_size > required_size) {
                size_t remaining_size = original_block_size - required_size;
                size_t new_free_block_start = original_start_address + required_size;
                memory_map.push_back({new_free_block_start, remaining_size, true, -1, ""});
            }
            
            // Sort to keep the map in order of address
            std::sort(memory_map.begin(), memory_map.end(), [](const MemoryBlock& a, const MemoryBlock& b) {
                return a.start_address < b.start_address;
            });

            return true; // Allocation successful
        }
    }
    return false; // No suitable block found
}

void MemoryManager::deallocate(int process_id) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& block : memory_map) {
        if (!block.is_free && block.process_id == process_id) {
            block.is_free = true;
            block.process_id = -1;
            block.process_name = "";
            merge_free_blocks(); // Immediately merge with neighbors if they are free
            return;
        }
    }
}

size_t MemoryManager::calculate_external_fragmentation() {
    std::lock_guard<std::mutex> lock(mtx);
    size_t total_free_mem = 0;
    size_t largest_free_block = 0;
    
    for (const auto& block : memory_map) {
        if (block.is_free) {
            total_free_mem += block.size;
        }
    }
    // External fragmentation is the total free memory that is non-contiguous
    // For this assignment, it seems they just want the sum of all free blocks.
    return total_free_mem;
}

int MemoryManager::get_process_count_in_memory() {
    std::lock_guard<std::mutex> lock(mtx);
    int count = 0;
    for (const auto& block : memory_map) {
        if (!block.is_free) {
            count++;
        }
    }
    return count;
}

std::string MemoryManager::generate_memory_snapshot(const std::vector<Process>& running_processes) {
    std::lock_guard<std::mutex> lock(mtx);
    std::stringstream ss;
    
    // Create a temporary copy and sort descending by address for top-down printing
    auto temp_map = memory_map;
    std::sort(temp_map.rbegin(), temp_map.rend(), [](const MemoryBlock& a, const MemoryBlock& b) {
        return a.start_address < b.start_address;
    });

    ss << "----end---- " << total_memory_size << "\n";

    for (const auto& block : temp_map) {
        if (!block.is_free) {
            ss << "\n";
            ss << block.start_address + block.size << "\n";
            ss << block.process_name << "\n";
            ss << block.start_address << "\n";
        }
    }
    
    ss << "\n";
    ss << "----start---- 0\n";
    
    return ss.str();
}