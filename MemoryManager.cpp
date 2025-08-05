#include "MemoryManager.h"
#include "Process.h" 
#include <fstream>
#include <iostream>
#include <unordered_set>

MemoryManager::MemoryManager() : page_size(0) {
    backing_store_stream.open(backing_store_filename, 
                               std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    if (!backing_store_stream.is_open()) {
        std::cerr << "FATAL: Could not create or open the backing store: " << backing_store_filename << std::endl;
    }
    backing_store_free_blocks.resize(4194304, true); 
}

MemoryManager::~MemoryManager() {
    if (backing_store_stream.is_open()) {
        backing_store_stream.close();
    }
}

long long MemoryManager::find_free_backing_store_block() {
    std::lock_guard<std::mutex> lock(backing_store_mtx);
    for (long long i = 0; i < backing_store_free_blocks.size(); ++i) {
        if (backing_store_free_blocks[i]) {
            backing_store_free_blocks[i] = false; 
            return i;
        }
    }
    return -1; // backing store is full
}

void MemoryManager::initialize(size_t total_size, size_t frame_size) {
    std::lock_guard<std::mutex> lock(mtx);
    page_size = frame_size;
    int num_frames = total_size / frame_size;
    physical_frames.resize(num_frames);
    main_memory.resize(total_size, 0); 
    fifo_queue.clear();
}

int MemoryManager::translate_address(Process& process, size_t virtual_address) {
    int page_num = virtual_address / page_size;
    int offset = virtual_address % page_size;

    if (page_num >= process.page_table.size() || !process.page_table[page_num].present) {
        return -1; 
    }

    int frame_num = process.page_table[page_num].frame_number;
    return (frame_num * page_size) + offset;
}


void MemoryManager::handle_page_fault(Process& process, int virtual_page_num) {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << "[MM] Page Fault for PID " << process.pid 
              << " on page " << virtual_page_num << std::endl;

    int frame_to_use = find_free_frame();
    if (frame_to_use == -1) {
        // No free frames, must run page replacement
        std::cout << "[MM] No free frames. Running page replacement..." << std::endl;
        frame_to_use = run_fifo_replacement();
    }
    
    // Now we have a frame, load the required page into it.
    load_page_into_frame(process, virtual_page_num, frame_to_use);
}

int MemoryManager::find_free_frame() {
    for (int i = 0; i < physical_frames.size(); ++i) {
        if (physical_frames[i].is_free) {
            return i;
        }
    }
    return -1; // No free frames
}

int MemoryManager::run_fifo_replacement() {
    int victim_frame_num = fifo_queue.front();
    fifo_queue.pop_front();

    // evict_page_from_frame(victim_frame_num);

    return victim_frame_num;
}

void MemoryManager::load_page_into_frame(Process& process, int virtual_page_num, int frame_num) {
    char* destination = &main_memory[frame_num * page_size];
    
    PageTableEntry& pte = process.page_table[virtual_page_num];

    if (pte.backing_store_block == -1) {
        std::fill(destination, destination + page_size, 0);
        
        std::cout << "[MM] Loading on-demand zero page " << virtual_page_num 
                  << " for PID " << process.pid << " into frame " << frame_num << std::endl;
    } 
    else {
        std::lock_guard<std::mutex> lock(backing_store_mtx);
        
        backing_store_stream.seekg(pte.backing_store_block * page_size);
        
        backing_store_stream.read(destination, page_size);

        if (backing_store_stream.fail()) {
            std::cerr << "CRITICAL ERROR: Failed to read from backing store block " << pte.backing_store_block
                      << ". Clearing frame to prevent data corruption.\n";
            std::fill(destination, destination + page_size, 0);
            backing_store_stream.clear();
        }
        
        std::cout << "[MM] Loaded page " << virtual_page_num << " for PID " << process.pid 
                  << " from backing store block " << pte.backing_store_block 
                  << " into frame " << frame_num << std::endl;
    }

    physical_frames[frame_num].is_free = false;
    physical_frames[frame_num].holding_pid = process.pid;
    physical_frames[frame_num].holding_page_num = virtual_page_num;

    pte.present = true;
    pte.frame_number = frame_num;
    pte.dirty = false; 

    fifo_queue.push_back(frame_num);
    
    g_page_ins++;
}

int MemoryManager::get_active_process_count() {
    std::lock_guard<std::mutex> lock(mtx);
    std::unordered_set<int> unique_pids;
    for (const auto& frame : physical_frames) {
        if (!frame.is_free) {
            unique_pids.insert(frame.holding_pid);
        }
    }
    return unique_pids.size();
}

size_t MemoryManager::get_free_memory_in_bytes() {
    std::lock_guard<std::mutex> lock(mtx);
    int free_frames = 0;
    for (const auto& frame : physical_frames) {
        if (frame.is_free) {
            free_frames++;
        }
    }
    return free_frames * page_size;
}

void MemoryManager::deallocate_all_frames_for_process(int pid_to_remove) {
    std::lock_guard<std::mutex> lock(mtx);

    // Iterate through all physical frames
    for (int i = 0; i < physical_frames.size(); ++i) {
        // If a frame belongs to the finished process...
        if (physical_frames[i].holding_pid == pid_to_remove) {
            // ...mark it as free.
            physical_frames[i].is_free = true;
            physical_frames[i].holding_pid = -1;
            physical_frames[i].holding_page_num = -1;

            // Also remove it from the FIFO queue if it's there
            fifo_queue.remove(i);
        }
    }
    std::cout << "[MM] Deallocated all frames for finished PID " << pid_to_remove << std::endl;
}

int MemoryManager::get_frame_count_for_process(int pid) {
    std::lock_guard<std::mutex> lock(mtx);
    
    int frame_count = 0;
    for (const auto& frame : physical_frames) {
        if (!frame.is_free && frame.holding_pid == pid) {
            frame_count++;
        }
    }
    return frame_count;
}

// TODO: Implement evict_page_from_frame, read_memory, write_memory