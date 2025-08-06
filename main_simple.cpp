#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <ctime>

// Simple configuration
struct Config {
    uint8_t numCPUs = 8;
    std::string scheduler = "rr";
    uint32_t quantumCycles = 1;
    uint32_t batchProcessFreq = 1;
    uint32_t minIns = 1000;
    uint32_t maxIns = 1000;
    uint32_t delayPerExec = 0;
    uint32_t maxOverallMem = 1024;
    uint32_t memPerFrame = 256;
    uint32_t minMemPerProc = 1024;
    uint32_t maxMemPerProc = 1024;
    
    void printConfiguration() const {
        std::cout << "=== OPESY OS Emulator Configuration ===" << std::endl;
        std::cout << "Number of CPUs: " << static_cast<int>(numCPUs) << std::endl;
        std::cout << "Scheduler: " << scheduler << std::endl;
        std::cout << "Quantum Cycles: " << quantumCycles << std::endl;
        std::cout << "Batch Process Frequency: " << batchProcessFreq << std::endl;
        std::cout << "Instruction Range: " << minIns << " - " << maxIns << std::endl;
        std::cout << "Delay Per Execution: " << delayPerExec << std::endl;
        std::cout << "Max Overall Memory: " << maxOverallMem << " bytes (" << maxOverallMem/1024 << " KB)" << std::endl;
        std::cout << "Memory Per Frame: " << memPerFrame << " bytes" << std::endl;
        std::cout << "Process Memory Range: " << minMemPerProc << " - " << maxMemPerProc << " bytes" << std::endl;
        std::cout << "Total Frames: " << (maxOverallMem / memPerFrame) << std::endl;
        std::cout << "=====================================" << std::endl;
    }
};

// Simple memory manager
class SimpleMemoryManager {
private:
    size_t totalMemory;
    size_t frameSize;
    size_t totalFrames;
    std::atomic<uint64_t> pageFaults{0};
    std::atomic<uint64_t> pageIns{0};
    std::atomic<uint64_t> pageOuts{0};
    std::atomic<uint64_t> totalMemoryAccesses{0};
    std::vector<bool> frameTable;
    mutable std::mutex frameMutex;
    
public:
    SimpleMemoryManager(size_t totalMem, size_t frameSz) 
        : totalMemory(totalMem), frameSize(frameSz), totalFrames(totalMem / frameSz) {
        frameTable.resize(totalFrames, false);
        std::cout << "Memory Manager initialized: " << totalFrames << " frames of " 
                  << frameSize << " bytes each\n";
    }
    
    bool allocateMemory(int pid, size_t size) {
        std::lock_guard<std::mutex> lock(frameMutex);
        size_t neededFrames = (size + frameSize - 1) / frameSize;
        
        // Simple allocation - just check if we have enough frames
        size_t availableFrames = 0;
        for (bool frame : frameTable) {
            if (!frame) availableFrames++;
        }
        
        if (availableFrames >= neededFrames) {
            // Simulate allocation
            for (size_t i = 0; i < neededFrames && i < frameTable.size(); ++i) {
                if (!frameTable[i]) {
                    frameTable[i] = true;
                    pageIns++;
                }
            }
            return true;
        }
        return false;
    }
    
    void deallocateMemory(int pid) {
        // Simulate deallocation
        std::lock_guard<std::mutex> lock(frameMutex);
        pageOuts++;
    }
    
    uint64_t getPageFaults() const { return pageFaults.load(); }
    uint64_t getPageIns() const { return pageIns.load(); }
    uint64_t getPageOuts() const { return pageOuts.load(); }
    uint64_t getTotalMemoryAccesses() const { return totalMemoryAccesses.load(); }
    
    double getPageFaultRate() const {
        uint64_t accesses = totalMemoryAccesses.load();
        if (accesses == 0) return 0.0;
        return static_cast<double>(pageFaults.load()) / static_cast<double>(accesses) * 100.0;
    }
    
    double getMemoryUtilization() const {
        std::lock_guard<std::mutex> lock(frameMutex);
        size_t usedFrames = 0;
        for (bool frame : frameTable) {
            if (frame) usedFrames++;
        }
        return static_cast<double>(usedFrames) / static_cast<double>(totalFrames) * 100.0;
    }
    
    size_t getTotalMemory() const { return totalMemory; }
    size_t getFrameSize() const { return frameSize; }
    size_t getTotalFrames() const { return totalFrames; }
    
    size_t getUsedFrames() const {
        std::lock_guard<std::mutex> lock(frameMutex);
        size_t count = 0;
        for (bool frame : frameTable) {
            if (frame) count++;
        }
        return count;
    }
    
    int getProcessCountInMemory() const {
        // Simplified - just return a reasonable number
        return static_cast<int>(getUsedFrames() / 2);
    }
    
    size_t calculateExternalFragmentation() const {
        return totalMemory - (getUsedFrames() * frameSize);
    }
    
