/*
 * Separate Instruction and Variable Pages Demonstration
 * 
 * This file demonstrates the key features of the separate page system:
 * 1. Separate instruction and variable page tables
 * 2. Program counter for instruction access
 * 3. Variable tracking (32 uint16_t variables)
 * 4. Page fault rate calculation
 * 5. Three-table system (Page table, Frames, Backing store)
 * 
 * To compile and run this demo:
 * g++ -o demo SeparatePagesDemo.cpp MemoryManager.cpp -std=c++11
 */

#include "MemoryManager.h"
#include <iostream>
#include <thread>
#include <chrono>

void demonstrate_separate_page_tables() {
    std::cout << "\n=== DEMONSTRATION 1: Separate Page Tables ===\n";
    
    MemoryManager mm;
    mm.initialize(8192, 1024); // 8KB total memory, 1KB frames (8 frames total)
    
    // Create process with separate instruction and variable pages
    int process_id = 1;
    size_t instruction_count = 100; // 100 instructions
    size_t variable_count = 32;     // 32 variables (uint16_t)
    
    std::cout << "Creating memory layout for process " << process_id << "\n";
    std::cout << "  Instructions: " << instruction_count << "\n";
    std::cout << "  Variables: " << variable_count << "\n";
    
    if (mm.create_process_memory_layout(process_id, instruction_count, variable_count)) {
        std::cout << "✓ Memory layout created successfully\n";
        
        // Show process memory layout
        std::cout << "\nProcess Memory Layout:\n";
        std::cout << mm.generate_process_memory_report(process_id);
        
        // Show separate page tables
        std::cout << "\nSeparate Page Tables:\n";
        std::cout << mm.generate_page_table_report(process_id);
    } else {
        std::cout << "✗ Failed to create memory layout\n";
    }
}

void demonstrate_program_counter_access() {
    std::cout << "\n=== DEMONSTRATION 2: Program Counter Access ===\n";
    
    MemoryManager mm;
    mm.initialize(4096, 1024); // 4KB total memory, 1KB frames (4 frames)
    
    int process_id = 2;
    size_t instruction_count = 50;  // 50 instructions
    size_t variable_count = 32;     // 32 variables
    
    mm.create_process_memory_layout(process_id, instruction_count, variable_count);
    
    std::cout << "Testing program counter-based instruction access:\n";
    
    // Simulate program execution with program counter
    for (int pc = 0; pc < 10; pc++) {
        std::cout << "Program Counter: " << pc << " - ";
        bool success = mm.access_instruction(process_id, pc, false);
        std::cout << (success ? "✓ Instruction accessed" : "✗ Access failed") << "\n";
    }
    
    std::cout << "Page faults occurred: " << mm.get_page_faults() << "\n";
    std::cout << "Page fault rate: " << std::fixed << std::setprecision(4) 
              << mm.get_page_fault_rate() * 100.0 << "%\n";
}

void demonstrate_variable_tracking() {
    std::cout << "\n=== DEMONSTRATION 3: Variable Tracking ===\n";
    
    MemoryManager mm;
    mm.initialize(4096, 1024); // 4KB total memory, 1KB frames (4 frames)
    
    int process_id = 3;
    size_t instruction_count = 30;  // 30 instructions
    size_t variable_count = 32;     // 32 variables (uint16_t)
    
    mm.create_process_memory_layout(process_id, instruction_count, variable_count);
    
    std::cout << "Testing variable access and tracking:\n";
    
    // Access different variables
    for (int var_index = 0; var_index < 8; var_index++) {
        std::cout << "Accessing variable " << var_index << ": ";
        bool success = mm.access_variable(process_id, var_index, true); // Write access
        std::cout << (success ? "✓ Variable accessed" : "✗ Access failed") << "\n";
    }
    
    // Show process memory layout with variables
    std::cout << "\nProcess Memory Layout (showing variables):\n";
    std::cout << mm.generate_process_memory_report(process_id);
}

void demonstrate_page_fault_rate() {
    std::cout << "\n=== DEMONSTRATION 4: Page Fault Rate Calculation ===\n";
    
    MemoryManager mm;
    mm.initialize(2048, 1024); // 2KB total memory, 1KB frames (only 2 frames)
    
    int process_id = 4;
    size_t instruction_count = 100; // 100 instructions (will cause page replacement)
    size_t variable_count = 32;     // 32 variables
    
    mm.create_process_memory_layout(process_id, instruction_count, variable_count);
    
    std::cout << "Testing page fault rate with limited memory:\n";
    std::cout << "  Total frames: 2\n";
    std::cout << "  Instructions: 100 (will cause page replacement)\n";
    std::cout << "  Algorithm: FIFO (safest)\n\n";
    
    // Access many instructions to trigger page replacement
    for (int i = 0; i < 20; i++) {
        mm.access_instruction(process_id, i, false);
    }
    
    // Access some variables
    for (int i = 0; i < 5; i++) {
        mm.access_variable(process_id, i, true);
    }
    
    std::cout << "Final Statistics:\n";
    std::cout << "  Pages paged in: " << mm.get_pages_paged_in() << "\n";
    std::cout << "  Pages paged out: " << mm.get_pages_paged_out() << "\n";
    std::cout << "  Page faults: " << mm.get_page_faults() << "\n";
    std::cout << "  Page fault rate: " << std::fixed << std::setprecision(4) 
              << mm.get_page_fault_rate() * 100.0 << "%\n";
    
    std::cout << "\nNote: Page fault rate is crucial for evaluation!\n";
}

