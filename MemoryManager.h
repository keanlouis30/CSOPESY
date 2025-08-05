#pragma once

#include <vector>
#include <mutex>
#include <string>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include <queue>
#include <ctime>

class Process;

// Page Table Entry (PTE) - represents a single page in a process's virtual address space
struct PageTableEntry {
    bool valid;           // Is this page currently in physical memory?
    int frame_number;     // Physical frame number (-1 if not in memory)
    bool dirty;           // Has this page been modified?
    bool referenced;      // Has this page been accessed recently? (for LRU)
    size_t disk_address;  // Address in backing store where page is stored
    std::time_t last_access_time; // Timestamp of last access (for LRU)
    
    PageTableEntry() : valid(false), frame_number(-1), dirty(false), 
                      referenced(false), disk_address(0), last_access_time(0) {}
};

// Physical Memory Frame - represents a single frame in physical memory
struct PhysicalFrame {
    bool is_free;         // Is this frame available for allocation?
    int process_id;       // Which process owns this frame (-1 if free)
    int page_number;      // Which page number within the process
    bool is_instruction_page; // True if this frame contains instructions, false if variables
    std::vector<uint8_t> data; // Actual data stored in this frame
    std::time_t last_access_time; // When this frame was last accessed
    std::time_t load_time; // When this frame was loaded (for FIFO)
    
    PhysicalFrame() : is_free(true), process_id(-1), page_number(-1), 
                     is_instruction_page(false), last_access_time(0), load_time(0) {}
};

// Memory Block - for backward compatibility with existing allocation system
struct MemoryBlock {
    size_t start_address;
    size_t size;
    bool is_free;
    int process_id; // -1 if free
    std::string process_name;
};

// Process Memory Layout - tracks instruction and variable pages separately
struct ProcessMemoryLayout {
    int process_id;
    size_t instruction_pages;     // Number of instruction pages
    size_t variable_pages;        // Number of variable pages
    size_t total_variables;       // Total number of variables (uint16_t)
    size_t total_instructions;    // Total number of instructions
    int program_counter;          // Current instruction being executed
    std::vector<uint16_t> variables; // Variable storage (64 bytes = 32 uint16_t variables)
    
    ProcessMemoryLayout() : process_id(-1), instruction_pages(0), variable_pages(0),
                           total_variables(0), total_instructions(0), program_counter(0) {}
};

class MemoryManager {
private:
    // Physical memory management
    std::vector<PhysicalFrame> physical_memory;  // Array of physical memory frames
    std::mutex mtx;                              // Mutex for thread safety
    size_t total_memory_size;                    // Total physical memory size
    size_t frame_size;                           // Size of each frame
    size_t total_frames;                         // Total number of frames
    
    // Page table management - SEPARATE for instructions and variables
    std::unordered_map<int, std::vector<PageTableEntry>> instruction_page_tables; // Process ID -> Instruction Page Table
    std::unordered_map<int, std::vector<PageTableEntry>> variable_page_tables;    // Process ID -> Variable Page Table
    
    // Process memory layouts
    std::unordered_map<int, ProcessMemoryLayout> process_layouts;
    
    // Backing store management
    std::string backing_store_file;              // File path for backing store
    size_t pages_paged_in;                       // Statistics: pages loaded from disk
    size_t pages_paged_out;                      // Statistics: pages written to disk
    size_t page_faults;                          // Statistics: total page faults
    
    // Page replacement algorithm support
    std::queue<int> fifo_queue;                  // FIFO queue for page replacement
    std::string replacement_algorithm;           // "FIFO" or "LRU"
    
    // Legacy memory block system (for backward compatibility)
    std::vector<MemoryBlock> memory_map;
    
    // Private helper methods
    void initialize_backing_store();
    void initialize_physical_memory();
    int find_free_frame();
    int select_victim_frame_fifo();
    int select_victim_frame_lru();
    int select_victim_frame(); // Main method that chooses between FIFO and LRU
    void page_in(int process_id, int page_number, int frame_number, bool is_instruction);
    void page_out(int frame_number);
    void merge_free_blocks();
    bool is_power_of_two(size_t size);
    size_t get_next_power_of_two(size_t size);
    
    // New methods for separate instruction/variable handling
    bool access_instruction_page(int process_id, int page_number, bool is_write);
    bool access_variable_page(int process_id, int page_number, bool is_write);
    size_t calculate_instruction_pages(size_t instruction_count);
    size_t calculate_variable_pages(size_t variable_count);

public:
    // Constructor
    MemoryManager() : total_memory_size(0), frame_size(0), total_frames(0), 
                      pages_paged_in(0), pages_paged_out(0), page_faults(0),
                      backing_store_file("csopesy-backing-store.txt"),
                      replacement_algorithm("FIFO") {}
    
    // Initialization
    void initialize(size_t total_size, size_t frame_size);
    void set_replacement_algorithm(const std::string& algorithm) { replacement_algorithm = algorithm; }
    
    // Memory allocation (legacy system)
    bool allocate(Process& process, size_t required_size);
    void deallocate(int process_id);
    
    // Demand paging interface - SEPARATE for instructions and variables
    bool access_memory(int process_id, size_t virtual_address, bool is_write);
    bool access_instruction(int process_id, int instruction_index, bool is_write);
    bool access_variable(int process_id, int variable_index, bool is_write);
    
    // Process memory layout management
    bool create_process_memory_layout(int process_id, size_t instruction_count, size_t variable_count);
    void remove_process_memory_layout(int process_id);
    ProcessMemoryLayout* get_process_layout(int process_id);
    
    // Page table management (legacy - now uses separate tables)
    bool create_page_table(int process_id, size_t virtual_memory_size);
    void remove_page_table(int process_id);
    
    // Statistics and reporting
    size_t get_pages_paged_in() const { return pages_paged_in; }
    size_t get_pages_paged_out() const { return pages_paged_out; }
    size_t get_page_faults() const { return page_faults; }
    double get_page_fault_rate() const; // New: page fault rate calculation
    size_t get_total_frames() const { return total_frames; }
    size_t get_used_frames() const;
    size_t get_free_frames() const;
    size_t calculate_external_fragmentation();
    int get_process_count_in_memory();
    
    // Memory visualization
    std::string generate_memory_snapshot(const std::vector<Process>& running_processes);
    std::string generate_vmstat_report();
    std::string generate_page_table_report(int process_id);
    std::string generate_process_memory_report(int process_id); // New: detailed process memory report
};