// MemoryManager.cpp
#include "MemoryManager.h"
#include "Config.h"
#include "Process.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <fstream>
#include <sstream>

MemoryManager::MemoryManager() 
    : totalMemorySize(0), frameSize(256), totalFrames(0), 
      pageFaults(0), pageIns(0), pageOuts(0), totalMemoryAccesses(0),
      backingStoreFile("csopesy-backing-store.txt") {
}

MemoryManager::~MemoryManager() {
    // Cleanup backing store file
    std::remove(backingStoreFile.c_str());
}

void MemoryManager::initialize(size_t totalSize) {
    totalMemorySize = totalSize;
    frameSize = Config::getInstance().getMemPerFrame();
    totalFrames = totalSize / frameSize;
    
    // Initialize frame table
    frameTable.resize(totalFrames);
    for (auto& frame : frameTable) {
        frame.pid = -1;
        frame.pageNumber = -1;
        frame.data.resize(frameSize);
        frame.isPinned = false;
        frame.isDirty = false;
        frame.lastAccessTime = 0;
    }
    
    // Initialize FIFO queue
    fifoQueue.clear();
    for (int i = 0; i < totalFrames; ++i) {
        fifoQueue.push_back(i);
    }
    
    // Clear backing store file
    std::ofstream file(backingStoreFile, std::ios::trunc);
    file.close();
    
    std::cout << "Memory Manager initialized: " << totalFrames << " frames of " 
              << frameSize << " bytes each\n";
}

bool MemoryManager::isPowerOfTwo(size_t value) const {
    return value > 0 && (value & (value - 1)) == 0;
}

size_t MemoryManager::calculatePageNumber(size_t address) const {
    return address / frameSize;
}

size_t MemoryManager::calculatePageOffset(size_t address) const {
    return address % frameSize;
}

size_t MemoryManager::calculateFrameAddress(int frameNumber) const {
    return frameNumber * frameSize;
}

MemoryAllocationResult MemoryManager::allocateMemory(int pid, size_t size) {
    MemoryAllocationResult result;
    result.success = false;
    result.size = size;
    
    // Validate memory size
    if (!Config::getInstance().validateMemorySize(size)) {
        result.errorMessage = "Invalid memory size";
        return result;
    }
    
    // Setup process segments
    setupProcessSegments(pid, size);
    
    // Create page table for process
    {
        std::lock_guard<std::mutex> lock(pageTableMutex);
        pageTables[pid] = std::vector<PageEntry>();
        size_t numPages = (size + frameSize - 1) / frameSize;
        pageTables[pid].resize(numPages);
    }
    
    result.success = true;
    result.startAddress = 0; // Virtual address space starts at 0
    return result;
}

void MemoryManager::deallocateMemory(int pid) {
    // Remove from frame table
    {
        std::lock_guard<std::mutex> lock(frameTableMutex);
        for (auto& frame : frameTable) {
            if (frame.pid == pid) {
                // Write dirty pages to backing store
                if (frame.isDirty) {
                    savePageToBackingStore(pid, frame.pageNumber, &frame - &frameTable[0]);
                }
                markFrameAsFree(&frame - &frameTable[0]);
            }
        }
    }
    
    // Remove page table
    {
        std::lock_guard<std::mutex> lock(pageTableMutex);
        pageTables.erase(pid);
    }
    
    // Cleanup backing store
    cleanupBackingStore(pid);
    
    // Remove segments
    {
        std::lock_guard<std::mutex> lock(segmentMutex);
        processSegments.erase(pid);
    }
}

