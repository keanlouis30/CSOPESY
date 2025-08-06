#include "Process.h"
#include "Config.h"
#include "MemoryManager.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <random>

// Global memory manager reference
extern MemoryManager g_memory_manager;

Process::Process(const std::string& processName, int processID, const Config& configRef)
    : name(processName), pid(processID), status(ProcessStatus::READY), config(configRef),
      memoryManager(nullptr), currentInstructionIndex(0), totalInstructions(0),
      assignedCoreId(-1), quantumRemaining(0), quantumMax(0), wakeupTick(0),
      startTime(0), totalExecutionTime(0), pageFaults(0), memoryAccesses(0),
      commandCounter(0), totalCommands(0) {
    
    // Set creation timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    creationTimestamp = ss.str();
    
    // Generate random instructions
    generateRandomInstructions();
    
    // Setup memory segments
    setupMemorySegments();
    
    // Initialize variables
    initializeVariables();
}

Process::Process(const std::string& processName, int processID, const Config& configRef, 
                 const std::string& customInstructions)
    : name(processName), pid(processID), status(ProcessStatus::READY), config(configRef),
      memoryManager(nullptr), currentInstructionIndex(0), totalInstructions(0),
      assignedCoreId(-1), quantumRemaining(0), quantumMax(0), wakeupTick(0),
      startTime(0), totalExecutionTime(0), pageFaults(0), memoryAccesses(0),
      commandCounter(0), totalCommands(0) {

    // Set creation timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    creationTimestamp = ss.str();
    
    // Parse custom instructions
    parseCustomInstructions(customInstructions);
    
    // Setup memory segments
    setupMemorySegments();
    
    // Initialize variables
    initializeVariables();
}

Process::Process(const Process& other)
    : name(other.name), pid(other.pid), status(other.status), 
      creationTimestamp(other.creationTimestamp), config(other.config),
      memoryStartAddress(other.memoryStartAddress), memorySize(other.memorySize),
      segments(other.segments), instructions(other.instructions),
      currentInstructionIndex(other.currentInstructionIndex), totalInstructions(other.totalInstructions),
      variables(other.variables), assignedCoreId(other.assignedCoreId),
      quantumRemaining(other.quantumRemaining), quantumMax(other.quantumMax),
      wakeupTick(other.wakeupTick), startTime(other.startTime),
      totalExecutionTime(other.totalExecutionTime), pageFaults(other.pageFaults),
      memoryAccesses(other.memoryAccesses), logs(other.logs),
      commandCounter(other.commandCounter), totalCommands(other.totalCommands) {
}

Process::Process()
    : name(""), pid(0), status(ProcessStatus::READY), 
      memoryManager(nullptr), currentInstructionIndex(0), totalInstructions(0),
      assignedCoreId(-1), quantumRemaining(0), quantumMax(0), wakeupTick(0),
      startTime(0), totalExecutionTime(0), pageFaults(0), memoryAccesses(0),
      commandCounter(0), totalCommands(0), config(Config()) {  // use default config

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    creationTimestamp = ss.str();
}


Process& Process::operator=(const Process& other) {
    if (this != &other) {
        name = other.name;
        pid = other.pid;
        status = other.status;
        creationTimestamp = other.creationTimestamp;
        memoryStartAddress = other.memoryStartAddress;
        memorySize = other.memorySize;
        segments = other.segments;
        instructions = other.instructions;
        currentInstructionIndex = other.currentInstructionIndex;
        totalInstructions = other.totalInstructions;
        variables = other.variables;
        assignedCoreId = other.assignedCoreId;
        quantumRemaining = other.quantumRemaining;
        quantumMax = other.quantumMax;
        wakeupTick = other.wakeupTick;
        startTime = other.startTime;
        totalExecutionTime = other.totalExecutionTime;
        pageFaults = other.pageFaults;
        memoryAccesses = other.memoryAccesses;
        logs = other.logs;
        commandCounter = other.commandCounter;
        totalCommands = other.totalCommands;
    }
    return *this;
}

void Process::initialize() {
    log("Process initialized");
    setStatus(ProcessStatus::READY);
}

ProcessStatus Process::getStatus() const {
    return status;
}

void Process::start() {
    if (status == ProcessStatus::READY) {
        setStatus(ProcessStatus::RUNNING);
        startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        log("Process started");
    }
}

void Process::pause() {
    if (status == ProcessStatus::RUNNING) {
        setStatus(ProcessStatus::READY);
        updateExecutionTime();
        log("Process paused");
    }
}

