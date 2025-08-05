# CSOPESY Demand Paging System

This document explains the implementation of the demand paging system in the CSOPESY emulator.

## Overview

The demand paging system implements virtual memory management with the following key features:

1. **Page Tables**: Each process has its own page table that maps virtual addresses to physical frames
2. **Page Fault Detection**: Automatically detects when a page is not in physical memory
3. **Page Replacement**: Uses FIFO or LRU algorithms to select victim frames when physical memory is full
4. **Backing Store**: Swaps pages between physical memory and disk storage

## Key Components

### 1. Page Table Entry (PTE)
```cpp
struct PageTableEntry {
    bool valid;           // Is page in physical memory?
    int frame_number;     // Physical frame number (-1 if not in memory)
    bool dirty;           // Has page been modified?
    bool referenced;      // Has page been accessed recently? (for LRU)
    size_t disk_address;  // Address in backing store
    std::time_t last_access_time; // Timestamp of last access
};
```

### 2. Physical Frame
```cpp
struct PhysicalFrame {
    bool is_free;         // Is frame available?
    int process_id;       // Which process owns this frame
    int page_number;      // Which page within the process
    std::vector<uint8_t> data; // Actual data
    std::time_t last_access_time; // Last access time
    std::time_t load_time; // When frame was loaded (for FIFO)
};
```

## How It Works

### 1. Memory Access Flow
```
Process requests memory access
         ↓
Check page table entry
         ↓
Is page valid (in physical memory)?
    ├─ Yes → Access granted
    └─ No  → PAGE FAULT
         ↓
Find free frame or select victim
         ↓
Page in from backing store
         ↓
Update page table entry
         ↓
Access granted
```

### 2. Page Fault Handling
When a process tries to access a page that's not in physical memory:

1. **Detection**: The `access_memory()` method detects the page fault
2. **Frame Selection**: Either find a free frame or select a victim frame
3. **Page Replacement**: If no free frames, evict a page using FIFO or LRU
4. **Page In**: Load the required page from backing store
5. **Update Tables**: Mark the page as valid and update access information

### 3. Page Replacement Algorithms

#### FIFO (First In, First Out)
- Selects the frame that was loaded first (oldest load time)
- Simple but may not be optimal
- Good for demonstration purposes

#### LRU (Least Recently Used)
- Selects the frame that was accessed least recently
- More complex but generally better performance
- Tracks access times for each frame

## Usage

### 1. Basic Commands

#### `process-smi`
Shows memory and process information:
```
=== Process and Memory Information ===
Memory Summary:
  Total Memory: 16384 bytes
  Used Memory: 8192 bytes
  Free Memory: 8192 bytes
  Utilization: 50.0%

Running Processes:
  process1 (PID: 1)
    Memory: 1024 bytes
    Status: Running
    Core: 0
```

#### `vmstat`
Shows detailed memory statistics:
```
=== Memory Statistics (vmstat) ===
Total Physical Memory: 16384 bytes
Frame Size: 1024 bytes
Total Frames: 16
Used Frames: 8
Free Frames: 8
Memory Utilization: 50.0%
Pages Paged In: 12
Pages Paged Out: 4
Page Replacement Algorithm: FIFO
Backing Store: csopesy-backing-store.txt
```

#### `test-paging`
Tests the demand paging system:
```
=== Testing Demand Paging System ===
Created page table for test process 999
Testing memory access (should trigger page faults):
  Access to address 0x0 successful (page 0)
  Access to address 0x400 successful (page 1)
Testing write access:
  Write to address 0x0: successful

Paging Statistics:
  Pages paged in: 2
  Pages paged out: 0
Test completed.
```

### 2. Configuration

The system uses the following configuration parameters from `config.txt`:

- `max_overall_mem`: Total physical memory size
- `mem_per_frame`: Size of each frame
- `mem_per_proc`: Default memory per process

### 3. Backing Store

The system creates a backing store file `csopesy-backing-store.txt` that:
- Stores pages when they're evicted from physical memory
- Is 100MB in size to accommodate multiple processes
- Handles both read and write operations

## Implementation Details

### Key Methods

#### `access_memory(process_id, virtual_address, is_write)`
- Main method for memory access
- Detects page faults
- Triggers page replacement when needed
- Returns true if access is successful

#### `create_page_table(process_id, virtual_memory_size)`
- Creates a page table for a new process
- Allocates virtual memory space
- Initializes all page table entries as invalid

#### `page_in(process_id, page_number, frame_number)`
- Loads a page from backing store into physical memory
- Updates frame information
- Adds frame to FIFO queue

#### `page_out(frame_number)`
- Writes a page from physical memory to backing store
- Only writes if page is dirty (modified)
- Updates page table entry

### Thread Safety

All memory management operations are protected by mutex locks to ensure thread safety in a multi-threaded environment.

## Testing

### Running the Demo
```bash
g++ -o demo DemandPagingDemo.cpp MemoryManager.cpp -std=c++11
./demo
```

### Demo Features
1. **Page Table Creation**: Shows how page tables are created
2. **Page Fault Detection**: Demonstrates page fault detection
3. **FIFO Replacement**: Shows FIFO page replacement algorithm
4. **LRU Replacement**: Shows LRU page replacement algorithm
5. **Backing Store Operations**: Demonstrates disk I/O operations

## Performance Considerations

1. **Frame Size**: Smaller frames reduce internal fragmentation but increase page table size
2. **Replacement Algorithm**: LRU generally performs better than FIFO but requires more overhead
3. **Backing Store**: Disk I/O is slow, so minimizing page faults is important
4. **Memory Pressure**: High memory utilization increases page replacement frequency

## Error Handling

The system handles various error conditions:
- Invalid memory addresses
- Missing page tables
- Backing store I/O errors
- Insufficient physical memory

All errors are logged with descriptive messages to help with debugging.

## Future Enhancements

Potential improvements to the demand paging system:
1. **Working Set Model**: Track recently used pages more efficiently
2. **Clock Algorithm**: Implement the clock page replacement algorithm
3. **Memory Compression**: Compress pages to reduce backing store usage
4. **Prefetching**: Predict and load pages before they're needed
5. **NUMA Support**: Handle non-uniform memory access architectures 