void demonstrate_three_table_system() {
    std::cout << "\n=== DEMONSTRATION 5: Three-Table System ===\n";
    
    MemoryManager mm;
    mm.initialize(3072, 1024); // 3KB total memory, 1KB frames (3 frames)
    
    int process_id = 5;
    size_t instruction_count = 50;  // 50 instructions
    size_t variable_count = 32;     // 32 variables
    
    mm.create_process_memory_layout(process_id, instruction_count, variable_count);
    
    std::cout << "Demonstrating the three-table system:\n";
    std::cout << "1. Page Tables (Instruction + Variable)\n";
    std::cout << "2. Physical Frames\n";
    std::cout << "3. Backing Store\n\n";
    
    // Load some pages to show the system in action
    std::cout << "Loading pages into memory:\n";
    for (int i = 0; i < 3; i++) {
        mm.access_instruction(process_id, i, false);
        std::cout << "  Loaded instruction page " << i << "\n";
    }
    
    // Access a variable to show variable page loading
    mm.access_variable(process_id, 0, true);
    std::cout << "  Loaded variable page 0\n";
    
    // Show current state
    std::cout << "\nCurrent System State:\n";
    std::cout << "  Used frames: " << mm.get_used_frames() << "/" << mm.get_total_frames() << "\n";
    std::cout << "  Pages in memory: " << mm.get_pages_paged_in() << "\n";
    std::cout << "  Pages in backing store: " << (instruction_count + variable_count - mm.get_used_frames()) << "\n";
    
    // Show page tables
    std::cout << "\nPage Tables:\n";
    std::cout << mm.generate_page_table_report(process_id);
}

void demonstrate_quantum_behavior() {
    std::cout << "\n=== DEMONSTRATION 6: Quantum Behavior ===\n";
    
    MemoryManager mm;
    mm.initialize(2048, 1024); // 2KB total memory, 1KB frames (2 frames)
    
    int process_id = 6;
    size_t instruction_count = 100; // 100 instructions
    size_t variable_count = 32;     // 32 variables
    
    mm.create_process_memory_layout(process_id, instruction_count, variable_count);
    
    std::cout << "Demonstrating quantum behavior (only first instruction in memory):\n";
    std::cout << "  Total frames: 2\n";
    std::cout << "  Instructions: 100\n";
    std::cout << "  Quantum = number of lines in memory\n\n";
    
    // Simulate quantum execution
    int quantum = 2; // Only 2 instructions can be in memory at once
    
    for (int quantum_cycle = 0; quantum_cycle < 3; quantum_cycle++) {
        std::cout << "Quantum Cycle " << (quantum_cycle + 1) << ":\n";
        
        for (int i = 0; i < quantum; i++) {
            int instruction_index = quantum_cycle * quantum + i;
            if (instruction_index < static_cast<int>(instruction_count)) {
                bool success = mm.access_instruction(process_id, instruction_index, false);
                std::cout << "  Executing instruction " << instruction_index 
                          << ": " << (success ? "✓" : "✗") << "\n";
            }
        }
        
        std::cout << "  Frames used: " << mm.get_used_frames() << "/" << mm.get_total_frames() << "\n";
        std::cout << "  Page faults this cycle: " << mm.get_page_faults() << "\n\n";
    }
    
    std::cout << "Final page fault rate: " << std::fixed << std::setprecision(4) 
              << mm.get_page_fault_rate() * 100.0 << "%\n";
}

int main() {
    std::cout << "CSOPESY Separate Instruction and Variable Pages System\n";
    std::cout << "=====================================================\n";
    std::cout << "Based on Professor's Requirements:\n";
    std::cout << "- Separate instruction and variable page arrays\n";
    std::cout << "- Program counter for access\n";
    std::cout << "- Variable tracking (32 uint16_t)\n";
    std::cout << "- Three-table system\n";
    std::cout << "- Page fault rate calculation\n";
    std::cout << "- FIFO as safest algorithm\n";
    std::cout << "=====================================================\n";
    
    try {
        demonstrate_separate_page_tables();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        demonstrate_program_counter_access();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        demonstrate_variable_tracking();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        demonstrate_page_fault_rate();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        demonstrate_three_table_system();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        demonstrate_quantum_behavior();
        
        std::cout << "\n=== DEMONSTRATION COMPLETE ===\n";
        std::cout << "All separate page system features have been demonstrated.\n";
        std::cout << "Key metrics for evaluation:\n";
        std::cout << "- Page fault rate\n";
        std::cout << "- Memory utilization\n";
        std::cout << "- Page replacement efficiency\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error during demonstration: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 