void Process::resume() {
    if (status == ProcessStatus::READY) {
        setStatus(ProcessStatus::RUNNING);
        log("Process resumed");
    }
}

void Process::terminate() {
    setStatus(ProcessStatus::DONE);
    updateExecutionTime();
    log("Process terminated");
}

bool Process::executeNextInstruction() {
    if (isExecutionComplete() || status != ProcessStatus::RUNNING) {
        return false;
    }
    
    if (currentInstructionIndex < instructions.size()) {
        auto& instruction = instructions[currentInstructionIndex];
        if (!instruction->isComplete()) {
            instruction->execute();
            commandCounter++;
            
            // Log instruction execution
            log("Executed: " + instruction->serialize());
            
            // Check if instruction is now complete
            if (instruction->isComplete()) {
                currentInstructionIndex++;
            }
            
            return true;
        } else {
            currentInstructionIndex++;
            return executeNextInstruction(); // Try next instruction
        }
    }
    
    return false;
}

bool Process::isExecutionComplete() const {
    return currentInstructionIndex >= instructions.size();
}

double Process::getProgress() const {
    if (totalInstructions == 0) return 0.0;
    return static_cast<double>(currentInstructionIndex) / static_cast<double>(totalInstructions) * 100.0;
}

bool Process::allocateMemory(size_t size) {
    auto result = g_memory_manager.allocateMemory(pid, size);
    if (result.success) {
        memoryStartAddress = result.startAddress;
        memorySize = result.size;
        log("Memory allocated: " + std::to_string(size) + " bytes");
        return true;
    } else {
        log("Memory allocation failed: " + result.errorMessage);
        return false;
    }
}

void Process::deallocateMemory() {
    g_memory_manager.deallocateMemory(pid);
    log("Memory deallocated");
}

bool Process::readVariable(const std::string& variable, uint16_t& value) {
    if (!validateVariableName(variable)) {
        return false;
    }
    
    // Simple variable lookup from variables map
    auto it = variables.find(variable);
    if (it != variables.end()) {
        value = it->second;
        memoryAccesses++;
        return true;
    }
    
    return false;
}

bool Process::writeVariable(const std::string& variable, uint16_t value) {
    if (!validateVariableName(variable)) {
        return false;
    }
    
    variables[variable] = value;
    memoryAccesses++;
    return true;
}

void Process::setStatus(ProcessStatus newStatus) {
    status = newStatus;
    log("Status changed to: " + getStatusString());
}

void Process::assignToCore(int coreId) {
    assignedCoreId = coreId;
    log("Assigned to core: " + std::to_string(coreId));
}

void Process::releaseFromCore() {
    assignedCoreId = -1;
    log("Released from core");
}

void Process::setQuantum(uint32_t quantum) {
    quantumMax = quantum;
    quantumRemaining = quantum;
}

void Process::decrementQuantum() {
    if (quantumRemaining > 0) {
        quantumRemaining--;
    }
}

void Process::sleep(uint64_t duration) {
    setStatus(ProcessStatus::WAITING);
    wakeupTick = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() + duration;
    log("Process sleeping for " + std::to_string(duration) + " ms");
}

bool Process::shouldWakeUp(uint64_t currentTick) const {
    return status == ProcessStatus::WAITING && currentTick >= wakeupTick;
}

void Process::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    logs.push_back("[" + ss.str() + "] PID " + std::to_string(pid) + ": " + message);
}

void Process::clearLogs() {
    std::lock_guard<std::mutex> lock(logMutex);
    logs.clear();
}

std::pair<size_t, size_t> Process::getSegmentBounds(MemorySegment segment) const {
    auto it = segments.find(segment);
    if (it != segments.end()) {
        return it->second;
    }
    return {0, 0};
}

bool Process::isValidAddress(size_t address) const {
    return g_memory_manager.isValidAddress(pid, address);
}

std::string Process::getStatusString() const {
    switch (status) {
        case ProcessStatus::READY: return "READY";
        case ProcessStatus::RUNNING: return "RUNNING";
        case ProcessStatus::WAITING: return "WAITING";
        case ProcessStatus::DONE: return "DONE";
        default: return "UNKNOWN";
    }
}

