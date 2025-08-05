/*
 * Demand Paging Demonstration
 * 
 * This file demonstrates the key features of the demand paging system:
 * 1. Page table creation and management
 * 2. Page fault detection
 * 3. Page replacement algorithms (FIFO and LRU)
 * 4. Backing store operations
 * 
 * To compile and run this demo:
 * g++ -o demo DemandPagingDemo.cpp MemoryManager.cpp -std=c++11
 */

#include "MemoryManager.h"
#include <iostream>
#include <thread>
#include <chrono>

void demonstrate_page_table_creation() {
    std::cout << "\n=== DEMONSTRATION 1: Page Table Creation ===\n";
    
    MemoryManager mm;
    mm.initialize(8192, 1024); // 8KB total memory, 1KB frames (8 frames total)
    
    // Create page table for a process with 4KB virtual memory
    int process_id = 1;
    size_t virtual_memory_size = 4096; // 4KB
    
    std::cout << "Creating page table for process " << process_id 
              << " with " << virtual_memory_size << " bytes virtual memory\n";
    
    if (mm.create_page_table(process_id, virtual_memory_size)) {
        std::cout << "✓ Page table created successfully\n";
        std::cout << "  - Virtual memory size: " << virtual_memory_size << " bytes\n";
        std::cout << "  - Frame size: 1024 bytes\n";
        std::cout << "  - Number of pages: " << (virtual_memory_size + 1023) / 1024 << "\n";
    } else {
        std::cout << "✗ Failed to create page table\n";
    }
}

void demonstrate_page_fault_detection() {
    std::cout << "\n=== DEMONSTRATION 2: Page Fault Detection ===\n";
    
    MemoryManager mm;
    mm.initialize(8192, 1024); // 8KB total memory, 1KB frames
    
    int process_id = 2;
    size_t virtual_memory_size = 2048; // 2KB (2 pages)
    
    mm.create_page_table(process_id, virtual_memory_size);
    
    std::cout << "Testing memory access to trigger page faults:\n";
    
    // Access page 0 (should trigger page fault)
    std::cout << "Accessing page 0 (address 0x0):\n";
    bool success1 = mm.access_memory(process_id, 0, false);
    std::cout << "  Result: " << (success1 ? "✓ Success" : "✗ Failed") << "\n";
    
    // Access page 1 (should trigger another page fault)
    std::cout << "Accessing page 1 (address 0x400):\n";
    bool success2 = mm.access_memory(process_id, 1024, false);
    std::cout << "  Result: " << (success2 ? "✓ Success" : "✗ Failed") << "\n";
    
    // Access invalid address (should fail)
    std::cout << "Accessing invalid address (0x1000):\n";
    bool success3 = mm.access_memory(process_id, 4096, false);
    std::cout << "  Result: " << (success3 ? "✓ Success" : "✗ Failed (expected)") << "\n";
    
    std::cout << "Pages paged in: " << mm.get_pages_paged_in() << "\n";
}

void demonstrate_page_replacement() {
    std::cout << "\n=== DEMONSTRATION 3: Page Replacement (FIFO) ===\n";
    
    MemoryManager mm;
    mm.initialize(3072, 1024); // 3KB total memory, 1KB frames (only 3 frames)
    mm.set_replacement_algorithm("FIFO");
    
    int process_id = 3;
    size_t virtual_memory_size = 4096; // 4KB (4 pages, but only 3 frames available)
    
    mm.create_page_table(process_id, virtual_memory_size);
    
    std::cout << "Testing FIFO page replacement:\n";
    std::cout << "  - Total frames: 3\n";
    std::cout << "  - Virtual memory: 4 pages\n";
    std::cout << "  - Algorithm: FIFO\n\n";
    
    // Access pages 0, 1, 2 (should all fit in memory)
    for (int i = 0; i < 3; i++) {
        std::cout << "Accessing page " << i << ":\n";
        bool success = mm.access_memory(process_id, i * 1024, false);
        std::cout << "  Result: " << (success ? "✓ Success" : "✗ Failed") << "\n";
        std::cout << "  Frames used: " << mm.get_used_frames() << "/" << mm.get_total_frames() << "\n";
    }
    
    // Access page 3 (should trigger page replacement)
    std::cout << "Accessing page 3 (should trigger page replacement):\n";
    bool success = mm.access_memory(process_id, 3 * 1024, false);
    std::cout << "  Result: " << (success ? "✓ Success" : "✗ Failed") << "\n";
    std::cout << "  Frames used: " << mm.get_used_frames() << "/" << mm.get_total_frames() << "\n";
    
    std::cout << "Pages paged in: " << mm.get_pages_paged_in() << "\n";
    std::cout << "Pages paged out: " << mm.get_pages_paged_out() << "\n";
}

