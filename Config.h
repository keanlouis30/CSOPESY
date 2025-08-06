#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_map>

enum class SchedulerType {
    FCFS,
    RR
};

class Config {
private:
    // Singleton instance
    static std::unique_ptr<Config> instance;
    
    // Configuration parameters
    uint8_t numCPUs = 4;
    SchedulerType scheduler = SchedulerType::RR;
    uint32_t quantumCycles = 5;
    uint32_t batchProcessFreq = 1;
    uint32_t minIns = 1000;
    uint32_t maxIns = 2000;
    uint32_t maxOverallMem = 32768;  // 32KB default
    uint32_t memPerFrame = 256;      // 256 bytes per frame
    
    // Parameter validation
    bool validateParameters() const;
    bool isPowerOfTwo(uint32_t value) const;

public:
    Config() = default;
    // Singleton access
    static Config& getInstance();
    uint32_t delayPerExec = 1;
    uint32_t minMemPerProc = 64;     // 64 bytes minimum
    uint32_t maxMemPerProc = 64;     // 64 bytes maximum
    
    // Prevent copying
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    
    // Configuration loading
    bool loadFromFile(const std::string& filename);
    bool loadFromString(const std::string& configStr);
    
    // Getters
    uint8_t getNumCPUs() const { return numCPUs; }
    SchedulerType getScheduler() const { return scheduler; }
    uint32_t getQuantumCycles() const { return quantumCycles; }
    uint32_t getBatchProcessFreq() const { return batchProcessFreq; }
    uint32_t getMinIns() const { return minIns; }
    uint32_t getMaxIns() const { return maxIns; }
    uint32_t getDelayPerExec() const { return delayPerExec; }
    uint32_t getMaxOverallMem() const { return maxOverallMem; }
    uint32_t getMemPerFrame() const { return memPerFrame; }
    uint32_t getMinMemPerProc() const { return minMemPerProc; }
    uint32_t getMaxMemPerProc() const { return maxMemPerProc; }
    
    // Utility methods
    std::string getSchedulerString() const;
    uint32_t getTotalFrames() const { return maxOverallMem / memPerFrame; }
    bool validateMemorySize(uint32_t memSize) const;
    
    // Display configuration
    void printConfiguration() const;
};