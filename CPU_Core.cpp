#include "CPU_Core.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Helper to parse a string
std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// Helper to trim quotes
std::string trim_quotes(const std::string& str) {
    if (str.length() >= 2 && str.front() == '"' && str.back() == '"') {
        return str.substr(1, str.length() - 2);
    }
    return str;
}

void CPU_Core::execute_command(Process &p)
{
    if (p.commandCounter >= p.commands.size()) return;

    std::string command_str = p.commands[p.commandCounter];
    std::vector<std::string> parts = split(command_str, ' ');
    std::string command = parts[0];

    std::ofstream outfile(p.name + "_log.txt", std::ios_base::app);

//     if (command == "DECLARE") {
//         if (parts.size() == 3) {
//             p.variables[parts[1]] = std::stoi(parts[2]);
//         }
//     } else if (command == "ADD") {
//         if (parts.size() == 4) {
//             uint16_t val1 = p.variables.count(parts[2]) ? p.variables[parts[2]] : std::stoi(parts[2]);
//             uint16_t val2 = p.variables.count(parts[3]) ? p.variables[parts[3]] : std::stoi(parts[3]);
//             p.variables[parts[1]] = val1 + val2;
//         }
//     } else if (command == "PRINT") {
//         if (parts.size() > 1) {
//             std::string output = parts[1];
//             if (p.variables.count(output)) {
//                 outfile << p.variables[output] << std::endl;
//             } else {
//                 outfile << trim_quotes(output) << std::endl;
//             }
//         }
//     } else if (command == "SLEEP") {
//         if (parts.size() == 2) {
//             int sleep_cycles = std::stoi(parts[1]);
//             // This is a simplification. A real implementation would involve the scheduler.
//             // For now, we can simulate it with a delay, but it will block the core.
//             std::this_thread::sleep_for(std::chrono::milliseconds(sleep_cycles * 100)); // Placeholder
//             outfile << "Slept for " << sleep_cycles << " ticks." << std::endl;
//         }
//     }

//     outfile.close();
// }
  if (command == "DECLARE") {
        // DECLARE variable_name value
        if (parts.size() == 3) {
            std::string var_name = parts[1];
            uint16_t value = std::stoi(parts[2]);
            p.variables[var_name] = value;
            outfile << "DECLARE: " << var_name << " set to " << value << std::endl;
        } else {
            outfile << "Executing DECLARE command: " << command_str << std::endl;
        }
    } else if (command == "ADD") {
        // ADD result_variable operand1 operand2
        if (parts.size() == 4) {
            std::string result_var = parts[1];
            // Resolve operand1: check if it's a variable or a literal
            uint16_t val1 = p.variables.count(parts[2]) ? p.variables[parts[2]] : std::stoi(parts[2]);
            // Resolve operand2: check if it's a variable or a literal
            uint16_t val2 = p.variables.count(parts[3]) ? p.variables[parts[3]] : std::stoi(parts[3]);
            p.variables[result_var] = val1 + val2;
            outfile << "ADD: " << result_var << " = " << val1 << " + " << val2 << " (Result: " << p.variables[result_var] << ")" << std::endl;
        } else {
            outfile << "Executing ADD command: " << command_str << std::endl;
        }
    } else if (command == "PRINT") {
        // PRINT variable_name or "string literal"
        if (parts.size() > 1) {
            std::string output_target = parts[1];
            if (p.variables.count(output_target)) {
                // If it's a variable, print its value
                outfile << "PRINT: " << p.variables[output_target] << std::endl;
            } else {
                // If it's not a variable, assume it's a string literal and trim quotes
                outfile << "PRINT: " << trim_quotes(output_target) << std::endl;
            }
        } else {
            outfile << "Executing PRINT command: " << command_str << std::endl;
        }
    } else if (command == "SLEEP") {
        // SLEEP cycles
        if (parts.size() == 2) {
            int sleep_cycles = std::stoi(parts[1]);
            // Simulate sleep by pausing the current thread.
            // In a real OS, this would involve yielding to the scheduler.
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_cycles * 100)); // Placeholder delay
            outfile << "SLEEP: Process slept for " << sleep_cycles << " ticks." << std::endl;
        } else {
            outfile << "Executing SLEEP command: " << command_str << std::endl;
        }
    } else {
        // Log unknown commands
        outfile << "Executing Command: " << command << std::endl;
    }

    // Close the log file.
    outfile.close();
}