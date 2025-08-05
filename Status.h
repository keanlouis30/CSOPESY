#pragma once
#include <string>

enum Status
{
    READY,
    RUNNING,
    BLOCKED,
    FINISHED
};

inline std::string statusToString(Status status) {
    switch (status) {
        case READY: return "Ready";
        case RUNNING: return "Running";
        case BLOCKED: return "Blocked (I/O)"; 
        case FINISHED: return "Finished";
        default: return "Unknown";
    }
}