// MemoryManager.cpp
#include "MemoryManager.h"
#include "Process.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstring>

// ============================================================================
// INITIALIZATION METHODS
// ============================================================================

void MemoryManager::initialize(size_t total_size, size_t frame_size) {
    std::lock_guard<std::mutex> lock(mtx);
    
    // Store configuration parameters
    this->total_memory_size = total_size;
    this->frame_size = frame_size;
    this->total_frames = total_size / frame_size;
    
    // Initialize physical memory frames
    initialize_physical_memory();
    
    // Initialize backing store
    initialize_backing_store();
    
    // Initialize legacy memory map for backward compatibility
    memory_map.clear();
    memory_map.push_back({0, total_memory_size, true, -1, ""});
    
    // Reset statistics
    pages_paged_in = 0;
    pages_paged_out = 0;
    page_faults = 0;
    
    std::cout << "[MemoryManager] Initialized with " << total_frames 
              << " frames of " << frame_size << " bytes each" << std::endl;
}

void MemoryManager::initialize_physical_memory() {
    // Resize physical memory to hold all frames
    physical_memory.resize(total_frames);
    
    // Initialize each frame
    for (size_t i = 0; i < total_frames; ++i) {
        physical_memory[i].is_free = true;
        physical_memory[i].process_id = -1;
        physical_memory[i].page_number = -1;
        physical_memory[i].is_instruction_page = false;
        physical_memory[i].data.resize(frame_size, 0); // Initialize with zeros
        physical_memory[i].last_access_time = 0;
        physical_memory[i].load_time = 0;
    }
    
    std::cout << "[MemoryManager] Physical memory initialized with " 
              << total_frames << " frames" << std::endl;
}

void MemoryManager::initialize_backing_store() {
    // Create or truncate the backing store file
    std::ofstream file(backing_store_file, std::ios::binary | std::ios::trunc);
    if (file.is_open()) {
        // Create a large backing store (100MB) to hold pages from multiple processes
        const size_t backing_store_size = 100 * 1024 * 1024; // 100MB
        std::vector<uint8_t> buffer(backing_store_size, 0);
        file.write(reinterpret_cast<const char*>(buffer.data()), backing_store_size);
        file.close();
        std::cout << "[MemoryManager] Backing store initialized: " << backing_store_file << std::endl;
    } else {
        std::cerr << "[MemoryManager] Warning: Could not create backing store file" << std::endl;
    }
}

// ============================================================================
// PROCESS MEMORY LAYOUT MANAGEMENT
// ============================================================================

bool MemoryManager::create_process_memory_layout(int process_id, size_t instruction_count, size_t variable_count) {
    std::lock_guard<std::mutex> lock(mtx);
    
    // Calculate number of pages needed for instructions and variables
    size_t instruction_pages = calculate_instruction_pages(instruction_count);
    size_t variable_pages = calculate_variable_pages(variable_count);
    
    // Create process memory layout
    ProcessMemoryLayout layout;
    layout.process_id = process_id;
    layout.instruction_pages = instruction_pages;
    layout.variable_pages = variable_pages;
    layout.total_instructions = instruction_count;
    layout.total_variables = variable_count;
    layout.program_counter = 0; // Start at first instruction
    layout.variables.resize(32, 0); // 32 uint16_t variables (64 bytes)
    
    // Create instruction page table
    std::vector<PageTableEntry> instruction_page_table(instruction_pages);
    for (size_t i = 0; i < instruction_pages; ++i) {
        instruction_page_table[i].valid = false;
        instruction_page_table[i].frame_number = -1;
        instruction_page_table[i].dirty = false;
        instruction_page_table[i].referenced = false;
        instruction_page_table[i].disk_address = i * frame_size; // Simple mapping
        instruction_page_table[i].last_access_time = 0;
    }
    
    // Create variable page table
    std::vector<PageTableEntry> variable_page_table(variable_pages);
    for (size_t i = 0; i < variable_pages; ++i) {
        variable_page_table[i].valid = false;
        variable_page_table[i].frame_number = -1;
        variable_page_table[i].dirty = false;
        variable_page_table[i].referenced = false;
        variable_page_table[i].disk_address = (instruction_pages + i) * frame_size; // After instruction pages
        variable_page_table[i].last_access_time = 0;
    }
    
    // Store the page tables and layout
    instruction_page_tables[process_id] = instruction_page_table;
    variable_page_tables[process_id] = variable_page_table;
    process_layouts[process_id] = layout;
    
    std::cout << "[MemoryManager] Created memory layout for process " << process_id 
              << " with " << instruction_pages << " instruction pages and " 
              << variable_pages << " variable pages" << std::endl;
    return true;
}