std::string Process::getFormattedInfo() const {
            std::stringstream ss;
    ss << "Process: " << name << " (PID: " << pid << ")\n";
    ss << "Status: " << getStatusString() << "\n";
    ss << "Core: " << (assignedCoreId >= 0 ? std::to_string(assignedCoreId) : "None") << "\n";
    ss << "Progress: " << std::fixed << std::setprecision(1) << getProgress() << "%\n";
    ss << "Memory: " << memorySize << " bytes\n";
    ss << "Instructions: " << currentInstructionIndex << "/" << totalInstructions << "\n";
    return ss.str();
}

void Process::updateStatistics() {
    if (status == ProcessStatus::RUNNING) {
        updateExecutionTime();
    }
}

void Process::generateRandomInstructions() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> numInstrDist(config.getMinIns(), config.getMaxIns());
    std::uniform_int_distribution<> memDist(0, 1023);
    std::uniform_int_distribution<> valueDist(1, 100);
    
    int numInstructions = numInstrDist(gen);
    totalInstructions = numInstructions;
    totalCommands = numInstructions;
    
    instructions.clear();
    
    for (int i = 0; i < numInstructions; ++i) {
        std::uniform_int_distribution<> instrTypeDist(0, 5);
        int instrType = instrTypeDist(gen);
        
        std::shared_ptr<Instruction> instruction;
        
        switch (instrType) {
            case 0: // DECLARE
                {
                    std::string varName = "var" + std::to_string(i);
                    int value = valueDist(gen);
                    instruction = std::make_shared<DeclareInstruction>(pid, varName, value);
                }
                break;
            case 1: // PRINT
                {
                    std::string varName = "var" + std::to_string(i % 10);
                    instruction = std::make_shared<PrintInstruction>(pid, varName);
                }
                break;
            case 2: // READ
                {
                    std::string varName = "var" + std::to_string(i);
                    int address = memDist(gen);
                    instruction = std::make_shared<ReadInstruction>(pid, varName, address);
                }
                break;
            case 3: // WRITE
                {
                    std::string varName = "var" + std::to_string(i);
                    int address = memDist(gen);
                    instruction = std::make_shared<WriteInstruction>(pid, varName, address);
                }
                break;
            case 4: // SLEEP
                {
                    int duration = std::uniform_int_distribution<>(1, 10)(gen);
                    instruction = std::make_shared<SleepInstruction>(pid, duration);
                }
                break;
            case 5: // ARITHMETIC
                {
                    std::string var1 = "var" + std::to_string(i);
                    std::string var2 = "var" + std::to_string(i + 1);
                    std::string result = "var" + std::to_string(i + 2);
                    instruction = std::make_shared<ArithmeticInstruction>(pid, "ADD", var1, var2, result);
                }
                break;
            }
        
        if (instruction) {
            instructions.push_back(instruction);
        }
    }
    
    log("Generated " + std::to_string(instructions.size()) + " random instructions");
}

void Process::parseCustomInstructions(const std::string& instructionString) {
    instructions = InstructionFactory::parseInstructionSequence(instructionString, pid);
    totalInstructions = instructions.size();
    totalCommands = instructions.size();
    log("Parsed " + std::to_string(instructions.size()) + " custom instructions");
}

void Process::setupMemorySegments() {
    // Simple segmentation: 50% TEXT, 30% DATA, 20% HEAP
    size_t textSize = memorySize * 5 / 10;
    size_t dataSize = memorySize * 3 / 10;
    size_t heapSize = memorySize * 2 / 10;
    
    segments[MemorySegment::TEXT] = {0, textSize};
    segments[MemorySegment::DATA] = {textSize, textSize + dataSize};
    segments[MemorySegment::HEAP] = {textSize + dataSize, memorySize};
}

void Process::initializeVariables() {
    // Initialize some default variables
    variables["x"] = 0;
    variables["y"] = 0;
    variables["z"] = 0;
}

void Process::updateExecutionTime() {
    if (startTime > 0) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        totalExecutionTime = now - startTime;
    }
}

bool Process::validateMemoryAccess(size_t address) const {
    return g_memory_manager.isValidAddress(pid, address);
}

bool Process::validateVariableName(const std::string& variable) const {
    if (variable.empty()) return false;
    
    // Check if variable name contains only alphanumeric characters and underscores
    for (char c : variable) {
        if (!std::isalnum(c) && c != '_') {
            return false;
        }
    }
    
    return true;
}

// Getters for legacy compatibility
int Process::getPID() const { return pid; }
void Process::setMemoryStartAddress(size_t address) { memoryStartAddress = address; }
void Process::setMemorySize(size_t size) { memorySize = size; }
