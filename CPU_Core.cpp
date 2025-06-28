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

    if (command == "DECLARE") {
        if (parts.size() == 3) {
            p.variables[parts[1]] = std::stoi(parts[2]);
        }
    } else if (command == "ADD") {
        if (parts.size() == 4) {
            uint16_t val1 = p.variables.count(parts[2]) ? p.variables[parts[2]] : std::stoi(parts[2]);
            uint16_t val2 = p.variables.count(parts[3]) ? p.variables[parts[3]] : std::stoi(parts[3]);
            p.variables[parts[1]] = val1 + val2;
        }
    } else if (command == "PRINT") {
        if (parts.size() > 1) {
            std::string output = parts[1];
            if (p.variables.count(output)) {
                outfile << p.variables[output] << std::endl;
            } else {
                outfile << trim_quotes(output) << std::endl;
            }
        }
    } else if (command == "SLEEP") {
        if (parts.size() == 2) {
            int sleep_cycles = std::stoi(parts[1]);
            // This is a simplification. A real implementation would involve the scheduler.
            // For now, we can simulate it with a delay, but it will block the core.
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_cycles * 100)); // Placeholder
            outfile << "Slept for " << sleep_cycles << " ticks." << std::endl;
        }
    }

    outfile.close();
}