bool MemoryManager::readMemory(int pid, size_t address, uint8_t& value) {
    incrementMemoryAccesses();
    
    // Validate address
    if (!isValidAddress(pid, address)) {
        return false;
    }
    
    // Calculate page number
    size_t pageNumber = calculatePageNumber(address);
    
    // Check if page is valid
    if (!isPageValid(pid, pageNumber)) {
        // Page fault
        incrementPageFaults();
        auto result = handlePageFault(pid, pageNumber);
        if (!result.success) {
            return false;
        }
    }
    
    // Get frame number
    int frameNumber = getFrameNumber(pid, pageNumber);
    if (frameNumber == -1) {
        return false;
    }
    
    // Read from frame
    size_t offset = calculatePageOffset(address);
    {
        std::lock_guard<std::mutex> lock(frameTableMutex);
        if (frameNumber < frameTable.size() && offset < frameTable[frameNumber].data.size()) {
            auto& data = frameTable[frameNumber].data[offset];
            if (data.has_value()) {
                value = data.value();
                frameTable[frameNumber].lastAccessTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                return true;
            }
        }
    }
    
    return false;
}

bool MemoryManager::writeMemory(int pid, size_t address, uint8_t value) {
    incrementMemoryAccesses();
    
    // Validate address
    if (!isValidAddress(pid, address)) {
        return false;
    }
    
    // Calculate page number
    size_t pageNumber = calculatePageNumber(address);
    
    // Check if page is valid
    if (!isPageValid(pid, pageNumber)) {
        // Page fault
        incrementPageFaults();
        auto result = handlePageFault(pid, pageNumber);
        if (!result.success) {
            return false;
        }
    }
    
    // Get frame number
    int frameNumber = getFrameNumber(pid, pageNumber);
    if (frameNumber == -1) {
        return false;
    }
    
    // Write to frame
    size_t offset = calculatePageOffset(address);
    {
        std::lock_guard<std::mutex> lock(frameTableMutex);
        if (frameNumber < frameTable.size() && offset < frameTable[frameNumber].data.size()) {
            frameTable[frameNumber].data[offset] = value;
            frameTable[frameNumber].isDirty = true;
            frameTable[frameNumber].lastAccessTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            setPageDirty(pid, pageNumber);
            return true;
        }
    }
    
    return false;
}

bool MemoryManager::isPageValid(int pid, int pageNumber) const {
    std::lock_guard<std::mutex> lock(pageTableMutex);
    auto it = pageTables.find(pid);
    if (it == pageTables.end() || pageNumber >= it->second.size()) {
        return false;
    }
    return it->second[pageNumber].isValid;
}

int MemoryManager::getFrameNumber(int pid, int pageNumber) const {
    std::lock_guard<std::mutex> lock(pageTableMutex);
    auto it = pageTables.find(pid);
    if (it == pageTables.end() || pageNumber >= it->second.size()) {
        return -1;
    }
    return it->second[pageNumber].frameNumber;
}

void MemoryManager::setPageDirty(int pid, int pageNumber) {
    std::lock_guard<std::mutex> lock(pageTableMutex);
    auto it = pageTables.find(pid);
    if (it != pageTables.end() && pageNumber < it->second.size()) {
        it->second[pageNumber].isDirty = true;
    }
}

PageFaultResult MemoryManager::handlePageFault(int pid, int pageNumber) {
    PageFaultResult result;
    result.success = false;
    
    // Try to allocate a frame
    int frameNumber = allocateFrame(pid, pageNumber);
    if (frameNumber == -1) {
        result.errorMessage = "No frames available";
        return result;
    }
    
    // Load page data
    if (!loadPageFromBackingStore(pid, pageNumber, frameNumber)) {
        // Initialize empty page
        std::lock_guard<std::mutex> lock(frameTableMutex);
        for (auto& data : frameTable[frameNumber].data) {
            data = 0;
        }
    }
    
    // Update page table
    {
        std::lock_guard<std::mutex> lock(pageTableMutex);
        auto it = pageTables.find(pid);
        if (it != pageTables.end() && pageNumber < it->second.size()) {
            it->second[pageNumber].isValid = true;
            it->second[pageNumber].frameNumber = frameNumber;
            it->second[pageNumber].inBackingStore = false;
        }
    }
    
    incrementPageIns();
    result.success = true;
    result.frameNumber = frameNumber;
    return result;
}

