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
    int min_instructions = config.min_ins;
    int max_instructions = config.max_ins;
    int max_nesting = 3;
    int expanded_count = 0;
    commands.clear();

    auto get_unique_var = [this](int& var_counter) {
        std::string var_name;
        do {
            var_name = "var_" + std::to_string(var_counter++);
        } while (variables.find(var_name) != variables.end());
        return var_name;
    };

    auto get_random_var = [this]() -> std::string {
        if (variables.empty()) return "";
        auto it = variables.begin();
        std::advance(it, get_random(0, static_cast<int>(variables.size()) - 1));
        return it->first;
    };

    std::function<void(int, int&, int&)> generate;
    generate = [&](int nesting, int& remaining, int& expanded) {
        int var_counter = static_cast<int>(variables.size());
        while (remaining > 0 && expanded < num_instructions) {
            bool can_try_for = (nesting < max_nesting && remaining >= 3 && expanded < num_instructions - 2);
            int instruction_type = get_random(0, can_try_for ? 5 : 4);
            std::stringstream ss;

            switch (instruction_type) {
                case 0: { // PRINT
                    int print_type = get_random(0, 2);
                    if (print_type == 0) {
                        ss << "PRINT " << get_random(0, 1000);
                    } else if (print_type == 1 && !variables.empty()) {
                        ss << "PRINT " << get_random_var();
                    } else {
                        std::string var_name = get_random_var();
                        if (var_name.empty()) {
                            var_name = get_unique_var(var_counter);
                            variables[var_name] = get_random(0, 1000);
                        }
                        ss << "PRINT " << var_name << ", " << get_random(0, 1000);
                    }
                    commands.emplace_back(ss.str());
                    --remaining;
                    ++expanded;
                    break;
                }
                case 1: { // DECLARE
                    std::string var_name = get_unique_var(var_counter);
                    uint16_t value = get_random(0, 1000);
                    variables[var_name] = value;
                    ss << "DECLARE " << var_name << " " << value;
                    commands.emplace_back(ss.str());
                    --remaining;
                    ++expanded;
                    break;
                }
                case 2: // ADD
                case 3: { // SUBTRACT
                    std::string op = (instruction_type == 2) ? "ADD" : "SUBTRACT";
                    int add_type = get_random(0, 3);
                    std::string arg1, arg2;
                    auto get_or_create_var = [&](int& var_counter) {
                        if (!variables.empty() && get_random(0, 1)) {
                            return get_random_var();
                        } else {
                            std::string v = get_unique_var(var_counter);
                            variables[v] = get_random(0, 1000);
                            return v;
                        }
                    };
                    switch (add_type) {
                        case 0: arg1 = get_or_create_var(var_counter); arg2 = std::to_string(get_random(0, 1000)); break;
                        case 1: arg1 = std::to_string(get_random(0, 1000)); arg2 = get_or_create_var(var_counter); break;
                        case 2: arg1 = get_or_create_var(var_counter); arg2 = get_or_create_var(var_counter); break;
                        case 3: arg1 = std::to_string(get_random(0, 1000)); arg2 = std::to_string(get_random(0, 1000)); break;
                    }
                    ss << op << " " << arg1 << " " << arg2;
                    commands.emplace_back(ss.str());
                    --remaining;
                    ++expanded;
                    break;
                }
                case 4: { // SLEEP
                    int max_sleep = std::min(255, remaining - 1);
                    if (max_sleep <= 0) max_sleep = 1;
                    ss << "SLEEP " << get_random(1, max_sleep);
                    commands.emplace_back(ss.str());
                    --remaining;
                    ++expanded;
                    break;
                }
                case 5: { // FOR
                    bool for_generated = false;
                    for (int for_iterations = 2; for_iterations >= 1; --for_iterations) {
                        for (int body_instructions = 1; body_instructions <= 2; ++body_instructions) {
                            int potential_expansion = 1 + (body_instructions * for_iterations) + 1;
                            if (potential_expansion <= (num_instructions - expanded) && potential_expansion <= remaining) {
                                ss << "FOR " << for_iterations << " {";
                                commands.emplace_back(ss.str());
                                --remaining;
                                ++expanded;
                                int body_remaining = body_instructions;
                                int body_expanded = 0;
                                generate(nesting + 1, body_remaining, body_expanded);
                                commands.emplace_back("}");
                                remaining -= (body_instructions - body_remaining);
                                expanded += (body_expanded * for_iterations) + 1;
                                for_generated = true;
                                break;
                            }
                        }
                        if (for_generated) break;
                    }
                    if (!for_generated) {
                        instruction_type = get_random(0, 4);
                        continue;
                    }
                    break;
                }
            }
        }
    };

    int remaining = num_instructions;
    generate(0, remaining, expanded_count);

    // Ensure minimum instruction count
    while ((int)commands.size() < min_instructions) {
        int instruction_type = get_random(0, 3);
        std::stringstream ss;
        switch (instruction_type) {
            case 0: ss << "PRINT " << get_random(0, 1000); break;
            case 1: ss << "DECLARE var_" << commands.size() << " " << get_random(0, 1000); break;
            case 2: ss << "ADD var_" << commands.size() << " " << get_random(0, 1000); break;
            case 3: ss << "SUBTRACT var_" << commands.size() << " " << get_random(0, 1000); break;
        }
        commands.emplace_back(ss.str());
    }
    // Ensure maximum instruction count
    if ((int)commands.size() > max_instructions) {
        commands.resize(max_instructions);
    }
    totalCommands = commands.size();
}