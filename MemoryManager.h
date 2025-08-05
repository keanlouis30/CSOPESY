#pragma once

#include <vector>
#include <mutex>
#include <string>
#include <list>
#include <fstream>

class Process;

class PageTableEntry
{
public:
    bool present = false;
    bool dirty = false;
    int frame_number = -1;
    long long backing_store_block = -1; 

    void markAsPresent(int f_num)
    {
        present = true;
        frame_number = f_num;
        dirty = false;
    }

    void invalidate()
    {
        present = false;
        frame_number = -1;
    }
};

class Frame
{
public:
    bool is_free = true;
    int holding_pid = -1;
    int holding_page_num = -1;
};

class MemoryManager
{
private:
    std::vector<Frame> physical_frames;
    std::vector<char> main_memory;
    std::list<int> fifo_queue;
    std::mutex mtx;
    size_t page_size;

    int find_free_frame();
    int run_fifo_replacement();
    void load_page_into_frame(Process &process, int virtual_page_num, int frame_num);
    // void evict_page_from_frame(int frame_num);

    int translate_address(Process &process, size_t virtual_address);

    std::string backing_store_filename = "csopesy-backing-store.txt";
    std::fstream backing_store_stream; 
    std::vector<bool> backing_store_free_blocks;
    std::mutex backing_store_mtx;

    long long find_free_backing_store_block();


public:
    MemoryManager();
    ~MemoryManager();
    void initialize(size_t total_size, size_t frame_size);

    void handle_page_fault(Process &process, int virtual_page_num);

    // uint16_t read_memory(Process &process, size_t virtual_address);
    // void write_memory(Process &process, size_t virtual_address, uint16_t value);
    int get_active_process_count();
    size_t get_free_memory_in_bytes();
    int get_process_count_in_memory();
    // size_t calculate_external_fragmentation();
    int get_frame_count_for_process(int pid);
    void deallocate_all_frames_for_process(int pid);
};