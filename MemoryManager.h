#pragma once

#include <vector>
#include <mutex>
#include <string>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <optional>
#include <fstream>
#include <memory>
#include <atomic>
#include "Config.h"

// Forward declarations
class Process;

// Memory segment types
enum class MemorySegment {
    TEXT,
    DATA,
    HEAP
};

// Page table entry structure
struct PageEntry {
    int frameNumber = -1;
    bool isValid = false;
    bool inBackingStore = false;
    bool isDirty = false;
    uint64_t lastAccessTime = 0;
};

// Frame information structure
struct FrameInfo {
    int pid = -1;
    int pageNumber = -1;
    std::vector<std::optional<uint8_t>> data;
    bool isPinned = false;
    bool isDirty = false;
    uint64_t lastAccessTime = 0;
};

// Stored data structure for backing store
struct StoredData {
    std::vector<uint8_t> data;
    int pid;
    int pageNumber;
    bool isDirty;
};

// Page fault result
struct PageFaultResult {
    bool success;
    int frameNumber;
    std::string errorMessage;
};

// Memory allocation result
struct MemoryAllocationResult {
    bool success;
    size_t startAddress;
    size_t size;
    std::string errorMessage;
};

class MemoryManager {
private:
    // Memory configuration
    size_t totalMemorySize;
    size_t frameSize;
    size_t totalFrames;
    
    // Frame table
    std::vector<FrameInfo> frameTable;
    mutable std::mutex frameTableMutex;
    
    // Page tables (per process)
    std::unordered_map<int, std::vector<PageEntry>> pageTables;
    mutable std::mutex pageTableMutex;
    
    // FIFO queue for eviction
    std::vector<int> fifoQueue;
    std::mutex fifoMutex;
    
    // Backing store file
    std::string backingStoreFile;
    std::mutex backingStoreMutex;
    
    // Statistics
    std::atomic<uint64_t> pageFaults;
    std::atomic<uint64_t> pageIns;
    std::atomic<uint64_t> pageOuts;
    std::atomic<uint64_t> totalMemoryAccesses;
    
    // Process memory segments
    std::unordered_map<int, std::unordered_map<MemorySegment, std::pair<size_t, size_t>>> processSegments;
    mutable std::mutex segmentMutex;
    
    // Utility methods
    bool isPowerOfTwo(size_t value) const;
    size_t calculatePageNumber(size_t address) const;
    size_t calculatePageOffset(size_t address) const;
    size_t calculateFrameAddress(int frameNumber) const;
    
    // Page fault handling
    PageFaultResult handlePageFault(int pid, int pageNumber);
    int allocateFrame(int pid, int pageNumber);
    int evictVictimFrame();
    bool loadPageFromBackingStore(int pid, int pageNumber, int frameNumber);
    bool savePageToBackingStore(int pid, int pageNumber, int frameNumber);
    
    // Frame management
    bool isFrameFree(int frameNumber) const;
    void markFrameAsUsed(int frameNumber, int pid, int pageNumber);
    void markFrameAsFree(int frameNumber);
    
    // Backing store operations
    bool writeToBackingStore(const StoredData& data);
    std::optional<StoredData> readFromBackingStore(int pid, int pageNumber);
    void cleanupBackingStore(int pid);
    
    // Memory segmentation
    void setupProcessSegments(int pid, size_t memorySize);
    std::pair<MemorySegment, size_t> translateAddress(int pid, size_t address) const;
    
    // Statistics
    void incrementPageFaults() { pageFaults++; }
    void incrementPageIns() { pageIns++; }
    void incrementPageOuts() { pageOuts++; }
    void incrementMemoryAccesses() { totalMemoryAccesses++; }

public:
    MemoryManager();
    ~MemoryManager();
    
    // Initialization
    void initialize(size_t totalSize);
    
    // Memory allocation and deallocation
    MemoryAllocationResult allocateMemory(int pid, size_t size);
    void deallocateMemory(int pid);
    
    // Memory access operations
    bool readMemory(int pid, size_t address, uint8_t& value);
    bool writeMemory(int pid, size_t address, uint8_t value);
    
    // Page table operations
    bool isPageValid(int pid, int pageNumber) const;
    int getFrameNumber(int pid, int pageNumber) const;
    void setPageDirty(int pid, int pageNumber);
    
    // Statistics and reporting
    uint64_t getPageFaults() const { return pageFaults.load(); }
    uint64_t getPageIns() const { return pageIns.load(); }
    uint64_t getPageOuts() const { return pageOuts.load(); }
    uint64_t getTotalMemoryAccesses() const { return totalMemoryAccesses.load(); }
    double getPageFaultRate() const;
    
    // Memory visualization
    std::string generateMemorySnapshot() const;
    std::string generateFrameMap() const;
    std::string generatePageTableInfo(int pid) const;
    
    // Utility methods
    size_t getTotalMemory() const { return totalMemorySize; }
    size_t getFrameSize() const { return frameSize; }
    size_t getTotalFrames() const { return totalFrames; }
    size_t getUsedFrames() const;
    double getMemoryUtilization() const;
    
    // Process management
    int getProcessCountInMemory() const;
    size_t calculateExternalFragmentation() const;
    
    // Address validation
    bool isValidAddress(int pid, size_t address) const;
    
    // Legacy compatibility methods
    bool allocate(Process& process, size_t required_size);
    void deallocate(int process_id);
    std::string generate_memory_snapshot(const std::vector<Process>& running_processes);
    int get_process_count_in_memory();
};