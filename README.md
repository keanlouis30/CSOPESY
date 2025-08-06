# OPESY OS Emulator - Comprehensive System

## Overview

The OPESY OS Emulator is a sophisticated C++23-based simulation of a multitasking operating system that demonstrates key OS concepts including demand paging, process scheduling, memory management, and instruction execution. The system operates as a console-based emulator with a modular architecture designed to simulate real-world OS behaviors.

## Key Features

### 🧠 Memory Management
- **Demand Paging**: Pages loaded only when accessed
- **Page Fault Handling**: Automatic frame allocation with eviction
- **Backing Store**: File-based storage for swapped pages
- **Memory Segmentation**: TEXT, DATA, HEAP separation
- **Frame Management**: FIFO-based eviction with pinned protection

### ⚡ Process Management
- **Multi-Core Support**: Parallel execution across CPU cores
- **Process States**: READY, RUNNING, WAITING, DONE
- **Dynamic Scheduling**: Round-Robin and FCFS algorithms
- **Memory Allocation**: Automatic based on instruction count
- **Process Lifecycle**: Complete creation to termination

### 🎯 Instruction System
- **Factory Pattern**: Dynamic instruction creation
- **Extensible Design**: Easy addition of new instruction types
- **Memory Operations**: READ, WRITE, DECLARE instructions
- **Control Flow**: FOR loops and conditional execution
- **Arithmetic Operations**: ADD, SUB, MUL, DIV support

### 🖥️ User Interface
- **Console Manager**: Singleton pattern for centralized control
- **Screen Management**: Main screen and process-specific screens
- **Color-Coded Output**: ANSI color codes for different message types
- **Real-Time Updates**: Live status and progress updates
- **Command Interface**: Intuitive command system

### ⚙️ Configuration System
- **Runtime Configuration**: Load parameters from config.txt
- **Parameter Validation**: Bounds checking for all parameters
- **Default Values**: Sensible defaults for missing parameters
- **Hot Reload**: Some parameters can be changed at runtime

## System Architecture

### Core Components

1. **Console Manager** (Singleton)
   - Central coordinator for user interface
   - Screen management and transitions
   - Process creation and lifecycle management

2. **Memory Manager** (Demand Paging)
   - Page table management
   - Frame allocation and eviction
   - Backing store operations
   - Memory segmentation

3. **Process Scheduler** (Multi-Core)
   - Thread-per-core execution model
   - Round-Robin and FCFS algorithms
   - Context switching and state management
   - CPU utilization tracking

4. **Instruction System** (Factory Pattern)
   - Dynamic instruction creation
   - Memory access operations
   - Variable management
   - Control flow constructs

5. **Configuration System** (Singleton)
   - Parameter validation and bounds checking
   - Default value management
   - Runtime configuration access

## Installation and Setup

### Prerequisites
- C++23 compatible compiler (GCC 13+, Clang 17+, MSVC 2022)
- CMake 3.20 or higher
- Make or Ninja build system

### Building the System

```bash
# Clone the repository
git clone <repository-url>
cd CSOPESY

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make -j$(nproc)

# Run the emulator
./opesy_os_emulator
```

### Configuration

The system uses `config.txt` for configuration:

```bash
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

## Usage Guide

### Starting the System

```bash
# Initialize the system
initialize

# Start automatic process generation
scheduler-start

# Stop automatic process generation
scheduler-stop
```

### Process Management

```bash
# Create a new process
screen -s process1 64

# Create process with custom instructions
screen -c process2 64 "DECLARE x 5; PRINT x; SLEEP 10"

# Resume existing process
screen -r process1

# List all processes
screen -ls
```

### Monitoring Commands

```bash
# CPU utilization report
report-util

# Process status and memory information
process-smi

# Virtual memory statistics
vmstat

# Real-time memory visualization
visualize
```

### Process Commands (when in process screen)

```bash
# Execute instruction
DECLARE x 5
PRINT x
READ y 100
WRITE x 200
ADD x y z
SLEEP 10

