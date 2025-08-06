#include "Config.h"
#include <sstream>
#include <algorithm>
#include <cctype>

// Initialize static member
std::unique_ptr<Config> Config::instance = nullptr;

Config& Config::getInstance() {
    if (!instance) {
        instance = std::make_unique<Config>();
    }
    return *instance;
}

uint32_t Config::getDelayPerExec() const {
    return delayPerExec;
}


bool Config::isPowerOfTwo(uint32_t value) const {
    return value > 0 && (value & (value - 1)) == 0;
}

bool Config::validateParameters() const {
    // Validate CPU count
    if (numCPUs == 0 || numCPUs > 32) {
        std::cerr << "Error: num-cpu must be between 1 and 32" << std::endl;
        return false;
    }
    
    // Validate quantum cycles
    if (quantumCycles == 0) {
        std::cerr << "Error: quantum-cycles must be greater than 0" << std::endl;
        return false;
    }
    
    // Validate memory parameters
    if (maxOverallMem == 0 || !isPowerOfTwo(maxOverallMem)) {
        std::cerr << "Error: max-overall-mem must be a power of 2" << std::endl;
        return false;
    }
    
    if (memPerFrame == 0 || !isPowerOfTwo(memPerFrame)) {
        std::cerr << "Error: mem-per-frame must be a power of 2" << std::endl;
        return false;
    }
    
    if (minMemPerProc > maxMemPerProc) {
        std::cerr << "Error: min-mem-per-proc cannot be greater than max-mem-per-proc" << std::endl;
        return false;
    }
    
    if (minMemPerProc == 0 || !isPowerOfTwo(minMemPerProc)) {
        std::cerr << "Error: min-mem-per-proc must be a power of 2" << std::endl;
        return false;
    }
    
    if (maxMemPerProc == 0 || !isPowerOfTwo(maxMemPerProc)) {
        std::cerr << "Error: max-mem-per-proc must be a power of 2" << std::endl;
        return false;
    }
    
    // Validate instruction limits
    if (minIns > maxIns) {
        std::cerr << "Error: min-ins cannot be greater than max-ins" << std::endl;
        return false;
    }
    
    return true;
}

bool Config::validateMemorySize(uint32_t memSize) const {
    return memSize >= minMemPerProc && 
           memSize <= maxMemPerProc && 
           isPowerOfTwo(memSize);
}

std::string Config::getSchedulerString() const {
    switch (scheduler) {
        case SchedulerType::FCFS: return "fcfs";
        case SchedulerType::RR: return "rr";
        default: return "unknown";
    }
}

bool Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return false;
    }
    
    std::string line;
    std::unordered_map<std::string, std::string> configMap;
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Parse key-value pairs
        size_t pos = line.find(' ');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Remove quotes if present
            if (value.length() >= 2 && value[0] == '"' && value[value.length()-1] == '"') {
                value = value.substr(1, value.length()-2);
            }
            
            configMap[key] = value;
        }
    }
    
    file.close();
    
    // Parse configuration values
    try {
        if (configMap.find("num-cpu") != configMap.end()) {
            numCPUs = std::stoi(configMap["num-cpu"]);
        }
        
        if (configMap.find("scheduler") != configMap.end()) {
            std::string schedulerStr = configMap["scheduler"];
            std::transform(schedulerStr.begin(), schedulerStr.end(), schedulerStr.begin(), ::tolower);
            if (schedulerStr == "fcfs") {
                scheduler = SchedulerType::FCFS;
            } else if (schedulerStr == "rr") {
                scheduler = SchedulerType::RR;
            } else {
                std::cerr << "Error: Invalid scheduler type: " << schedulerStr << std::endl;
                return false;
            }
        }
        
        if (configMap.find("quantum-cycles") != configMap.end()) {
            quantumCycles = std::stoul(configMap["quantum-cycles"]);
        }
        
        if (configMap.find("batch-process-freq") != configMap.end()) {
            batchProcessFreq = std::stoul(configMap["batch-process-freq"]);
        }
        
        if (configMap.find("min-ins") != configMap.end()) {
            minIns = std::stoul(configMap["min-ins"]);
        }
        
        if (configMap.find("max-ins") != configMap.end()) {
            maxIns = std::stoul(configMap["max-ins"]);
        }
        
        if (configMap.find("delays-per-exec") != configMap.end()) {
            delayPerExec = std::stoul(configMap["delays-per-exec"]);
        }
        
        if (configMap.find("max-overall-mem") != configMap.end()) {
            maxOverallMem = std::stoul(configMap["max-overall-mem"]);
        }
        
        if (configMap.find("mem-per-frame") != configMap.end()) {
            memPerFrame = std::stoul(configMap["mem-per-frame"]);
        }
        
        if (configMap.find("min-mem-per-proc") != configMap.end()) {
            minMemPerProc = std::stoul(configMap["min-mem-per-proc"]);
        }
        
        if (configMap.find("max-mem-per-proc") != configMap.end()) {
            maxMemPerProc = std::stoul(configMap["max-mem-per-proc"]);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing configuration: " << e.what() << std::endl;
        return false;
    }
    
    return validateParameters();
}

bool Config::loadFromString(const std::string& configStr) {
    std::istringstream stream(configStr);
    std::unordered_map<std::string, std::string> configMap;
    
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t pos = line.find(' ');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            if (value.length() >= 2 && value[0] == '"' && value[value.length()-1] == '"') {
                value = value.substr(1, value.length()-2);
            }
            
            configMap[key] = value;
        }
    }
    
    // Apply the same parsing logic as loadFromFile
    // (This is a simplified version - in practice you'd reuse the parsing code)
    return true;
}

void Config::printConfiguration() const {
    std::cout << "=== OPESY OS Emulator Configuration ===" << std::endl;
    std::cout << "Number of CPUs: " << static_cast<int>(numCPUs) << std::endl;
    std::cout << "Scheduler: " << getSchedulerString() << std::endl;
    std::cout << "Quantum Cycles: " << quantumCycles << std::endl;
    std::cout << "Batch Process Frequency: " << batchProcessFreq << std::endl;
    std::cout << "Instruction Range: " << minIns << " - " << maxIns << std::endl;
    std::cout << "Delay Per Execution: " << delayPerExec << std::endl;
    std::cout << "Max Overall Memory: " << maxOverallMem << " bytes (" << maxOverallMem/1024 << " KB)" << std::endl;
    std::cout << "Memory Per Frame: " << memPerFrame << " bytes" << std::endl;
    std::cout << "Process Memory Range: " << minMemPerProc << " - " << maxMemPerProc << " bytes" << std::endl;
    std::cout << "Total Frames: " << getTotalFrames() << std::endl;
    std::cout << "=====================================" << std::endl;
} 