int MemoryManager::allocateFrame(int pid, int pageNumber) {
    std::lock_guard<std::mutex> lock(frameTableMutex);
    
    // Look for free frame
    for (int i = 0; i < frameTable.size(); ++i) {
        if (isFrameFree(i)) {
            markFrameAsUsed(i, pid, pageNumber);
            return i;
        }
    }
    
    // No free frames, need to evict
    return evictVictimFrame();
}

int MemoryManager::evictVictimFrame() {
    // FIFO eviction
    if (fifoQueue.empty()) {
        return -1;
    }
    
    int victimFrame = fifoQueue.front();
    fifoQueue.erase(fifoQueue.begin());
    
    // Save dirty page to backing store
    if (frameTable[victimFrame].isDirty) {
        savePageToBackingStore(frameTable[victimFrame].pid, 
                             frameTable[victimFrame].pageNumber, 
                             victimFrame);
        incrementPageOuts();
    }
    
    // Invalidate page table entry
    {
        std::lock_guard<std::mutex> lock(pageTableMutex);
        auto it = pageTables.find(frameTable[victimFrame].pid);
        if (it != pageTables.end() && frameTable[victimFrame].pageNumber < it->second.size()) {
            it->second[frameTable[victimFrame].pageNumber].isValid = false;
            it->second[frameTable[victimFrame].pageNumber].inBackingStore = true;
        }
    }
    
    // Mark frame as free
    markFrameAsFree(victimFrame);
    
    return victimFrame;
}

bool MemoryManager::isFrameFree(int frameNumber) const {
    return frameTable[frameNumber].pid == -1;
}

