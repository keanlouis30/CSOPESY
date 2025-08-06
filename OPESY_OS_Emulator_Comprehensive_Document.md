# OPESY OS Emulator - Comprehensive System Documentation

## Table of Contents
1. [System Outline](#system-outline)
2. [Requirements](#requirements)
3. [Architecture Deep Dive](#architecture-deep-dive)
4. [Memory Management](#memory-management)
5. [Process Management](#process-management)
6. [CPU Scheduling](#cpu-scheduling)
7. [Instruction System](#instruction-system)
8. [User Interface](#user-interface)
9. [Configuration System](#configuration-system)
10. [Debugging and Monitoring](#debugging-and-monitoring)

---

## System Outline

### Overview
The OPESY OS Emulator is a sophisticated C++23-based simulation of a multitasking operating system that demonstrates key OS concepts including demand paging, process scheduling, memory management, and instruction execution. The system operates as a console-based emulator with a modular architecture designed to simulate real-world OS behaviors.

### Core Architecture
The system follows a **layered architecture** with distinct components:

1. **Console Manager** - Singleton pattern managing user interface and screen transitions
2. **Process Scheduler** - Multi-threaded CPU scheduler with configurable algorithms
3. **Memory Management** - Demand paging with backing store and frame allocation
4. **Instruction System** - Extensible instruction set with factory pattern
5. **Configuration System** - Runtime configurable parameters via config.txt

### Design Philosophy
The emulator implements **demand paging** as its core memory management strategy, requiring processes to explicitly request memory pages before they can be accessed. This creates realistic memory pressure and demonstrates page fault handling, eviction policies, and backing store management.

### Key Design Principles:
1. **Educational Focus**: Prioritize clarity and learning over performance
2. **Realistic Behavior**: Simulate actual OS challenges and constraints
3. **Modularity**: Separate concerns for maintainability and extensibility
4. **Thread Safety**: Proper synchronization for multi-threaded operations
5. **Error Handling**: Graceful degradation and informative error messages

---

## Requirements

### 1. Memory Management Requirements

#### Demand Paging System
- **Page Size**: Configurable via `mem-per-frame` (default: 256 bytes)
- **Total Memory**: Configurable via `max-overall-mem` (default: 32KB)
- **Process Memory Limits**: `min-mem-per-proc` to `max-mem-per-proc` (64-64 bytes default)
- **Page Fault Handling**: Automatic frame allocation with eviction when necessary
- **Backing Store**: File-based (`csopesy-backing-store.txt`) for swapped pages
- **Memory Constraints**: Processes cannot exceed allocated memory limits

#### Memory Allocation Strategy
- **Frame Allocation**: First-fit algorithm with victim frame selection
- **Eviction Policy**: FIFO-based with pinned frame protection
- **Memory Visualization**: Real-time frame map display
- **Memory Statistics**: Usage tracking and reporting

### 2. Process Creation and Management

#### Process Creation Commands
- **`screen -s <name> <mem_size>`**: Create process with specified memory
- **`screen -c <name> <mem_size> "<instructions>"`**: Create with custom instructions
- **`screen -r <name>`**: Resume existing process
- **`screen -ls`**: List all processes

#### Process Lifecycle
- **States**: READY, RUNNING, WAITING, DONE
- **Memory Allocation**: Automatic based on instruction count
- **Core Assignment**: Dynamic CPU core allocation
- **Termination**: Automatic cleanup of memory and resources

### 3. Memory Access Operations

#### Instruction Set Support
- **READ**: Memory read operations with page fault handling
- **WRITE**: Memory write operations with dirty bit management
- **DECLARE**: Variable declaration and memory allocation
- **ARITHMETIC**: Basic arithmetic operations
- **PRINT**: Output operations
- **SLEEP**: Process suspension with wakeup scheduling
- **FOR**: Loop constructs with nested instruction support

#### Memory Access Patterns
- **Demand Loading**: Pages loaded only when accessed
- **Dirty Bit Tracking**: Modified pages marked for write-back
- **Address Validation**: Heap address bounds checking
- **Memory Segments**: TEXT, DATA, HEAP separation

### 4. Debugging and Visualization Commands

#### System Monitoring
- **`process-smi`**: Process status and memory information
- **`vmstat`**: Virtual memory statistics
- **`visualize`**: Real-time memory frame visualization
- **`report-util`**: CPU utilization report

#### Debugging Features
- **Process Logging**: Per-process execution logs
- **Memory Tracking**: Frame allocation/deallocation logging
- **Performance Metrics**: CPU ticks, memory usage, page faults
- **Error Handling**: Graceful shutdown with reason reporting

### 5. Configuration System

#### Configuration File (config.txt)
```
num-cpu 8                    # Number of CPU cores
scheduler "rr"               # Round-robin scheduling
quantum-cycles 4             # Time quantum for RR
batch-process-freq 1         # Dummy process generation frequency
min-ins 1000                # Minimum instructions per process
max-ins 2000                # Maximum instructions per process
delays-per-exec 0           # Execution delays
max-overall-mem 32768       # Total system memory (bytes)
mem-per-frame 256           # Page size (bytes)
min-mem-per-proc 64         # Minimum process memory
max-mem-per-proc 64         # Maximum process memory
```

#### Runtime Configuration
- **Dynamic Loading**: Configuration loaded at startup
- **Validation**: Parameter bounds checking
- **Defaults**: Sensible defaults for missing parameters
- **Hot Reload**: Some parameters can be changed at runtime

### 6. Scheduler and CPU Execution

#### Scheduling Algorithms
- **FCFS (First-Come-First-Served)**: Non-preemptive
- **RR (Round-Robin)**: Preemptive with configurable quantum
- **Multi-Core Support**: Parallel execution across cores
- **Context Switching**: Automatic process state management

#### CPU Execution Model
- **Tick-Based**: Synchronized execution cycles
- **Core Assignment**: Dynamic process-to-core mapping
- **Quantum Management**: Time slice enforcement
- **Idle Detection**: CPU utilization tracking

---

## Architecture Deep Dive

### Console Manager (Singleton Pattern)
The `ConsoleManager` serves as the central coordinator, implementing the singleton pattern to ensure a single point of control for the entire system.

**Key Responsibilities:**
- Screen management and transitions
- Process creation and lifecycle management
- User input parsing and command delegation
- System initialization and shutdown

**Design Rationale:**
The singleton pattern ensures consistent state management across the entire system, preventing multiple instances from conflicting with each other. This is particularly important for memory management and process scheduling coordination.

### Memory Management Architecture

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

### Process Management System

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

### CPU Scheduling Architecture

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

### Instruction System Design

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

---

## Memory Management

### Demand Paging Implementation

#### Page Table Structure
```cpp
struct PageEntry {
    int frameNumber = -1;
    bool isValid = false;
    bool inBackingStore = false;
    bool isDirty = false;
};
```

**Page Table Management:**
- **Virtual-to-Physical Mapping**: Page number to frame number
- **Validity Tracking**: Present/absent bit simulation
- **Dirty Bit**: Modified page tracking
- **Backing Store Flag**: Swapped page indication

#### Page Fault Handling Algorithm
```cpp
PageFaultResult handlePageFault(int pid, int pageNumber) {
    // 1. Load page data
    // 2. Attempt frame allocation
    // 3. Evict victim frame if needed
    // 4. Load page into frame
    // 5. Update page table
}
```

**Fault Resolution Steps:**
1. **Data Retrieval**: Get page data from memory or backing store
2. **Frame Allocation**: Find available frame or evict victim
3. **Page Loading**: Copy data into allocated frame
4. **Table Update**: Update page table with frame mapping
5. **Statistics Update**: Increment page fault counters

### Memory Allocation Constraints

#### Process Memory Limits
- **Minimum**: 64 bytes per process (configurable)
- **Maximum**: 64 bytes per process (configurable)
- **Total System**: 32KB total memory (configurable)
- **Frame Size**: 256 bytes per frame (configurable)

#### Memory Validation
```cpp
bool validateMemorySize(int memSize) const {
    return memSize >= minMemPerProc && 
           memSize <= maxMemPerProc &&
           isPowerOfTwo(memSize);
}
```

### Backing Store Management

#### File-Based Storage
The backing store uses a simple text file format for reliability and debugging:

**File Format:**
```
<process_id> <page_number>
<serialized_instruction_data>
<serialized_variable_data>
```

**Operations:**
- **Swap Out**: Write page data to backing store
- **Swap In**: Read page data from backing store
- **Cleanup**: Remove process pages on termination

---

## Process Management

### Process Creation and Lifecycle

#### Creation Commands
```bash
# Create process with memory allocation
screen -s process1 64

# Create process with custom instructions
screen -c process2 64 "DECLARE x 5; PRINT x; SLEEP 10"

# Resume existing process
screen -r process1

# List all processes
screen -ls
```

#### Process State Management
```cpp
class Process {
    std::atomic<ProcessStatus> status;
    std::atomic<int> currentCore;
    uint64_t wakeupTick;
    // ... other members
};
```

**State Transitions:**
- **Creation**: Process created with initial memory allocation
- **Scheduling**: Process moved to ready queue
- **Execution**: Process assigned to CPU core
- **Blocking**: Process moved to wait queue
- **Completion**: Process marked as done and cleaned up

### Memory Segmentation

#### Segment Boundaries
```cpp
std::unordered_map<MemorySegment, uint16_t> segmentBoundaries;
```

**Segment Layout:**
- **TEXT**: Instructions (0 to text_boundary)
- **DATA**: Variables (text_boundary to data_boundary)
- **HEAP**: Dynamic allocation (data_boundary to heap_boundary)

#### Address Translation
```cpp
static std::pair<int, int> splitAddress(int address);
```

**Translation Process:**
1. **Segment Identification**: Determine which segment contains address
2. **Page Calculation**: Convert address to page number
3. **Offset Calculation**: Determine offset within page
4. **Frame Lookup**: Find physical frame for page
5. **Physical Address**: Combine frame number and offset

---

## CPU Scheduling

### Multi-Core Architecture

#### Thread Model
```cpp
class ProcessScheduler {
    std::vector<std::thread> cpuWorkers;
    std::unique_ptr<std::barrier<std::function<void()>>> tickBarrier;
    // ... other members
};
```

**Core Components:**
- **Worker Threads**: One thread per CPU core
- **Tick Synchronization**: Barrier-based cycle coordination
- **Queue Management**: Thread-safe process queues
- **Core Assignment**: Dynamic process-to-core mapping

#### Scheduling Algorithms

**Round-Robin (RR):**
```cpp
void executeRR(const std::shared_ptr<Process>& proc, uint64_t& lastTickSeen) {
    uint64_t quantum = Config::getInstance().getQuantumCycles();
    uint64_t startTick = totalCPUTicks;
    
    while (totalCPUTicks - startTick < quantum && !proc->getIsFinished()) {
        // Execute one instruction
        // Check for blocking conditions
    }
}
```

**First-Come-First-Served (FCFS):**
```cpp
void executeFCFS(const std::shared_ptr<Process>& proc, uint64_t& lastTickSeen) {
    while (!proc->getIsFinished()) {
        // Execute instructions until process blocks or completes
    }
}
```

### Queue Management

#### Ready Queue
```cpp
std::deque<std::shared_ptr<Process>> readyQueue;
std::mutex readyMutex;
```

**Operations:**
- **Enqueue**: Add process to ready queue
- **Dequeue**: Remove process for execution
- **Priority**: FCFS ordering maintained

#### Wait Queue
```cpp
std::priority_queue<std::shared_ptr<Process>, 
                   std::vector<std::shared_ptr<Process>>, 
                   WakeupComparator> waitQueue;
```

**Wakeup Comparator:**
```cpp
struct WakeupComparator {
    bool operator()(const std::shared_ptr<Process>& a, 
                   const std::shared_ptr<Process>& b) const {
        return a->getWakeupTick() > b->getWakeupTick();
    }
};
```

### Performance Metrics

#### CPU Utilization Tracking
```cpp
std::atomic<uint64_t> activeCpuTicks = 0;
std::atomic<uint64_t> idleCpuTicks = 0;
```

**Calculation:**
- **Active Ticks**: Time spent executing processes
- **Idle Ticks**: Time spent with no ready processes
- **Utilization**: (Active Ticks / Total Ticks) × 100%

---

## Instruction System

### Instruction Hierarchy

#### Base Instruction Class
```cpp
class Instruction {
protected:
    int lineCount;
    int pid;
    std::string opCode;
    
public:
    virtual void execute() = 0;
    virtual std::string serialize() const = 0;
    virtual bool isComplete() const { return true; }
};
```

#### Instruction Types

**Arithmetic Instructions:**
```cpp
class ArithmeticInstruction : public Instruction {
    std::string operation;
    std::string operand1, operand2, result;
    
public:
    void execute() override {
        // Perform arithmetic operation
        // Handle memory access for variables
    }
};
```

**Memory Instructions:**
```cpp
class ReadInstruction : public Instruction {
    std::string variable;
    int address;
    
public:
    void execute() override {
        // Handle page fault if needed
        // Read value from memory
        // Store in variable
    }
};
```

**Control Instructions:**
```cpp
class ForInstruction : public Instruction {
    int iterations;
    int currentIteration;
    std::vector<std::shared_ptr<Instruction>> loopBody;
    
public:
    bool isComplete() const override {
        return currentIteration >= iterations;
    }
    
    void execute() override {
        // Execute loop body
        // Increment iteration counter
    }
};
```

### Instruction Factory

#### Dynamic Creation
```cpp
static std::shared_ptr<Instruction> parseInstructionString(
    const std::string& instrStr, int processID) {
    // Parse instruction string
    // Create appropriate instruction type
    // Validate parameters
    // Return instruction object
}
```

**Supported Instructions:**
- **DECLARE**: Variable declaration
- **PRINT**: Output operation
- **READ**: Memory read
- **WRITE**: Memory write
- **ADD/SUB/MUL/DIV**: Arithmetic operations
- **SLEEP**: Process suspension
- **FOR**: Loop construct

### Memory Access Patterns

#### Page Fault Handling
```cpp
void safePageFault(int page) const {
    // Log page fault
    // Trigger page fault handler
    // Wait for page to be loaded
}
```

**Access Flow:**
1. **Address Validation**: Check if address is valid
2. **Page Lookup**: Find page number for address
3. **Page Table Check**: Check if page is present
4. **Page Fault**: Trigger fault if page not present
5. **Memory Access**: Access data once page is loaded

---

## User Interface

### Console Manager Architecture

#### Screen Management
```cpp
class ConsoleManager {
private:
    std::shared_ptr<Screen> currentScreen;
    std::unordered_map<std::string, std::shared_ptr<Process>> processNameMap;
    
public:
    void switchConsole(const std::string& processName);
    void renderConsole() const;
};
```

**Screen Types:**
- **MainScreen**: Main menu and command interface
- **ProcessScreen**: Process-specific interface
- **Screen Interface**: Abstract base for all screens

#### Command Processing
```cpp
void handleUserInput() {
    // Parse user input
    // Route to appropriate handler
    // Execute command
    // Update display
}
```

### Command Interface

#### Main Commands
```bash
# System control
initialize          # Initialize the system
exit               # Exit the program
clear              # Clear the console

# Process management
screen -s <name> <mem>     # Create process
screen -c <name> <mem> "<instrs>"  # Create with instructions
screen -r <name>           # Resume process
screen -ls                 # List processes

# Scheduler control
scheduler-start            # Start dummy generation
scheduler-stop             # Stop dummy generation
scheduler-status           # Show scheduler status

# Monitoring
report-util               # CPU utilization report
process-smi               # Process status
vmstat                    # Memory statistics
visualize                 # Memory visualization
```

#### Process Commands
```bash
# Process-specific commands (when in process screen)
exit                      # Return to main screen
<instruction>             # Execute instruction
```

### Display System

#### Color-Coded Output
```cpp
static void setColor(int color);
static void resetColor();
```

**Color Scheme:**
- **Blue (36)**: Welcome messages
- **Yellow (33)**: Instructions and hints
- **Magenta (35)**: Prompts and headers
- **Cyan (36)**: Status information

#### ASCII Art and Formatting
```cpp
const std::string asciiArt = R"(
__________                         .__       ________    _________
\______   \ _______  __ ___________|__| ____ \_____  \  /   _____/
 |       _// __ \  \/ // __ \_  __ \  |/ __ \ /   |   \ \_____  \
 |    |   \  ___/\   /\  ___/|  | \/  \  ___//    |    \/        \
 |____|_  /\___  >\_/  \___  >__|  |__|\___  >_______  /_______  /
        \/     \/          \/              \/        \/        \/
)";
```

---

## Configuration System

### Configuration Loading

#### File Format
```bash
# config.txt
num-cpu 8                    # Number of CPU cores
scheduler "rr"               # Scheduling algorithm
quantum-cycles 4             # Time quantum for RR
batch-process-freq 1         # Dummy process frequency
min-ins 1000                # Min instructions per process
max-ins 2000                # Max instructions per process
delays-per-exec 0           # Execution delays
max-overall-mem 32768       # Total system memory
mem-per-frame 256           # Page size
min-mem-per-proc 64         # Min process memory
max-mem-per-proc 64         # Max process memory
```

#### Loading Process
```cpp
bool Config::loadFromFile() {
    std::ifstream file("config.txt");
    // Parse each line
    // Validate parameters
    // Set configuration values
}
```

### Parameter Validation

#### Bounds Checking
```cpp
bool validateParameters() {
    return numCPUs > 0 && numCPUs <= MAX_CPUS &&
           quantumCycles > 0 &&
           maxOverallMem > 0 &&
           memPerFrame > 0 &&
           minMemPerProc <= maxMemPerProc;
}
```

#### Default Values
```cpp
// Default configuration values
int delayPerExec = 1;
uint8_t numCPUs = 4;
SchedulerType scheduler = SchedulerType::RR;
uint32_t quantumCycles = 5;
uint32_t maxOverallMem = 1024;
uint32_t memPerFrame = 64;
```

---

## Debugging and Monitoring

### System Monitoring Commands

#### Process Status (process-smi)
```cpp
void generateProcessSMI() {
    // Display process information
    // Show memory usage
    // Display execution status
    // List active processes
}
```

**Output Format:**
```
Process Status Report:
PID  Name     Status   Core   Memory   Progress
1    process1 RUNNING  0      64B      45/100
2    process2 READY    -      64B      0/100
```

#### Memory Statistics (vmstat)
```cpp
void generateVmStat() {
    // Show memory usage statistics
    // Display page fault information
    // Show frame allocation status
    // Report backing store usage
}
```

**Statistics Tracked:**
- **Total Memory**: System memory usage
- **Free Memory**: Available frames
- **Page Faults**: Number of page faults
- **Page Ins/Outs**: Swapping statistics
- **Frame Utilization**: Frame allocation percentage

#### Memory Visualization
```cpp
void visualizeMemory() {
    // Display frame map
    // Show process assignments
    // Highlight dirty pages
    // Show free frames
}
```

**Visualization Format:**
```
Memory Frame Map:
Frame  PID  Page  Dirty  Data
0      1    0     Yes    [DECLARE x 5]
1      2    1     No     [PRINT x]
2      -    -     -      [FREE]
```

### Logging System

#### Process Logging
```cpp
void log(const std::string& entry) {
    logs.push_back(entry);
    // Timestamp and format log entry
}
```

**Log Format:**
```
[2025-01-27 14:30:15] PID 1: DECLARE x 5
[2025-01-27 14:30:16] PID 1: PRINT x
[2025-01-27 14:30:17] PID 1: SLEEP 10
```

#### File Output
```cpp
void writeLogToFile() const {
    // Write logs to file in logs folder
    // Include timestamps and CPU core ID
    // Format for analysis
}
```

### Performance Metrics

#### CPU Utilization
```cpp
double calculateCPUUtilization() {
    return static_cast<double>(activeCpuTicks) / 
           static_cast<double>(totalCPUTicks) * 100.0;
}
```

#### Memory Efficiency
```cpp
double calculateMemoryEfficiency() {
    return static_cast<double>(allocatedFrames) / 
           static_cast<double>(totalFrames) * 100.0;
}
```

#### Page Fault Rate
```cpp
double calculatePageFaultRate() {
    return static_cast<double>(numPagedIn) / 
           static_cast<double>(totalMemoryAccesses);
}
```

---

## Conclusion

The OPESY OS Emulator represents a comprehensive implementation of key operating system concepts, demonstrating:

1. **Demand Paging**: Realistic memory management with page faults and eviction
2. **Multi-Core Scheduling**: Parallel execution with configurable algorithms
3. **Process Management**: Complete lifecycle from creation to termination
4. **Instruction System**: Extensible instruction set with memory access
5. **User Interface**: Intuitive console-based interaction
6. **Monitoring**: Comprehensive debugging and performance tools

The system's design choices prioritize educational value and conceptual clarity while maintaining realistic OS behavior. The demand paging strategy, in particular, creates authentic memory pressure scenarios that demonstrate real-world OS challenges.

This emulator serves as an excellent platform for understanding operating system principles, from basic process management to advanced memory management techniques. Its modular architecture makes it suitable for further extensions and educational use.

### Key Achievements

1. **Educational Value**: Clear demonstration of OS concepts
2. **Realistic Behavior**: Authentic memory pressure and scheduling scenarios
3. **Modular Design**: Extensible architecture for future enhancements
4. **Comprehensive Monitoring**: Detailed debugging and performance tools
5. **User-Friendly Interface**: Intuitive command system with helpful feedback

The OPESY OS Emulator successfully bridges the gap between theoretical OS concepts and practical implementation, providing a valuable tool for operating system education and research. 