void MemoryManager::remove_process_memory_layout(int process_id) {
    std::lock_guard<std::mutex> lock(mtx);
    
    // Remove instruction page table
    auto inst_it = instruction_page_tables.find(process_id);
    if (inst_it != instruction_page_tables.end()) {
        for (auto& pte : inst_it->second) {
            if (pte.valid && pte.frame_number >= 0) {
                if (pte.dirty) {
                    page_out(pte.frame_number);
                }
                physical_memory[pte.frame_number].is_free = true;
                physical_memory[pte.frame_number].process_id = -1;
                physical_memory[pte.frame_number].page_number = -1;
            }
        }
        instruction_page_tables.erase(inst_it);
    }
    
    // Remove variable page table
    auto var_it = variable_page_tables.find(process_id);
    if (var_it != variable_page_tables.end()) {
        for (auto& pte : var_it->second) {
            if (pte.valid && pte.frame_number >= 0) {
                if (pte.dirty) {
                    page_out(pte.frame_number);
                }
                physical_memory[pte.frame_number].is_free = true;
                physical_memory[pte.frame_number].process_id = -1;
                physical_memory[pte.frame_number].page_number = -1;
            }
        }
        variable_page_tables.erase(var_it);
    }
    
    // Remove process layout
    process_layouts.erase(process_id);
    
    std::cout << "[MemoryManager] Removed memory layout for process " << process_id << std::endl;
}

ProcessMemoryLayout* MemoryManager::get_process_layout(int process_id) {
    auto it = process_layouts.find(process_id);
    if (it != process_layouts.end()) {
        return &(it->second);
    }
    return nullptr;
}

// ============================================================================
// PAGE CALCULATION METHODS
// ============================================================================

size_t MemoryManager::calculate_instruction_pages(size_t instruction_count) {
    // Each instruction is assumed to be 64 bytes (for simplicity)
    // This can be adjusted based on actual instruction size
    size_t total_instruction_bytes = instruction_count * 64;
    return (total_instruction_bytes + frame_size - 1) / frame_size;
}

size_t MemoryManager::calculate_variable_pages(size_t variable_count) {
    // Each variable is uint16_t (2 bytes)
    // Variables are stored in 64-byte chunks (32 variables per chunk)
    size_t variable_chunks = (variable_count + 31) / 32; // 32 variables per chunk
    size_t total_variable_bytes = variable_chunks * 64;
    return (total_variable_bytes + frame_size - 1) / frame_size;
}

// ============================================================================
// SEPARATE INSTRUCTION AND VARIABLE ACCESS
// ============================================================================

bool MemoryManager::access_instruction(int process_id, int instruction_index, bool is_write) {
    std::lock_guard<std::mutex> lock(mtx);
    
    // Get process layout
    auto layout_it = process_layouts.find(process_id);
    if (layout_it == process_layouts.end()) {
        std::cerr << "[MemoryManager] Process " << process_id << " not found" << std::endl;
        return false;
    }
    
    ProcessMemoryLayout& layout = layout_it->second;
    
    // Validate instruction index
    if (instruction_index >= static_cast<int>(layout.total_instructions)) {
        std::cerr << "[MemoryManager] Invalid instruction index: " << instruction_index << std::endl;
        return false;
    }
    
    // Calculate which page this instruction belongs to
    int page_number = instruction_index / (frame_size / 64); // 64 bytes per instruction
    
    // Access the instruction page
    return access_instruction_page(process_id, page_number, is_write);
}

bool MemoryManager::access_variable(int process_id, int variable_index, bool is_write) {
    std::lock_guard<std::mutex> lock(mtx);
    
    // Get process layout
    auto layout_it = process_layouts.find(process_id);
    if (layout_it == process_layouts.end()) {
        std::cerr << "[MemoryManager] Process " << process_id << " not found" << std::endl;
        return false;
    }
    
    ProcessMemoryLayout& layout = layout_it->second;
    
    // Validate variable index
    if (variable_index >= static_cast<int>(layout.total_variables)) {
        std::cerr << "[MemoryManager] Invalid variable index: " << variable_index << std::endl;
        return false;
    }
    
    // Calculate which page this variable belongs to
    int page_number = variable_index / 32; // 32 variables per page
    
    // Access the variable page
    return access_variable_page(process_id, page_number, is_write);
}

