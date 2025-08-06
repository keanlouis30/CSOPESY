# OPESY OS Emulator - Requirements

## 1. Memory Management Requirements

### Demand Paging System
- **Page Size**: Configurable via `mem-per-frame` (default: 256 bytes)
- **Total Memory**: Configurable via `max-overall-mem` (default: 32KB)
- **Process Memory Limits**: `min-mem-per-proc` to `max-mem-per-proc` (64-64 bytes default)
- **Page Fault Handling**: Automatic frame allocation with eviction when necessary
- **Backing Store**: File-based (`csopesy-backing-store.txt`) for swapped pages
- **Memory Constraints**: Processes cannot exceed allocated memory limits

### Memory Allocation Strategy
- **Frame Allocation**: First-fit algorithm with victim frame selection
- **Eviction Policy**: FIFO-based with pinned frame protection
- **Memory Visualization**: Real-time frame map display
- **Memory Statistics**: Usage tracking and reporting

### Memory Access Operations
- **READ**: Memory read operations with page fault handling
- **WRITE**: Memory write operations with dirty bit management
- **DECLARE**: Variable declaration and memory allocation
- **ARITHMETIC**: Basic arithmetic operations
- **PRINT**: Output operations
- **SLEEP**: Process suspension with wakeup scheduling
- **FOR**: Loop constructs with nested instruction support

### Memory Access Patterns
- **Demand Loading**: Pages loaded only when accessed
- **Dirty Bit Tracking**: Modified pages marked for write-back
- **Address Validation**: Heap address bounds checking
- **Memory Segments**: TEXT, DATA, HEAP separation

## 2. Process Creation and Management

### Process Creation Commands
- **`screen -s <name> <mem_size>`**: Create process with specified memory
- **`screen -c <name> <mem_size> "<instructions>"`**: Create with custom instructions
- **`screen -r <name>`**: Resume existing process
- **`screen -ls`**: List all processes

### Process Lifecycle
- **States**: READY, RUNNING, WAITING, DONE
- **Memory Allocation**: Automatic based on instruction count
- **Core Assignment**: Dynamic CPU core allocation
- **Termination**: Automatic cleanup of memory and resources

### Process Management Features
- **Process ID Assignment**: Unique identifier for each process
- **Memory Segmentation**: TEXT, DATA, HEAP segment management
- **Variable Management**: Dynamic variable allocation and access
- **Instruction Tracking**: Progress monitoring and execution state
- **Logging System**: Per-process execution logs with timestamps

## 3. CPU Scheduling Requirements

### Scheduling Algorithms
- **FCFS (First-Come-First-Served)**: Non-preemptive scheduling
- **RR (Round-Robin)**: Preemptive scheduling with configurable quantum
- **Multi-Core Support**: Parallel execution across multiple CPU cores
- **Context Switching**: Automatic process state management

### CPU Execution Model
- **Tick-Based Execution**: Synchronized execution cycles
- **Core Assignment**: Dynamic process-to-core mapping
- **Quantum Management**: Time slice enforcement for RR scheduling
- **Idle Detection**: CPU utilization tracking and reporting

### Queue Management
- **Ready Queue**: Thread-safe deque for ready processes
- **Wait Queue**: Priority queue for sleeping processes
- **Core Assignment Tracking**: Process-to-core mapping
- **Queue Visualization**: Real-time queue status display

## 4. Instruction System Requirements

### Supported Instructions
- **DECLARE**: Variable declaration with memory allocation
- **PRINT**: Output operations with formatting
- **READ**: Memory read operations with address validation
- **WRITE**: Memory write operations with dirty bit management
- **ADD/SUB/MUL/DIV**: Arithmetic operations with variable support
- **SLEEP**: Process suspension with wakeup scheduling
- **FOR**: Loop constructs with nested instruction support

### Instruction Execution
- **Factory Pattern**: Dynamic instruction creation from strings
- **Memory Access**: Page fault handling for memory operations
- **State Management**: Process state updates during execution
- **Progress Tracking**: Instruction completion monitoring
- **Error Handling**: Graceful error handling and reporting

### Instruction Validation
- **Parameter Validation**: Instruction parameter bounds checking
- **Memory Validation**: Memory size and address validation
- **Syntax Checking**: Instruction string parsing and validation
- **Resource Validation**: Memory and CPU resource availability

## 5. User Interface Requirements

### Console Commands
- **System Control**:
  - `initialize`: Initialize the system
  - `exit`: Exit the program
  - `clear`: Clear the console

- **Process Management**:
  - `screen -s <name> <mem>`: Create process
  - `screen -c <name> <mem> "<instrs>"`: Create with instructions
  - `screen -r <name>`: Resume process
  - `screen -ls`: List processes

- **Scheduler Control**:
  - `scheduler-start`: Start dummy generation
  - `scheduler-stop`: Stop dummy generation
  - `scheduler-status`: Show scheduler status

- **Monitoring**:
  - `report-util`: CPU utilization report
  - `process-smi`: Process status
  - `vmstat`: Memory statistics
  - `visualize`: Memory visualization

### Display Features
- **Color-Coded Output**: ANSI color codes for different message types
- **ASCII Art**: System branding and visual appeal
- **Real-Time Updates**: Live status and progress updates
- **Error Messages**: Clear and informative error reporting

## 6. Configuration System Requirements

### Configuration File (config.txt)
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

