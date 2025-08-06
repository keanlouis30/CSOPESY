#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <ctime>
#include <atomic>
#include <memory>
#include <chrono>
#include "Status.h"
#include "Config.h"
#include "Instructions.h"
#include "MemoryManager.h"

// Process status enum
enum class ProcessStatus {
    READY,
    RUNNING,
    WAITING,
    DONE
};

// Process class
class Process {
public:
    // Basic process information
    std::string name;
    int pid;
    ProcessStatus status;
    std::string creationTimestamp;
    
    // Memory management
    size_t memoryStartAddress;
    size_t memorySize;
    std::unordered_map<MemorySegment, std::pair<size_t, size_t>> segments;
    
    // Instruction management
    std::vector<std::shared_ptr<Instruction>> instructions;
    size_t currentInstructionIndex;
    size_t totalInstructions;
    
    // Variable management
    std::unordered_map<std::string, uint16_t> variables;
    
    // CPU scheduling
    int assignedCoreId;
    uint32_t quantumRemaining;
    uint32_t quantumMax;
    uint64_t wakeupTick;
    
    // Statistics and monitoring
    uint64_t startTime;
    uint64_t totalExecutionTime;
    uint64_t pageFaults;
    uint64_t memoryAccesses;
    
    // Logging
    std::vector<std::string> logs;
    std::mutex logMutex;
    
    // Constructor
    Process(const std::string& processName, int processID, const Config& config);
    Process(const std::string& processName, int processID, const Config& config, 
            const std::string& customInstructions);
    
    // Copy constructor and assignment
    Process(const Process& other);
    Process& operator=(const Process& other);
    
    // Destructor
    ~Process() = default;
    
    // Process lifecycle management
    void initialize();
    void start();
    void pause();
    void resume();
    void terminate();
    
    // Instruction execution
    bool executeNextInstruction();
    bool isExecutionComplete() const;
    double getProgress() const;
    
    // Memory operations
    bool allocateMemory(size_t size);
    void deallocateMemory();
    bool readVariable(const std::string& variable, uint16_t& value);
    bool writeVariable(const std::string& variable, uint16_t value);
    
    // State management
    void setStatus(ProcessStatus newStatus);
    ProcessStatus getStatus() const { return status; }
    bool isReady() const { return status == ProcessStatus::READY; }
    bool isRunning() const { return status == ProcessStatus::RUNNING; }
    bool isWaiting() const { return status == ProcessStatus::WAITING; }
    bool isDone() const { return status == ProcessStatus::DONE; }
    
    // CPU core management
    void assignToCore(int coreId);
    void releaseFromCore();
    int getAssignedCore() const { return assignedCoreId; }
    
    // Quantum management
    void setQuantum(uint32_t quantum);
    uint32_t getQuantumRemaining() const { return quantumRemaining; }
    void decrementQuantum();
    bool hasQuantumExpired() const { return quantumRemaining == 0; }
    
    // Sleep management
    void sleep(uint64_t duration);
    bool shouldWakeUp(uint64_t currentTick) const;
    uint64_t getWakeupTick() const { return wakeupTick; }
    
    // Statistics
    uint64_t getTotalExecutionTime() const { return totalExecutionTime; }
    uint64_t getPageFaults() const { return pageFaults; }
    uint64_t getMemoryAccesses() const { return memoryAccesses; }
    
    // Logging
    void log(const std::string& message);
    const std::vector<std::string>& getLogs() const { return logs; }
    void clearLogs();
    
    // Memory segmentation
    std::pair<size_t, size_t> getSegmentBounds(MemorySegment segment) const;
    bool isValidAddress(size_t address) const;
    
    // Utility methods
    std::string getStatusString() const;
    std::string getFormattedInfo() const;
    void updateStatistics();
    
    // Legacy compatibility
    int commandCounter;
    int totalCommands;
    
    // Getters for legacy compatibility
    int getPID() const;
    void setMemoryStartAddress(size_t address);
    void setMemorySize(size_t size);
    
private:
    // Configuration reference
    const Config& config;
    
    // Memory manager reference
    MemoryManager* memoryManager;
    
    // Private methods
    void generateRandomInstructions();
    void parseCustomInstructions(const std::string& instructionString);
    void setupMemorySegments();
    void initializeVariables();
    void updateExecutionTime();
    
    // Memory validation
    bool validateMemoryAccess(size_t address) const;
    bool validateVariableName(const std::string& variable) const;
};