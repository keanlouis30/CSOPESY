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

auto resolve = [&](const std::string& tok,
                   const std::unordered_map<std::string,uint16_t>& vars,
                   bool& ok) -> uint16_t
{
    // Assume success until something goes wrong
    ok = true;

    // 1. If it's a known variable, return its value
    if (auto it = vars.find(tok); it != vars.end())
        return it->second;

    // 2. Otherwise try it as a literal number
    try {
        return static_cast<uint16_t>(std::stoi(tok));
    }
    catch (const std::exception&) {
        ok = false;            // parsing failed
        return 0;              
    }
};


    //std::cout <<  " \033[35m" << command << "\033[0m" << std::endl;
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
    }else if (command == "ADD" || command == "SUBTRACT"){
            if (parts.size() == 3)
            {
                bool ok1, ok2;
                uint16_t val1 = resolve(parts[1], p.variables, ok1);
                uint16_t val2 = resolve(parts[2], p.variables, ok2);

                if (ok1 && ok2)            // both operands parsed successfully
                {
                    int result = (command == "ADD")
                            ? int(val1) + int(val2)
                            : int(val1) - int(val2);

                    outfile << command << ": " << result
                            << " = " << val1
                            << (command == "ADD" ? " + " : " - ")
                            << val2 << " (Result: " << result << ")\n";
                }
                else                       // at least one operand was invalid
                {
                    outfile << "Executing " << command
                            << " command (raw): " << command_str << '\n';
                }
            }
            else
            {
                outfile << "Executing " << command
                        << " command (raw): " << command_str << '\n';
            }
        }else if (command == "PRINT") {
        // PRINT variable_name or "string literal"
        if (parts.size() > 1) {
            std::string output_target = parts[1];
            if (p.variables.count(output_target)) {
                // If it's a variable, print its value
                outfile << "Hello World from  " << p.name << " !" << std::endl;
            } else {
                // If it's not a variable, assume it's a string literal and trim quotes
                outfile << "Hello World from  " << p.name << " !" << std::endl;
            }
        } else {
                outfile << "Hello World from  " << p.name << " !" << std::endl;
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
        outfile << "Executing Command: " << command_str << std::endl;
    }

    // Close the log file.
    outfile.close();
}