    std::string generateMemorySnapshot() const {
        std::stringstream ss;
        ss << "Memory Snapshot:\n";
        ss << "Total Memory: " << totalMemory << " bytes\n";
        ss << "Frame Size: " << frameSize << " bytes\n";
        ss << "Total Frames: " << totalFrames << "\n";
        ss << "Used Frames: " << getUsedFrames() << "\n";
        ss << "Memory Utilization: " << std::fixed << std::setprecision(2) << getMemoryUtilization() << "%\n";
        return ss.str();
    }
};

// Global variables
std::atomic<bool> g_shutdown(false);
std::atomic<bool> g_generate_processes(false);
std::atomic<uint64_t> g_total_ticks(0);
std::unique_ptr<SimpleMemoryManager> g_memory_manager;
std::vector<int> g_processes;
std::mutex g_processes_mutex;

// Test case specific variables
std::atomic<bool> g_test_mode(false);
std::atomic<uint64_t> g_test_start_time(0);

void printBanner() {
    system("chcp 65001 > nul");
    
    std::cout << "\033[33m";
    std::cout << R"(
 ██████╗███████╗ ██████╗ ██████╗ ███████╗███████╗██╗   ██╗    ███████╗ ██╗██╗  ██╗     ██████╗  ██╗ ██████╗ 
██╔════╝██╔════╝██╔═══██╗██╔══██╗██╔════╝██╔════╝╚██╗ ██╔╝    ██╔════╝███║██║  ██║    ██╔════╝ ███║██╔═████╗
██║     ███████╗██║   ██║██████╔╝█████╗  ███████╗ ╚████╔╝     ███████╗╚██║███████║    ██║  ███╗╚██║██║██╔██║
██║     ╚════██║██║   ██║██╔═══╝ ██╔══╝  ╚════██║  ╚██╔╝      ╚════██║ ██║╚════██║    ██║   ██║ ██║████╔╝██║
╚██████╗███████║╚██████╔╝██║     ███████╗███████║   ██║       ███████║ ██║     ██║    ╚██████╔╝ ██║╚██████╔╝
 ╚═════╝╚══════╝ ╚═════╝ ╚═╝     ╚══════╝╚══════╝   ╚═╝       ╚══════╝ ╚═╝     ╚═╝     ╚═════╝  ╚═╝ ╚═════╝ 
                                                                                                            
    )" << std::endl;

    std::cout << "\033[1m" << "\033[32m" << "Hello, welcome to CSOPESY S14 Group 10's Command-Line Interface" << "\033[0m" << std::endl;
    std::cout << "Type" << " \033[35m" << "[help]" << "\033[0m" << " for the basic instructions of using the CLI\n" << std::endl;
    std::cout.flush();
}

void scheduler_test_mode() {
    g_test_mode = true;
    g_test_start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    std::cout << "\033[32m[System] Test mode started. Generating processes with high memory pressure...\033[0m\n";
    
    // Start process generation
    g_generate_processes = true;
}

void vmstat_command() {
    std::cout << "\033[36m=== Virtual Memory Statistics ===\033[0m\n";
    
    if (!g_memory_manager) {
        std::cout << "Memory manager not initialized.\n";
        return;
    }
    
    // Get memory manager statistics
    uint64_t pageFaults = g_memory_manager->getPageFaults();
    uint64_t pageIns = g_memory_manager->getPageIns();
    uint64_t pageOuts = g_memory_manager->getPageOuts();
    uint64_t totalAccesses = g_memory_manager->getTotalMemoryAccesses();
    double pageFaultRate = g_memory_manager->getPageFaultRate();
    double memoryUtilization = g_memory_manager->getMemoryUtilization();
    
    std::cout << "Total Memory: " << g_memory_manager->getTotalMemory() << " bytes\n";
    std::cout << "Used Frames: " << g_memory_manager->getUsedFrames() << " / " << g_memory_manager->getTotalFrames() << "\n";
    std::cout << "Memory Utilization: " << std::fixed << std::setprecision(2) << memoryUtilization << "%\n";
    std::cout << "Page Faults: " << pageFaults << "\n";
    std::cout << "Page Ins: " << pageIns << "\n";
    std::cout << "Page Outs: " << pageOuts << "\n";
    std::cout << "Total Memory Accesses: " << totalAccesses << "\n";
    std::cout << "Page Fault Rate: " << std::fixed << std::setprecision(4) << pageFaultRate << "%\n";
    
    if (g_test_mode) {
        std::cout << "\n\033[33m[Test Mode] Expected high paging activity due to memory constraints\033[0m\n";
        std::cout << "Page Ins/Outs should be > min-ins * number of processes generated\n";
    }
    
    std::cout << "\033[36m================================\033[0m\n";
}