bool MemoryManager::access_instruction_page(int process_id, int page_number, bool is_write) {
    // Find the instruction page table for this process
    auto page_table_it = instruction_page_tables.find(process_id);
    if (page_table_it == instruction_page_tables.end()) {
        std::cerr << "[MemoryManager] Instruction page table not found for process " << process_id << std::endl;
        return false;
    }
    
    auto& page_table = page_table_it->second;
    
    // Validate page number
    if (page_number >= static_cast<int>(page_table.size())) {
        std::cerr << "[MemoryManager] Invalid instruction page number: " << page_number << std::endl;
        return false;
    }
    
    PageTableEntry& pte = page_table[page_number];
    
    // Check if page is currently in physical memory
    if (!pte.valid) {
        // PAGE FAULT DETECTED - instruction page not in physical memory
        page_faults++;
        std::cout << "[MemoryManager] Instruction page fault for process " << process_id 
                  << " at page " << page_number << std::endl;
        
        // Find a free frame or select a victim frame
        int frame_number = find_free_frame();
        if (frame_number == -1) {
            // No free frames available, need to evict a page
            std::cout << "[MemoryManager] No free frames, selecting victim frame" << std::endl;
            frame_number = select_victim_frame(); // Uses configured algorithm (FIFO or LRU)
            if (frame_number == -1) {
                std::cerr << "[MemoryManager] Error: No frames available for allocation" << std::endl;
                return false; // No frames available
            }
            // Page out the victim frame
            page_out(frame_number);
        }
        
        // Load the required instruction page into the selected frame
        page_in(process_id, page_number, frame_number, true); // true = instruction page
        
        // Update page table entry
        pte.valid = true;
        pte.frame_number = frame_number;
        pte.referenced = true;
        pte.last_access_time = std::time(nullptr);
        
        pages_paged_in++;
        std::cout << "[MemoryManager] Instruction page " << page_number << " loaded into frame " 
                  << frame_number << " (pages paged in: " << pages_paged_in << ")" << std::endl;
    }
    
    // Update access information
    pte.referenced = true;
    pte.last_access_time = std::time(nullptr);
    
    if (is_write) {
        pte.dirty = true;
        std::cout << "[MemoryManager] Write access to instruction page " << page_number 
                  << " (marked as dirty)" << std::endl;
    }
    
    // Update frame access time
    physical_memory[pte.frame_number].last_access_time = std::time(nullptr);
    
    return true; // Instruction access successful
}

bool MemoryManager::access_variable_page(int process_id, int page_number, bool is_write) {
    // Find the variable page table for this process
    auto page_table_it = variable_page_tables.find(process_id);
    if (page_table_it == variable_page_tables.end()) {
        std::cerr << "[MemoryManager] Variable page table not found for process " << process_id << std::endl;
        return false;
    }
    
    auto& page_table = page_table_it->second;
    
    // Validate page number
    if (page_number >= static_cast<int>(page_table.size())) {
        std::cerr << "[MemoryManager] Invalid variable page number: " << page_number << std::endl;
        return false;
    }
    
    PageTableEntry& pte = page_table[page_number];
    
    // Check if page is currently in physical memory
    if (!pte.valid) {
        // PAGE FAULT DETECTED - variable page not in physical memory
        page_faults++;
        std::cout << "[MemoryManager] Variable page fault for process " << process_id 
                  << " at page " << page_number << std::endl;
        
        // Find a free frame or select a victim frame
        int frame_number = find_free_frame();
        if (frame_number == -1) {
            // No free frames available, need to evict a page
            std::cout << "[MemoryManager] No free frames, selecting victim frame" << std::endl;
            frame_number = select_victim_frame(); // Uses configured algorithm (FIFO or LRU)
            if (frame_number == -1) {
                std::cerr << "[MemoryManager] Error: No frames available for allocation" << std::endl;
                return false; // No frames available
            }
            // Page out the victim frame
            page_out(frame_number);
        }
        
        // Load the required variable page into the selected frame
        page_in(process_id, page_number, frame_number, false); // false = variable page
        
        // Update page table entry
        pte.valid = true;
        pte.frame_number = frame_number;
        pte.referenced = true;
        pte.last_access_time = std::time(nullptr);
        
        pages_paged_in++;
        std::cout << "[MemoryManager] Variable page " << page_number << " loaded into frame " 
                  << frame_number << " (pages paged in: " << pages_paged_in << ")" << std::endl;
    }
    
    // Update access information
    pte.referenced = true;
    pte.last_access_time = std::time(nullptr);
    
    if (is_write) {
        pte.dirty = true;
        std::cout << "[MemoryManager] Write access to variable page " << page_number 
                  << " (marked as dirty)" << std::endl;
    }
    
    // Update frame access time
    physical_memory[pte.frame_number].last_access_time = std::time(nullptr);
    
    return true; // Variable access successful
}

