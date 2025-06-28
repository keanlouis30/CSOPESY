#include "Process.h"
#include "Utils.h"
#include <random>
#include <sstream>
#include <iostream>
#include <functional>


Process::Process(std::string n, int p, const Config& config)
: name(std::move(n)), pid(p), status(READY), commandCounter(0), assigned_core_id(-1) {
    
    // Set creation timestamp
    time_t now = time(nullptr);
    char buf[100];
    strftime(buf, sizeof(buf), "%m/%d/%Y %I:%M:%S%p", localtime(&now));
    creation_timestamp = buf;

    // Always add variable 'x' with value 0
    variables["x"] = 0;

    // Generate instructions
    generate_instructions(config);
    totalCommands = commands.size();

    // Set quantum for RR
    quantum_max = config.quantum_cycles;
    quantum_remaining = quantum_max;
}

void Process::generate_instructions(const Config& config) {
    int num_instructions = get_random(config.min_ins, config.max_ins);
    commands.clear();
    // Alternate PRINT and ADD instructions
    for (int i = 0; i < num_instructions; ++i) {
        std::stringstream ss;
        if (i % 2 == 0) {
            ss << "PRINT Value from: x";
        } else {
            int add_val = get_random(1, 10);
            ss << "ADD x x " << add_val;
        }
        commands.push_back(ss.str());
    }
    totalCommands = commands.size();
}