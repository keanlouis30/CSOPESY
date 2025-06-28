#pragma once
#include <string>

enum Status
{
    READY,
    RUNNING,
    FINISHED
};

inline std::string statusToString(Status status) {
    switch (status) {
        case READY: return "Ready";
        case RUNNING: return "Running";
        case FINISHED: return "Finished";
        default: return "Unknown";
    }
}