// ============================================================================
// PAGE REPLACEMENT ALGORITHMS
// ============================================================================

int MemoryManager::find_free_frame() {
    // Find the first available free frame
    for (size_t i = 0; i < physical_memory.size(); ++i) {
        if (physical_memory[i].is_free) {
            return static_cast<int>(i);
        }
    }
    return -1; // No free frames available
}

int MemoryManager::select_victim_frame_fifo() {
    // FIFO (First In, First Out) page replacement
    // Select the frame that was loaded first (oldest load time)
    
    if (physical_memory.empty()) {
        return -1;
    }
    
    int oldest_frame = 0;
    std::time_t oldest_time = physical_memory[0].load_time;
    
    for (size_t i = 1; i < physical_memory.size(); ++i) {
        if (!physical_memory[i].is_free && physical_memory[i].load_time < oldest_time) {
            oldest_time = physical_memory[i].load_time;
            oldest_frame = static_cast<int>(i);
        }
    }
    
    std::cout << "[MemoryManager] FIFO selected frame " << oldest_frame 
              << " as victim (loaded at " << oldest_time << ")" << std::endl;
    return oldest_frame;
}

int MemoryManager::select_victim_frame_lru() {
    // LRU (Least Recently Used) page replacement
    // Select the frame that was accessed least recently
    
    if (physical_memory.empty()) {
        return -1;
    }
    
    int lru_frame = 0;
    std::time_t lru_time = physical_memory[0].last_access_time;
    
    for (size_t i = 1; i < physical_memory.size(); ++i) {
        if (!physical_memory[i].is_free && physical_memory[i].last_access_time < lru_time) {
            lru_time = physical_memory[i].last_access_time;
            lru_frame = static_cast<int>(i);
        }
    }
    
    std::cout << "[MemoryManager] LRU selected frame " << lru_frame 
              << " as victim (last accessed at " << lru_time << ")" << std::endl;
    return lru_frame;
}

int MemoryManager::select_victim_frame() {
    // Main method that chooses between FIFO and LRU based on configuration
    if (replacement_algorithm == "LRU") {
        return select_victim_frame_lru();
    } else {
        // Default to FIFO (safest as per professor's recommendation)
        return select_victim_frame_fifo();
    }
}

// ============================================================================
// PAGE SWAPPING (BACKING STORE OPERATIONS)
// ============================================================================

void MemoryManager::page_in(int process_id, int page_number, int frame_number, bool is_instruction) {
    // Load a page from backing store into physical memory
    
    std::cout << "[MemoryManager] Paging in: Process " << process_id 
              << ", " << (is_instruction ? "Instruction" : "Variable") 
              << " Page " << page_number << " -> Frame " << frame_number << std::endl;
    
    // Calculate disk address for this page
    size_t disk_address;
    if (is_instruction) {
        auto& page_table = instruction_page_tables[process_id];
        disk_address = page_table[page_number].disk_address;
    } else {
        auto& page_table = variable_page_tables[process_id];
        disk_address = page_table[page_number].disk_address;
    }
    
    // Read page data from backing store
    std::ifstream backing_store(backing_store_file, std::ios::binary);
    if (backing_store.is_open()) {
        backing_store.seekg(disk_address);
        backing_store.read(reinterpret_cast<char*>(physical_memory[frame_number].data.data()), frame_size);
        backing_store.close();
        
        std::cout << "[MemoryManager] Page data loaded from backing store address 0x" 
                  << std::hex << disk_address << std::dec << std::endl;
    } else {
        std::cerr << "[MemoryManager] Warning: Could not read from backing store" << std::endl;
        // Initialize frame with zeros if backing store is unavailable
        physical_memory[frame_number].data.assign(frame_size, 0);
    }
    
    // Update frame information
    physical_memory[frame_number].is_free = false;
    physical_memory[frame_number].process_id = process_id;
    physical_memory[frame_number].page_number = page_number;
    physical_memory[frame_number].is_instruction_page = is_instruction;
    physical_memory[frame_number].last_access_time = std::time(nullptr);
    physical_memory[frame_number].load_time = std::time(nullptr);
    
    // Add to FIFO queue for page replacement
    fifo_queue.push(frame_number);
}