void demonstrate_lru_replacement() {
    std::cout << "\n=== DEMONSTRATION 4: Page Replacement (LRU) ===\n";
    
    MemoryManager mm;
    mm.initialize(3072, 1024); // 3KB total memory, 1KB frames (only 3 frames)
    mm.set_replacement_algorithm("LRU");
    
    int process_id = 4;
    size_t virtual_memory_size = 4096; // 4KB (4 pages, but only 3 frames available)
    
    mm.create_page_table(process_id, virtual_memory_size);
    
    std::cout << "Testing LRU page replacement:\n";
    std::cout << "  - Total frames: 3\n";
    std::cout << "  - Virtual memory: 4 pages\n";
    std::cout << "  - Algorithm: LRU\n\n";
    
    // Load pages 0, 1, 2
    for (int i = 0; i < 3; i++) {
        mm.access_memory(process_id, i * 1024, false);
        std::cout << "Loaded page " << i << "\n";
    }
    
    // Access page 0 again (make it recently used)
    std::cout << "Re-accessing page 0 (making it recently used):\n";
    mm.access_memory(process_id, 0, false);
    
    // Access page 3 (should evict page 1, not page 0)
    std::cout << "Accessing page 3 (should evict least recently used page):\n";
    bool success = mm.access_memory(process_id, 3 * 1024, false);
    std::cout << "  Result: " << (success ? "✓ Success" : "✗ Failed") << "\n";
    
    std::cout << "Pages paged in: " << mm.get_pages_paged_in() << "\n";
    std::cout << "Pages paged out: " << mm.get_pages_paged_out() << "\n";
}

void demonstrate_backing_store_operations() {
    std::cout << "\n=== DEMONSTRATION 5: Backing Store Operations ===\n";
    
    MemoryManager mm;
    mm.initialize(2048, 1024); // 2KB total memory, 1KB frames (only 2 frames)
    
    int process_id = 5;
    size_t virtual_memory_size = 3072; // 3KB (3 pages, but only 2 frames available)
    
    mm.create_page_table(process_id, virtual_memory_size);
    
    std::cout << "Testing backing store operations:\n";
    std::cout << "  - Total frames: 2\n";
    std::cout << "  - Virtual memory: 3 pages\n";
    std::cout << "  - Backing store: csopesy-backing-store.txt\n\n";
    
    // Load pages 0 and 1
    std::cout << "Loading pages 0 and 1:\n";
    mm.access_memory(process_id, 0, false);
    mm.access_memory(process_id, 1024, false);
    
    // Write to page 0 (mark as dirty)
    std::cout << "Writing to page 0 (marking as dirty):\n";
    mm.access_memory(process_id, 0, true);
    
    // Access page 2 (should trigger page replacement)
    std::cout << "Accessing page 2 (should trigger page replacement):\n";
    mm.access_memory(process_id, 2048, false);
    
    std::cout << "Pages paged in: " << mm.get_pages_paged_in() << "\n";
    std::cout << "Pages paged out: " << mm.get_pages_paged_out() << "\n";
    std::cout << "Note: Dirty pages are written back to backing store\n";
}

int main() {
    std::cout << "CSOPESY Demand Paging System Demonstration\n";
    std::cout << "==========================================\n";
    
    try {
        demonstrate_page_table_creation();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        demonstrate_page_fault_detection();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        demonstrate_page_replacement();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        demonstrate_lru_replacement();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        demonstrate_backing_store_operations();
        
        std::cout << "\n=== DEMONSTRATION COMPLETE ===\n";
        std::cout << "All demand paging features have been demonstrated.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error during demonstration: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 