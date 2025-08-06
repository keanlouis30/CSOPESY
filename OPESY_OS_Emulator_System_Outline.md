# OPESY OS Emulator - System Outline

## Overview

The OPESY OS Emulator is a sophisticated C++23-based simulation of a multitasking operating system that demonstrates key OS concepts including demand paging, process scheduling, memory management, and instruction execution. The system operates as a console-based emulator with a modular architecture designed to simulate real-world OS behaviors.

## Core Architecture

The system follows a **layered architecture** with distinct components:

### 1. Console Manager (Singleton Pattern)
The `ConsoleManager` serves as the central coordinator, implementing the singleton pattern to ensure a single point of control for the entire system.

**Key Responsibilities:**
- Screen management and transitions
- Process creation and lifecycle management
- User input parsing and command delegation
- System initialization and shutdown

**Design Rationale:**
The singleton pattern ensures consistent state management across the entire system, preventing multiple instances from conflicting with each other. This is particularly important for memory management and process scheduling coordination.

### 2. Memory Management Architecture

#### PagingAllocator (Singleton)
The paging system implements a sophisticated demand paging strategy with the following components:

**Frame Management:**
```cpp
struct FrameInfo {
    int pid = -1;
    int pageNumber = -1;
    std::vector<std::optional<StoredData>> data;
    bool isPinned = false;
};
```

**Page Fault Handling:**
1. **Detection**: Memory access triggers page fault
2. **Validation**: Check if page exists in backing store
3. **Allocation**: Find or evict frame for allocation
4. **Loading**: Load page data from memory or backing store
5. **Mapping**: Update page table with frame number

**Eviction Strategy:**
- **FIFO Queue**: Maintains order of frame allocation
- **Pinned Protection**: Critical frames cannot be evicted
- **Dirty Bit Handling**: Modified pages written to backing store
- **Retry Logic**: Multiple eviction attempts if needed

#### Backing Store Implementation
The backing store uses a simple file-based approach (`csopesy-backing-store.txt`) with the following format:
```
<pid> <page_number>
<serialized_data>
```

This design choice prioritizes simplicity and reliability over performance, making it suitable for educational purposes while demonstrating real-world concepts.

### 3. Process Management System

#### Process States and Transitions
```cpp
enum ProcessStatus { READY, RUNNING, WAITING, DONE };
```

**State Transitions:**
- **READY → RUNNING**: Process scheduled on CPU core
- **RUNNING → WAITING**: Process sleeps or blocks
- **WAITING → READY**: Process wakes up
- **RUNNING → DONE**: Process completes execution

#### Memory Segmentation
The system implements a simplified memory segmentation model:
```cpp
enum MemorySegment { TEXT, DATA, HEAP };
```

**Segment Management:**
- **TEXT**: Instruction storage with precomputed page mapping
- **DATA**: Variable storage with address translation
- **HEAP**: Dynamic memory allocation with bounds checking

### 4. CPU Scheduling Architecture

#### Multi-Core Execution Model
The scheduler implements a **thread-per-core** model where each CPU core runs in its own thread, synchronized through barriers and mutexes.

**Key Components:**
- **Worker Threads**: One per CPU core
- **Tick Synchronization**: Barrier-based cycle coordination
- **Queue Management**: Thread-safe ready and wait queues
- **Core Assignment**: Dynamic process-to-core mapping

#### Scheduling Algorithm Implementation

**Round-Robin (RR):**
```cpp
void executeRR(const std::shared_ptr<Process>& proc, uint64_t& lastTickSeen) {
    uint64_t quantum = Config::getInstance().getQuantumCycles();
    // Execute for quantum cycles or until process blocks
}
```

**First-Come-First-Served (FCFS):**
```cpp
void executeFCFS(const std::shared_ptr<Process>& proc, uint64_t& lastTickSeen) {
    // Execute until process blocks or completes
}
```

### 5. Instruction System Design

#### Factory Pattern Implementation
The `InstructionFactory` uses the factory pattern to create instructions dynamically:

```cpp
static std::shared_ptr<Instruction> parseInstructionString(
    const std::string& instrStr, int processID);
```