# Return to main screen
exit
```

## Memory Management Details

### Demand Paging System

The system implements a sophisticated demand paging strategy:

- **Page Size**: Configurable (default: 256 bytes)
- **Total Memory**: Configurable (default: 32KB)
- **Process Limits**: 64-64 bytes per process (configurable)
- **Page Faults**: Automatic frame allocation with eviction
- **Backing Store**: File-based (`csopesy-backing-store.txt`)

### Memory Segmentation

Each process has three memory segments:

1. **TEXT**: Instruction storage
2. **DATA**: Variable storage
3. **HEAP**: Dynamic memory allocation

### Page Fault Handling

1. **Detection**: Memory access triggers page fault
2. **Validation**: Check if page exists in backing store
3. **Allocation**: Find or evict frame for allocation
4. **Loading**: Load page data from memory or backing store
5. **Mapping**: Update page table with frame number

## Process Scheduling

### Multi-Core Architecture

- **Worker Threads**: One per CPU core
- **Tick Synchronization**: Barrier-based cycle coordination
- **Queue Management**: Thread-safe ready and wait queues
- **Core Assignment**: Dynamic process-to-core mapping

### Scheduling Algorithms

#### Round-Robin (RR)
- Preemptive scheduling with configurable quantum
- Time slice enforcement
- Process preemption when quantum expires

#### First-Come-First-Served (FCFS)
- Non-preemptive scheduling
- Process runs until completion or blocking
- Simple and predictable

## Instruction System

### Supported Instructions

- **DECLARE**: Variable declaration with memory allocation
- **PRINT**: Output operations with formatting
- **READ**: Memory read operations with address validation
- **WRITE**: Memory write operations with dirty bit management
- **ADD/SUB/MUL/DIV**: Arithmetic operations with variable support
- **SLEEP**: Process suspension with wakeup scheduling
- **FOR**: Loop constructs with nested instruction support

### Instruction Execution Model

1. **Validation**: Check instruction parameters
2. **Memory Access**: Handle page faults if needed
3. **Execution**: Perform instruction operation
4. **State Update**: Update process state and logs
5. **Completion**: Mark instruction as complete

## Monitoring and Debugging

### System Monitoring

- **CPU Utilization**: Active vs idle CPU time tracking
- **Memory Efficiency**: Frame utilization and allocation statistics
- **Page Fault Rate**: Page fault frequency and impact
- **Process Statistics**: Execution time and resource usage

### Debugging Features

- **Process Logging**: Per-process execution logs with timestamps
- **Memory Tracking**: Frame allocation/deallocation logging
- **Performance Metrics**: CPU ticks, memory usage, page faults
- **Error Handling**: Graceful shutdown with reason reporting

## File Structure

```
CSOPESY/
├── main.cpp                 # Main application entry point
├── Config.h/.cpp           # Configuration system (singleton)
├── Console.h/.cpp          # Console manager (singleton)
├── Instructions.h/.cpp     # Instruction system (factory pattern)
├── MemoryManager.h/.cpp    # Memory management (demand paging)
├── Process.h/.cpp          # Process management
├── Scheduler.h/.cpp        # CPU scheduling (multi-core)
├── CPU_Core.h/.cpp         # CPU core implementation
├── Status.h                # Status enums and utilities
├── Utils.h/.cpp           # Utility functions
├── config.txt              # Configuration file
├── README.md              # This file
└── docs/                  # Documentation
    ├── OPESY_OS_Emulator_Comprehensive_Document.md
    ├── OPESY_OS_Emulator_Requirements.md
    └── OPESY_OS_Emulator_System_Outline.md
```

## Design Principles

### Educational Focus
- **Clarity**: Code designed for learning and understanding
- **Documentation**: Comprehensive comments and explanations
- **Modularity**: Clear separation of concerns
- **Extensibility**: Easy to add new features

### Realistic Behavior
- **Memory Pressure**: Authentic memory constraints
- **Page Faults**: Real page fault scenarios
- **Scheduling**: Realistic process scheduling
- **Resource Contention**: Processes competing for resources

### Thread Safety
- **Proper Synchronization**: Mutexes and atomic operations
- **Race Condition Prevention**: Careful design of concurrent operations
- **Deadlock Prevention**: Proper lock ordering
- **Performance**: Minimal synchronization overhead

## Performance Considerations

### Memory Management
- **Efficient Page Tables**: Hash-based page table lookup
- **Frame Allocation**: First-fit algorithm with victim selection
- **Backing Store**: File-based with buffering
- **Memory Visualization**: Real-time frame map display

### CPU Scheduling
- **Barrier Synchronization**: Efficient tick coordination
- **Queue Management**: Thread-safe operations
- **Context Switching**: Minimal overhead
- **Core Assignment**: Dynamic load balancing

### Monitoring
- **Minimal Overhead**: Monitoring doesn't impact performance
- **Efficient Logging**: Timestamp-based log management
- **Memory Usage**: Optimized data structures
- **Update Frequency**: Reasonable update intervals

## Troubleshooting

### Common Issues

1. **Configuration Errors**
   - Check `config.txt` format and parameter values
   - Ensure all required parameters are present
   - Validate parameter bounds

2. **Memory Issues**
   - Check available system memory
   - Verify backing store file permissions
   - Monitor page fault rates

3. **Scheduling Issues**
   - Check CPU core count configuration
   - Monitor queue sizes and process states
   - Verify quantum settings

4. **Process Issues**
   - Check process memory limits
   - Verify instruction syntax
   - Monitor process state transitions

### Debug Commands

```bash
# Check system status
process-smi

# Monitor memory usage
vmstat

# Visualize memory layout
visualize

# Generate utilization report
report-util
```

## Contributing

### Development Guidelines

1. **Code Style**: Follow C++23 standards and project conventions
2. **Documentation**: Add comprehensive comments for new features
3. **Testing**: Test new features thoroughly
4. **Modularity**: Keep components loosely coupled
5. **Thread Safety**: Ensure thread-safe operations

### Adding New Features

1. **Instruction Types**: Extend the instruction system
2. **Scheduling Algorithms**: Add new scheduling strategies
3. **Memory Policies**: Implement new eviction policies
4. **Monitoring Tools**: Add new debugging features

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Based on the MO2 OS Emulator concept
- Designed for educational purposes
- Demonstrates real-world OS concepts
- Suitable for operating system courses

## Contact

For questions, issues, or contributions, please contact the development team.

---

**OPESY OS Emulator** - A comprehensive simulation of operating system concepts for educational purposes. 