### Configuration Features
- **Dynamic Loading**: Configuration loaded at startup
- **Parameter Validation**: Bounds checking for all parameters
- **Default Values**: Sensible defaults for missing parameters
- **Runtime Access**: Configuration values accessible throughout system

## 7. Debugging and Monitoring Requirements

### System Monitoring Commands
- **`process-smi`**: Process status and memory information
- **`vmstat`**: Virtual memory statistics
- **`visualize`**: Real-time memory frame visualization
- **`report-util`**: CPU utilization report

### Debugging Features
- **Process Logging**: Per-process execution logs with timestamps
- **Memory Tracking**: Frame allocation/deallocation logging
- **Performance Metrics**: CPU ticks, memory usage, page faults
- **Error Handling**: Graceful shutdown with reason reporting

### Performance Metrics
- **CPU Utilization**: Active vs idle CPU time tracking
- **Memory Efficiency**: Frame utilization and allocation statistics
- **Page Fault Rate**: Page fault frequency and impact
- **Process Statistics**: Execution time and resource usage

## 8. Memory Management Constraints

### Memory Limits
- **Total System Memory**: Configurable maximum (default: 32KB)
- **Per-Process Memory**: Minimum and maximum limits (64-64 bytes default)
- **Page Size**: Configurable frame size (256 bytes default)
- **Frame Count**: Limited by total memory and page size

### Memory Validation
- **Power-of-Two Validation**: Memory sizes must be powers of two
- **Bounds Checking**: Address validation for all memory accesses
- **Segment Validation**: Memory segment boundary checking
- **Resource Validation**: Memory availability before allocation

## 9. Process Management Constraints

### Process Limits
- **Memory Constraints**: Cannot exceed allocated memory
- **Instruction Limits**: Minimum and maximum instructions per process
- **Variable Limits**: Memory-based variable count limits
- **Execution Time**: Quantum-based execution limits

### Process Validation
- **Name Validation**: Unique process name requirements
- **Memory Validation**: Memory size within system limits
- **Instruction Validation**: Valid instruction syntax and parameters
- **Resource Validation**: Available memory and CPU resources

## 10. Scheduling Constraints

### CPU Constraints
- **Core Count**: Limited by configuration (default: 8 cores)
- **Quantum Limits**: Configurable time quantum for RR scheduling
- **Queue Limits**: Memory-based queue size limits
- **Context Switch Overhead**: Simulated context switching delays

### Scheduling Validation
- **Process State Validation**: Valid state transitions
- **Resource Availability**: CPU core availability checking
- **Queue Management**: Thread-safe queue operations
- **Synchronization**: Proper barrier and mutex usage

## 11. Instruction System Constraints

### Instruction Limits
- **Memory-Based Limits**: Instructions constrained by process memory
- **Syntax Validation**: Valid instruction format requirements
- **Parameter Validation**: Instruction parameter bounds checking
- **Resource Validation**: Memory and CPU resource availability

### Execution Constraints
- **Page Fault Handling**: Automatic page fault resolution
- **Memory Access Validation**: Address bounds checking
- **State Management**: Process state consistency
- **Error Recovery**: Graceful error handling and recovery

## 12. User Interface Constraints

### Input Validation
- **Command Syntax**: Valid command format requirements
- **Parameter Validation**: Command parameter bounds checking
- **Resource Validation**: System resource availability
- **Error Reporting**: Clear and informative error messages

### Display Constraints
- **Console Compatibility**: ANSI escape code support
- **Screen Size**: Console window size considerations
- **Update Frequency**: Real-time update performance
- **Memory Usage**: Display memory usage optimization

## 13. Configuration Constraints

### File Format
- **Text Format**: Human-readable configuration file
- **Parameter Validation**: Valid parameter ranges and types
- **Default Values**: Sensible defaults for missing parameters
- **Error Handling**: Graceful handling of invalid configurations

### Runtime Constraints
- **Memory Usage**: Configuration storage memory limits
- **Access Performance**: Fast configuration value access
- **Thread Safety**: Thread-safe configuration access
- **Validation Overhead**: Minimal validation performance impact

## 14. Monitoring and Debugging Constraints

### Performance Impact
- **Minimal Overhead**: Monitoring should not significantly impact performance
- **Memory Usage**: Monitoring data storage limits
- **Update Frequency**: Reasonable update intervals
- **Display Performance**: Real-time display performance

### Data Management
- **Log File Size**: Log file size limits and rotation
- **Memory Usage**: Monitoring data memory usage
- **Cleanup**: Automatic cleanup of old monitoring data
- **Privacy**: Process data privacy and security

## 15. System Integration Requirements

### Component Integration
- **Singleton Pattern**: Proper singleton implementation across components
- **Thread Safety**: Thread-safe operations across all components
- **Error Propagation**: Proper error handling and propagation
- **Resource Management**: Proper resource allocation and cleanup

### Performance Requirements
- **Responsive UI**: Quick command response times
- **Efficient Memory**: Minimal memory overhead
- **Scalable Design**: Support for multiple processes and cores
- **Reliable Operation**: Stable operation under various conditions

### Extensibility Requirements
- **Modular Design**: Easy addition of new features
- **Plugin Architecture**: Support for new instruction types
- **Configuration Extensibility**: Easy addition of new configuration parameters
- **Monitoring Extensibility**: Easy addition of new monitoring features

This comprehensive requirements specification ensures the OPESY OS Emulator provides a realistic and educational simulation of operating system concepts while maintaining system stability and performance. 