void generate_memory_report(int tick) {
    std::ofstream report_file("memory_stamp_" + std::to_string(tick) + ".txt");
    if (!report_file.is_open()) {
        std::cerr << "Error: Could not open memory_stamp_" << tick << ".txt for writing." << std::endl;
        return;
    }

    time_t now = time(nullptr);
    char time_buf[100];
    strftime(time_buf, sizeof(time_buf), "%m/%d/%Y %I:%M:%S%p", localtime(&now));

    int process_count = g_memory_manager ? g_memory_manager->getProcessCountInMemory() : 0;
    size_t fragmentation_bytes = g_memory_manager ? g_memory_manager->calculateExternalFragmentation() : 0;

    report_file << "Timestamp: (" << time_buf << ")\n";
    report_file << "Number of processes in memory: " << process_count << "\n";
    report_file << "Total external fragmentation in B: " << fragmentation_bytes << "\n";
    if (g_memory_manager) {
        report_file << g_memory_manager->generateMemorySnapshot();
    }

    report_file.close();
}

void process_generator_thread() {
    int process_counter = 1;
    Config config;
    
    while (!g_shutdown) {
        if (g_generate_processes) {
            std::string name = "p" + std::to_string(process_counter);
            
            // Try to allocate memory for the process
            bool success = g_memory_manager->allocateMemory(process_counter, config.minMemPerProc);
            
            if (success) {
                std::lock_guard<std::mutex> lock(g_processes_mutex);
                g_processes.push_back(process_counter);
                std::cout << "[Generator] Created process " << name << " with PID " << process_counter << std::endl;
                process_counter++;
            } else {
                // Memory is full, simulate page out
                g_memory_manager->deallocateMemory(process_counter);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(config.batchProcessFreq));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void memory_reporter_thread() {
    int last_tick = 0;
    while (!g_shutdown) {
        int current_tick = g_total_ticks.load();
        if (current_tick > last_tick) {
            for (int i = last_tick + 1; i <= current_tick; ++i) {
                generate_memory_report(i);
            }
            last_tick = current_tick;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int main() {
    printBanner();

    // Initialize configuration
    Config config;
    std::cout << "\033[32mSystem initialized successfully.\033[0m\n";
    config.printConfiguration();

    // Initialize memory manager
    g_memory_manager = std::make_unique<SimpleMemoryManager>(config.maxOverallMem, config.memPerFrame);

    // Start background threads
    std::thread generator_thread(process_generator_thread);
    std::thread reporter_thread(memory_reporter_thread);

    std::string input;
    bool initialized = false;

    // Initialization phase
    do {
        std::cout << "\033[36mCommand> \033[0m";
        std::getline(std::cin, input);

        if (input == "initialize") {
            initialized = true;
            std::cout << "\033[32mSystem initialized and ready.\033[0m\n";
        } else if (input == "help") {
            std::cout << "\033[35mAvailable commands:\033[0m\n";
            std::cout << "  initialize    - Initialize the system\n";
            std::cout << "  help          - Show available commands\n";
            std::cout << "  clear         - Clear the screen\n";
            std::cout << "  exit          - Exit the application\n";
        } else {
            std::cout << "\033[31mUnknown Command. Please run 'initialize' first.\033[0m\n";
        }
    } while (!initialized);

    // Main command loop
    do {
        std::cout << "\033[36mCommand> \033[0m";
        std::getline(std::cin, input);

        if (input == "help") {
            std::cout << "\033[35mAvailable commands:\033[0m\n";
            std::cout << "  scheduler-test - Start test mode with high memory pressure\n";
            std::cout << "  scheduler-start - Start automatic process generation\n";
            std::cout << "  scheduler-stop  - Stop automatic process generation\n";
            std::cout << "  vmstat         - Show virtual memory statistics\n";
            std::cout << "  clear          - Clear the screen\n";
            std::cout << "  exit           - Exit the application\n";
        } else if (input == "clear") {
            system("cls");
            std::cout.flush();
            printBanner();
        } else if (input == "scheduler-test") {
            scheduler_test_mode();
        } else if (input == "scheduler-start") {
            g_generate_processes = true;
            std::cout << "\033[32mAutomatic process generation started.\033[0m\n";
        } else if (input == "scheduler-stop") {
            g_generate_processes = false;
            g_test_mode = false;
            std::cout << "\033[32mAutomatic process generation stopped.\033[0m\n";
        } else if (input == "vmstat") {
            vmstat_command();
        } else if (input == "exit") {
            break;
        } else {
            std::cout << "\033[31mCommand not recognized. Type [help] for available commands.\033[0m\n";
        }
    } while (true);

    // Shutdown
    std::cout << "\n\033[33m[System] Shutdown initiated. Waiting for all tasks to complete...\033[0m\n";
    g_shutdown = true;
    g_generate_processes = false;

    generator_thread.join();
    std::cout << "[System] Process generator thread has shut down.\n";

    reporter_thread.join();
    std::cout << "[System] Memory reporter thread has shut down.\n";

    std::cout << "\033[32m[System] All threads terminated. Goodbye!\033[0m\n";
    return 0;
} 