**Instruction Types:**
- **ArithmeticInstruction**: Basic math operations
- **PrintInstruction**: Output operations
- **ReadInstruction**: Memory read operations
- **WriteInstruction**: Memory write operations
- **DeclareInstruction**: Variable declarations
- **SleepInstruction**: Process suspension
- **ForInstruction**: Loop constructs

#### Instruction Execution Model
Each instruction implements a consistent interface:
```cpp
virtual void execute() = 0;
virtual std::string serialize() const = 0;
virtual bool isComplete() const { return true; }
```

**Execution Flow:**
1. **Validation**: Check instruction parameters
2. **Memory Access**: Handle page faults if needed
3. **Execution**: Perform instruction operation
4. **State Update**: Update process state and logs
5. **Completion**: Mark instruction as complete

## Design Philosophy

The emulator implements **demand paging** as its core memory management strategy, requiring processes to explicitly request memory pages before they can be accessed. This creates realistic memory pressure and demonstrates page fault handling, eviction policies, and backing store management.

### Key Design Principles:

1. **Educational Focus**: Prioritize clarity and learning over performance
2. **Realistic Behavior**: Simulate actual OS challenges and constraints
3. **Modularity**: Separate concerns for maintainability and extensibility
4. **Thread Safety**: Proper synchronization for multi-threaded operations
5. **Error Handling**: Graceful degradation and informative error messages

### Memory Management Strategy

The demand paging system creates authentic memory pressure scenarios by:
- **Limiting Physical Memory**: Configurable total system memory
- **Process Memory Constraints**: Minimum and maximum per-process limits
- **Page Fault Simulation**: Real page faults requiring frame allocation
- **Eviction Pressure**: FIFO-based eviction when memory is full
- **Backing Store**: File-based storage for swapped pages

This approach demonstrates real-world OS challenges where:
- Memory is a scarce resource
- Processes compete for limited physical frames
- Page faults impact performance
- Eviction policies affect system behavior
- Backing store I/O creates additional overhead

### Process Scheduling Philosophy

The multi-core scheduler demonstrates:
- **Parallel Execution**: Multiple processes running simultaneously
- **Resource Contention**: Processes competing for CPU cores
- **Scheduling Algorithms**: Different strategies for process selection
- **Context Switching**: Process state management and transitions
- **Performance Metrics**: CPU utilization and efficiency tracking

### Instruction System Philosophy

The instruction system emphasizes:
- **Memory Access Patterns**: Realistic memory usage scenarios
- **Page Fault Triggers**: Instructions that cause memory access
- **Variable Management**: Dynamic memory allocation and access
- **Control Flow**: Loops and conditional execution
- **I/O Operations**: Print and input/output simulation

## System Flow

### Initialization Flow
1. **Configuration Loading**: Parse config.txt for system parameters
2. **Memory Initialization**: Set up frame table and backing store
3. **Scheduler Setup**: Create worker threads and synchronization primitives
4. **Console Initialization**: Set up main screen and command interface
5. **System Ready**: Accept user commands and process creation

### Process Creation Flow
1. **Command Parsing**: Parse user input for process parameters
2. **Memory Validation**: Check memory size constraints
3. **Process Creation**: Allocate process ID and initial memory
4. **Instruction Generation**: Create or parse process instructions
5. **Scheduling**: Add process to ready queue for execution

### Memory Access Flow
1. **Address Calculation**: Convert virtual address to page number
2. **Page Table Lookup**: Check if page is present in memory
3. **Page Fault Handling**: If page not present, trigger fault handler
4. **Frame Allocation**: Find or evict frame for page
5. **Data Access**: Read or write data from allocated frame

### Execution Flow
1. **Process Selection**: Scheduler selects next process from ready queue
2. **Core Assignment**: Assign process to available CPU core
3. **Instruction Execution**: Execute next instruction in process
4. **State Update**: Update process state and progress
5. **Context Switch**: Switch to next process when quantum expires

This comprehensive system design provides a realistic simulation of operating system concepts while maintaining educational clarity and extensibility for further development. 