void MemoryManager::markFrameAsUsed(int frameNumber, int pid, int pageNumber) {
    frameTable[frameNumber].pid = pid;
    frameTable[frameNumber].pageNumber = pageNumber;
    frameTable[frameNumber].isPinned = false;
    frameTable[frameNumber].isDirty = false;
    frameTable[frameNumber].lastAccessTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void MemoryManager::markFrameAsFree(int frameNumber) {
    frameTable[frameNumber].pid = -1;
    frameTable[frameNumber].pageNumber = -1;
    frameTable[frameNumber].isPinned = false;
    frameTable[frameNumber].isDirty = false;
    frameTable[frameNumber].lastAccessTime = 0;
}

bool MemoryManager::loadPageFromBackingStore(int pid, int pageNumber, int frameNumber) {
    auto data = readFromBackingStore(pid, pageNumber);
    if (!data.has_value()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(frameTableMutex);
    if (frameNumber < frameTable.size()) {
        auto& frame = frameTable[frameNumber];
        frame.data.clear();
        frame.data.resize(frameSize);
        
        for (size_t i = 0; i < std::min(data->data.size(), frameSize); ++i) {
            frame.data[i] = data->data[i];
        }
        return true;
    }
    return false;
}

bool MemoryManager::savePageToBackingStore(int pid, int pageNumber, int frameNumber) {
    std::lock_guard<std::mutex> lock(frameTableMutex);
    if (frameNumber >= frameTable.size()) {
        return false;
    }
    
    StoredData data;
    data.pid = pid;
    data.pageNumber = pageNumber;
    data.isDirty = frameTable[frameNumber].isDirty;
    
    for (const auto& byte : frameTable[frameNumber].data) {
        if (byte.has_value()) {
            data.data.push_back(byte.value());
        } else {
            data.data.push_back(0);
        }
    }
    
    return writeToBackingStore(data);
}

bool MemoryManager::writeToBackingStore(const StoredData& data) {
    std::lock_guard<std::mutex> lock(backingStoreMutex);
    std::ofstream file(backingStoreFile, std::ios::app | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file << data.pid << " " << data.pageNumber << " " << data.isDirty << "\n";
    file.write(reinterpret_cast<const char*>(data.data.data()), data.data.size());
    file << "\n";
    
    return true;
}

std::optional<StoredData> MemoryManager::readFromBackingStore(int pid, int pageNumber) {
    std::lock_guard<std::mutex> lock(backingStoreMutex);
    std::ifstream file(backingStoreFile);
    if (!file.is_open()) {
        return std::nullopt;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        int storedPid, storedPageNumber;
        bool isDirty;
        
        if (iss >> storedPid >> storedPageNumber >> isDirty) {
            if (storedPid == pid && storedPageNumber == pageNumber) {
                StoredData data;
                data.pid = pid;
                data.pageNumber = pageNumber;
                data.isDirty = isDirty;
                
                // Read data
                std::string dataLine;
                if (std::getline(file, dataLine)) {
                    data.data.resize(dataLine.size());
                    std::copy(dataLine.begin(), dataLine.end(), data.data.begin());
                    return data;
                }
            }
        }
    }
    
    return std::nullopt;
}

void MemoryManager::cleanupBackingStore(int pid) {
    std::lock_guard<std::mutex> lock(backingStoreMutex);
    
    // Read all data except for the specified PID
    std::vector<StoredData> keepData;
    std::ifstream file(backingStoreFile);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            int storedPid, storedPageNumber;
            bool isDirty;
            
            if (iss >> storedPid >> storedPageNumber >> isDirty) {
                if (storedPid != pid) {
                    StoredData data;
                    data.pid = storedPid;
                    data.pageNumber = storedPageNumber;
                    data.isDirty = isDirty;
                    
                    std::string dataLine;
                    if (std::getline(file, dataLine)) {
                        data.data.resize(dataLine.size());
                        std::copy(dataLine.begin(), dataLine.end(), data.data.begin());
                        keepData.push_back(data);
                    }
                } else {
                    // Skip data line for this PID
                    std::string dataLine;
                    std::getline(file, dataLine);
                }
            }
        }
    }
    
    // Rewrite file without the specified PID
    std::ofstream outFile(backingStoreFile, std::ios::trunc);
    if (outFile.is_open()) {
        for (const auto& data : keepData) {
            outFile << data.pid << " " << data.pageNumber << " " << data.isDirty << "\n";
            outFile.write(reinterpret_cast<const char*>(data.data.data()), data.data.size());
            outFile << "\n";
        }
    }
}

void MemoryManager::setupProcessSegments(int pid, size_t memorySize) {
    std::lock_guard<std::mutex> lock(segmentMutex);
    
    // Simple segmentation: 50% TEXT, 30% DATA, 20% HEAP
    size_t textSize = memorySize * 5 / 10;
    size_t dataSize = memorySize * 3 / 10;
    size_t heapSize = memorySize * 2 / 10;
    
    processSegments[pid][MemorySegment::TEXT] = {0, textSize};
    processSegments[pid][MemorySegment::DATA] = {textSize, textSize + dataSize};
    processSegments[pid][MemorySegment::HEAP] = {textSize + dataSize, memorySize};
}

std::pair<MemorySegment, size_t> MemoryManager::translateAddress(int pid, size_t address) const {
    std::lock_guard<std::mutex> lock(segmentMutex);
    auto it = processSegments.find(pid);
    if (it == processSegments.end()) {
        return {MemorySegment::TEXT, 0};
    }
    
    const auto& segments = it->second;
    for (const auto& [segment, bounds] : segments) {
        if (address >= bounds.first && address < bounds.second) {
            return {segment, address - bounds.first};
        }
    }
    
    return {MemorySegment::TEXT, 0};
}

bool MemoryManager::isValidAddress(int pid, size_t address) const {
    std::lock_guard<std::mutex> lock(segmentMutex);
    auto it = processSegments.find(pid);
    if (it == processSegments.end()) {
        return false;
    }
    
    const auto& segments = it->second;
    for (const auto& [segment, bounds] : segments) {
        if (address >= bounds.first && address < bounds.second) {
            return true;
        }
    }
    
    return false;
}

double MemoryManager::getPageFaultRate() const {
    uint64_t accesses = totalMemoryAccesses.load();
    if (accesses == 0) return 0.0;
    return static_cast<double>(pageFaults.load()) / static_cast<double>(accesses) * 100.0;
}

size_t MemoryManager::getUsedFrames() const {
    std::lock_guard<std::mutex> lock(frameTableMutex);
    size_t count = 0;
    for (const auto& frame : frameTable) {
        if (!isFrameFree(&frame - &frameTable[0])) {
            count++;
        }
    }
    return count;
}

double MemoryManager::getMemoryUtilization() const {
    if (totalFrames == 0) return 0.0;
    return static_cast<double>(getUsedFrames()) / static_cast<double>(totalFrames) * 100.0;
}

int MemoryManager::getProcessCountInMemory() const {
    std::lock_guard<std::mutex> lock(pageTableMutex);
    return pageTables.size();
}

size_t MemoryManager::calculateExternalFragmentation() const {
    // In a paging system, external fragmentation is minimal
    // This is a simplified calculation
    return totalMemorySize - (getUsedFrames() * frameSize);
}

std::string MemoryManager::generateMemorySnapshot() const {
    std::stringstream ss;
    ss << "Memory Snapshot:\n";
    ss << "Total Memory: " << totalMemorySize << " bytes\n";
    ss << "Frame Size: " << frameSize << " bytes\n";
    ss << "Total Frames: " << totalFrames << "\n";
    ss << "Used Frames: " << getUsedFrames() << "\n";
    ss << "Memory Utilization: " << std::fixed << std::setprecision(2) << getMemoryUtilization() << "%\n";
    return ss.str();
}

std::string MemoryManager::generateFrameMap() const {
    std::stringstream ss;
    ss << "Frame Map:\n";
    ss << "Frame\tPID\tPage\tDirty\tLast Access\n";
    
    std::lock_guard<std::mutex> lock(frameTableMutex);
    for (size_t i = 0; i < frameTable.size(); ++i) {
        const auto& frame = frameTable[i];
        ss << i << "\t";
        if (frame.pid == -1) {
            ss << "FREE\t-\t-\t-\n";
        } else {
            ss << frame.pid << "\t" << frame.pageNumber << "\t" 
               << (frame.isDirty ? "Yes" : "No") << "\t" << frame.lastAccessTime << "\n";
        }
    }
    return ss.str();
}

std::string MemoryManager::generatePageTableInfo(int pid) const {
    std::stringstream ss;
    ss << "Page Table for PID " << pid << ":\n";
    ss << "Page\tFrame\tValid\tDirty\tBacking Store\n";
    
    std::lock_guard<std::mutex> lock(pageTableMutex);
    auto it = pageTables.find(pid);
    if (it != pageTables.end()) {
        for (size_t i = 0; i < it->second.size(); ++i) {
            const auto& entry = it->second[i];
            ss << i << "\t" << entry.frameNumber << "\t" 
               << (entry.isValid ? "Yes" : "No") << "\t"
               << (entry.isDirty ? "Yes" : "No") << "\t"
               << (entry.inBackingStore ? "Yes" : "No") << "\n";
        }
    }
    return ss.str();
}

// Legacy compatibility methods
bool MemoryManager::allocate(Process& process, size_t required_size) {
    auto result = allocateMemory(process.getPID(), required_size);
    if (result.success) {
        process.setMemoryStartAddress(result.startAddress);
        process.setMemorySize(result.size);
    }
    return result.success;
}

void MemoryManager::deallocate(int process_id) {
    deallocateMemory(process_id);
}

int MemoryManager::get_process_count_in_memory() {
    return getProcessCountInMemory();
}

std::string MemoryManager::generate_memory_snapshot(const std::vector<Process>& running_processes) {
    return generateMemorySnapshot();
}