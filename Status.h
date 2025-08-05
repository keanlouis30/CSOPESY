#pragma once
#include <string>

enum Status
{
    READY,
    RUNNING,
    BLOCKED,
    FINISHED,
    MEMORY_ERROR
};

inline std::string statusToString(Status status) {
    switch (status) {
        case READY: return "Ready";
        case RUNNING: return "Running";
        case BLOCKED: return "Blocked (I/O)"; 
        case FINISHED: return "Finished";
        case MEMORY_ERROR: return "Memory Error";
        default: return "Unknown";
    }
}