void MemoryManager::page_out(int frame_number) {
    // Write a page from physical memory to backing store
    
    auto& frame = physical_memory[frame_number];
    if (frame.process_id == -1) {
        std::cout << "[MemoryManager] Frame " << frame_number << " is already free" << std::endl;
        return;
    }
    
    std::cout << "[MemoryManager] Paging out: Frame " << frame_number 
              << " (Process " << frame.process_id << ", " 
              << (frame.is_instruction_page ? "Instruction" : "Variable") 
              << " Page " << frame.page_number << ")" << std::endl;
    
    // Get the appropriate page table entry for this frame
    PageTableEntry* pte = nullptr;
    if (frame.is_instruction_page) {
        auto& page_table = instruction_page_tables[frame.process_id];
        pte = &page_table[frame.page_number];
    } else {
        auto& page_table = variable_page_tables[frame.process_id];
        pte = &page_table[frame.page_number];
    }
    
    // Write back to backing store if the page is dirty (has been modified)
    if (pte->dirty) {
        std::ofstream backing_store(backing_store_file, std::ios::binary | std::ios::in);
        if (backing_store.is_open()) {
            backing_store.seekp(pte->disk_address);
            backing_store.write(reinterpret_cast<const char*>(frame.data.data()), frame_size);
            backing_store.close();
            
            std::cout << "[MemoryManager] Dirty page written to backing store address 0x" 
                      << std::hex << pte->disk_address << std::dec << std::endl;
        } else {
            std::cerr << "[MemoryManager] Warning: Could not write to backing store" << std::endl;
        }
    } else {
        std::cout << "[MemoryManager] Page was not dirty, skipping write-back" << std::endl;
    }
    
    // Invalidate the page table entry
    pte->valid = false;
    pte->frame_number = -1;
    pte->dirty = false;
    pte->referenced = false;
    
    // Update statistics
    pages_paged_out++;
    
    // Clear the frame (but don't mark as free yet - caller will do that)
    frame.data.assign(frame_size, 0);
}

// ============================================================================
// STATISTICS AND REPORTING
// ============================================================================

double MemoryManager::get_page_fault_rate() const {
    // Page fault rate = page faults / total memory accesses
    size_t total_accesses = pages_paged_in + page_faults;
    if (total_accesses == 0) {
        return 0.0;
    }
    return static_cast<double>(page_faults) / total_accesses;
}

size_t MemoryManager::get_used_frames() const {
    size_t used = 0;
    for (const auto& frame : physical_memory) {
        if (!frame.is_free) {
            used++;
        }
    }
    return used;
}

size_t MemoryManager::get_free_frames() const {
    return total_frames - get_used_frames();
}

std::string MemoryManager::generate_vmstat_report() {
    std::stringstream report;
    report << "=== Memory Statistics (vmstat) ===\n";
    report << "Total Physical Memory: " << total_memory_size << " bytes\n";
    report << "Frame Size: " << frame_size << " bytes\n";
    report << "Total Frames: " << total_frames << "\n";
    report << "Used Frames: " << get_used_frames() << "\n";
    report << "Free Frames: " << get_free_frames() << "\n";
    report << "Memory Utilization: " << std::fixed << std::setprecision(1) 
           << (static_cast<double>(get_used_frames()) / total_frames) * 100.0 << "%\n";
    report << "Pages Paged In: " << pages_paged_in << "\n";
    report << "Pages Paged Out: " << pages_paged_out << "\n";
    report << "Page Faults: " << page_faults << "\n";
    report << "Page Fault Rate: " << std::fixed << std::setprecision(4) 
           << get_page_fault_rate() * 100.0 << "%\n";
    report << "Page Replacement Algorithm: " << replacement_algorithm << "\n";
    report << "Backing Store: " << backing_store_file << "\n";
    report << "================================\n";
    return report.str();
}

