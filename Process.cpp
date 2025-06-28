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

    // Generate instructions
    generate_instructions(config);
    totalCommands = commands.size();

    // Set quantum for RR
    quantum_max = config.quantum_cycles;
    quantum_remaining = quantum_max;
}

void Process::generate_instructions(const Config& config) {
    int num_instructions = get_random(config.min_ins, config.max_ins);
    int max_nesting = 3;
    int total_generated = 0;
    int expanded_count = 0; // Track actual expanded instructions
    
    auto get_unique_var = [this](int& var_counter) {
        std::string var_name;
        do {
            var_name = "var_" + std::to_string(var_counter++);
        } while (variables.find(var_name) != variables.end());
        return var_name;
    };

    std::function<void(int, int&, int&)> generate;
    generate = [&](int nesting, int& remaining, int& expanded) {
        int var_counter = (int)variables.size();
        while (remaining > 0 && expanded < num_instructions) {
            int instruction_type = get_random(0, 5); // 0:PRINT, 1:DECLARE, 2:ADD, 3:SUBTRACT, 4:SLEEP, 5:FOR
            std::stringstream ss;
            switch (instruction_type) {
                case 0: { // PRINT
                    int print_type = get_random(0, 2);
                    if (print_type == 0) { // PRINT value
                        ss << "PRINT " << get_random(0, 1000);
                    } else if (print_type == 1 && !variables.empty()) { // PRINT var
                        auto it = variables.begin();
                        std::advance(it, get_random(0, (int)variables.size() - 1));
                        ss << "PRINT " << it->first;
                    } else { // PRINT var, value
                        std::string var_name;
                        if (!variables.empty()) {
                            auto it = variables.begin();
                            std::advance(it, get_random(0, (int)variables.size() - 1));
                            var_name = it->first;
                        } else {
                            var_name = get_unique_var(var_counter);
                            uint16_t value = get_random(0, 1000);
                            variables[var_name] = value;
                        }
                        ss << "PRINT " << var_name << ", " << get_random(0, 1000);
                    }
                    commands.push_back(ss.str());
                    --remaining;
                    ++expanded;
                    break;
                }
                case 1: { // DECLARE
                    std::string var_name = get_unique_var(var_counter);
                    uint16_t value = get_random(0, 1000);
                    variables[var_name] = value;
                    ss << "DECLARE " << var_name << " " << value;
                    commands.push_back(ss.str());
                    --remaining;
                    ++expanded;
                    break;
                }
                case 2: // ADD
                case 3: { // SUBTRACT
                    std::string op = (instruction_type == 2) ? "ADD" : "SUBTRACT";
                    int add_type = get_random(0, 3);
                    std::string arg1, arg2;
                    // Helper to get or create a variable
                    auto get_or_create_var = [&](int& var_counter) {
                        if (!variables.empty() && get_random(0, 1)) {
                            auto it = variables.begin();
                            std::advance(it, get_random(0, (int)variables.size() - 1));
                            return it->first;
                        } else {
                            std::string v = get_unique_var(var_counter);
                            uint16_t value = get_random(0, 1000);
                            variables[v] = value;
                            return v;
                        }
                    };
                    switch (add_type) {
                        case 0: // (var, value)
                            arg1 = get_or_create_var(var_counter);
                            arg2 = std::to_string(get_random(0, 1000));
                            break;
                        case 1: // (value, var)
                            arg1 = std::to_string(get_random(0, 1000));
                            arg2 = get_or_create_var(var_counter);
                            break;
                        case 2: // (var, var)
                            arg1 = get_or_create_var(var_counter);
                            arg2 = get_or_create_var(var_counter);
                            break;
                        case 3: // (value, value)
                            arg1 = std::to_string(get_random(0, 1000));
                            arg2 = std::to_string(get_random(0, 1000));
                            break;
                    }
                    ss << op << " " << arg1 << " " << arg2;
                    commands.push_back(ss.str());
                    --remaining;
                    ++expanded;
                    break;
                }
                case 4: { // SLEEP
                    int max_sleep = std::min(255, remaining - 1); 
                    if (max_sleep <= 0) {
                        max_sleep = 1;
                    }
                    uint8_t sleep_ticks = get_random(1, max_sleep);
                    ss << "SLEEP " << (int)sleep_ticks;
                    commands.push_back(ss.str());
                    --remaining;
                    ++expanded;
                    break;
                }
                case 5: { // FOR
                    if (nesting < max_nesting && remaining > 1) {
                        // Check if we have enough remaining instructions for a FOR loop
                        // We need at least 3: FOR instruction, 1 body instruction, closing brace
                        if (remaining >= 3 && expanded < num_instructions - 2) {
                            int for_iterations = get_random(1, 2); // Keep iterations very low
                            int body_instructions = 1; // Keep body size minimal
                            
                            // Check if this would exceed our expanded count
                            int potential_expansion = 1 + (body_instructions * for_iterations) + 1; // FOR + body*iterations + closing
                            if (expanded + potential_expansion <= num_instructions) {
                                ss << "FOR " << for_iterations << " {";
                                commands.push_back(ss.str());
                                --remaining;
                                ++expanded;
                                
                                int body_remaining = body_instructions;
                                int body_expanded = 0;
                                generate(nesting + 1, body_remaining, body_expanded);
                                commands.push_back("}");
                                remaining -= (body_instructions - body_remaining);
                                expanded += (body_expanded * for_iterations); // Account for loop expansion
                            }
                        }
                    }
                    break;
                }
            }
        }
    };
    int remaining = num_instructions;
    generate(0, remaining, expanded_count);
    totalCommands = commands.size();
}