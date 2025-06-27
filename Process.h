#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <ctime>
#include "Status.h"
#include "Config.h"
#include "Globals.h"

class Process {
public:
    std::string name;
    int pid;
    std::vector<std::string> commands;
    std::unordered_map<std::string, uint16_t> variables;
    int totalCommands;
    Status status;
    std::string creation_timestamp;
    int assigned_core_id;
    int quantum_remaining;
    int quantum_max;
    int commandCounter;

    Process(std::string n, int p, const Config& config);
    Process(const Process&) = default;
    Process() = default;

private:
    void generate_instructions(const Config& config);
};