std::string MemoryManager::generate_process_memory_report(int process_id) {
    std::stringstream report;
    
    auto layout_it = process_layouts.find(process_id);
    if (layout_it == process_layouts.end()) {
        report << "Process " << process_id << " not found or has no memory layout.\n";
        return report.str();
    }
    
    const auto& layout = layout_it->second;
    report << "=== Process Memory Layout for Process " << process_id << " ===\n";
    report << "Total Instructions: " << layout.total_instructions << "\n";
    report << "Total Variables: " << layout.total_variables << "\n";
    report << "Instruction Pages: " << layout.instruction_pages << "\n";
    report << "Variable Pages: " << layout.variable_pages << "\n";
    report << "Program Counter: " << layout.program_counter << "\n";
    report << "Variables (32 uint16_t):\n";
    
    for (size_t i = 0; i < layout.variables.size(); ++i) {
        if (i % 8 == 0) report << "  ";
        report << "var" << i << "=" << layout.variables[i];
        if (i % 8 == 7) report << "\n";
        else report << " ";
    }
    if (layout.variables.size() % 8 != 0) report << "\n";
    
    report << "================================\n";
    return report.str();
}

// ============================================================================
// LEGACY METHODS (for backward compatibility)
// ============================================================================

bool MemoryManager::access_memory(int process_id, size_t virtual_address, bool is_write) {
    // Legacy method - now delegates to appropriate access method
    // This is kept for backward compatibility
    return access_instruction(process_id, static_cast<int>(virtual_address / 64), is_write);
}

bool MemoryManager::create_page_table(int process_id, size_t virtual_memory_size) {
    // Legacy method - now uses the new process memory layout system
    // Estimate instruction and variable counts based on memory size
    size_t estimated_instructions = virtual_memory_size / 64; // 64 bytes per instruction
    size_t estimated_variables = 32; // Default 32 variables
    
    return create_process_memory_layout(process_id, estimated_instructions, estimated_variables);
}

void MemoryManager::remove_page_table(int process_id) {
    // Legacy method - now uses the new process memory layout system
    remove_process_memory_layout(process_id);
}

bool MemoryManager::allocate(Process& process, size_t required_size) {
    std::lock_guard<std::mutex> lock(mtx);
    
    // Validate memory size (must be power of 2 between 2^6 and 2^16)
    if (!is_power_of_two(required_size) || required_size < 64 || required_size > 65536) {
        std::cerr << "[MemoryManager] Invalid memory size: " << required_size 
                  << " (must be power of 2 between 64 and 65536)" << std::endl;
        return false;
    }
    
    // Create process memory layout with estimated instruction and variable counts
    size_t estimated_instructions = required_size / 64; // 64 bytes per instruction
    size_t estimated_variables = 32; // Default 32 variables
    
    if (!create_process_memory_layout(process.pid, estimated_instructions, estimated_variables)) {
        return false;
    }
    
    // Use legacy allocation for backward compatibility
    for (auto& block : memory_map) {
        if (block.is_free && block.size >= required_size) {
            size_t original_block_size = block.size;
            size_t original_start_address = block.start_address;
            
            block.is_free = false;
            block.size = required_size;
            block.process_id = process.pid;
            block.process_name = process.name;
            
            process.memory_start_address = block.start_address;
            process.memory_size = required_size;
            
            if (original_block_size > required_size) {
                size_t remaining_size = original_block_size - required_size;
                size_t new_free_block_start = original_start_address + required_size;
                memory_map.push_back({new_free_block_start, remaining_size, true, -1, ""});
            }
            
            std::sort(memory_map.begin(), memory_map.end(), [](const MemoryBlock& a, const MemoryBlock& b) {
                return a.start_address < b.start_address;
            });
            
            std::cout << "[MemoryManager] Allocated " << required_size << " bytes for process " 
                      << process.name << " (PID: " << process.pid << ")" << std::endl;
            return true;
        }
    }
    
    std::cerr << "[MemoryManager] No suitable memory block found for allocation" << std::endl;
    return false;
}

void MemoryManager::deallocate(int process_id) {
    std::lock_guard<std::mutex> lock(mtx);
    
    // Remove process memory layout
    remove_process_memory_layout(process_id);
    
    // Use legacy deallocation for backward compatibility
    for (auto& block : memory_map) {
        if (!block.is_free && block.process_id == process_id) {
            block.is_free = true;
            block.process_id = -1;
            block.process_name = "";
            merge_free_blocks();
            std::cout << "[MemoryManager] Deallocated memory for process " << process_id << std::endl;
            return;
        }
    }
}

void MemoryManager::merge_free_blocks() {
    if (memory_map.size() <= 1) return;
    
    std::sort(memory_map.begin(), memory_map.end(), [](const MemoryBlock& a, const MemoryBlock& b) {
        return a.start_address < b.start_address;
    });
    
    for (size_t i = 0; i < memory_map.size() - 1; ) {
        if (memory_map[i].is_free && memory_map[i + 1].is_free) {
            memory_map[i].size += memory_map[i + 1].size;
            memory_map.erase(memory_map.begin() + i + 1);
        } else {
            i++;
        }
    }
}

size_t MemoryManager::calculate_external_fragmentation() {
    std::lock_guard<std::mutex> lock(mtx);
    size_t total_free_mem = 0;
    
    for (const auto& block : memory_map) {
        if (block.is_free) {
            total_free_mem += block.size;
        }
    }
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

std::string MemoryManager::generate_page_table_report(int process_id) {
    std::stringstream report;
    
    // Check instruction page table
    auto inst_it = instruction_page_tables.find(process_id);
    if (inst_it != instruction_page_tables.end()) {
        const auto& page_table = inst_it->second;
        report << "=== Instruction Page Table for Process " << process_id << " ===\n";
        report << "Total Instruction Pages: " << page_table.size() << "\n";
        report << "Page | Valid | Frame | Dirty | Referenced | Disk Address\n";
        report << "-----|-------|-------|-------|------------|-------------\n";
        
        for (size_t i = 0; i < page_table.size(); ++i) {
            const auto& pte = page_table[i];
            report << std::setw(4) << i << " | ";
            report << (pte.valid ? "  Yes  " : "  No   ") << " | ";
            report << std::setw(5) << (pte.frame_number >= 0 ? std::to_string(pte.frame_number) : "-") << " | ";
            report << (pte.dirty ? "  Yes  " : "  No   ") << " | ";
            report << (pte.referenced ? "    Yes    " : "    No     ") << " | ";
            report << "0x" << std::hex << pte.disk_address << std::dec << "\n";
        }
        report << "\n";
    }
    
    // Check variable page table
    auto var_it = variable_page_tables.find(process_id);
    if (var_it != variable_page_tables.end()) {
        const auto& page_table = var_it->second;
        report << "=== Variable Page Table for Process " << process_id << " ===\n";
        report << "Total Variable Pages: " << page_table.size() << "\n";
        report << "Page | Valid | Frame | Dirty | Referenced | Disk Address\n";
        report << "-----|-------|-------|-------|------------|-------------\n";
        
        for (size_t i = 0; i < page_table.size(); ++i) {
            const auto& pte = page_table[i];
            report << std::setw(4) << i << " | ";
            report << (pte.valid ? "  Yes  " : "  No   ") << " | ";
            report << std::setw(5) << (pte.frame_number >= 0 ? std::to_string(pte.frame_number) : "-") << " | ";
            report << (pte.dirty ? "  Yes  " : "  No   ") << " | ";
            report << (pte.referenced ? "    Yes    " : "    No     ") << " | ";
            report << "0x" << std::hex << pte.disk_address << std::dec << "\n";
        }
    }
    
    if (inst_it == instruction_page_tables.end() && var_it == variable_page_tables.end()) {
        report << "Process " << process_id << " not found or has no page tables.\n";
    }
    
    report << "================================\n";
    return report.str();
}

// ============================================================================
// UTILITY METHODS
// ============================================================================

bool MemoryManager::is_power_of_two(size_t size) {
    return size > 0 && (size & (size - 1)) == 0;
}

size_t MemoryManager::get_next_power_of_two(size_t size) {
    if (size <= 1) return 1;
    
    size_t power = 1;
    while (power < size) {
        power <<= 1;
    }
    return power;
}