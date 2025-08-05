#include "CPU_Core.h"
#include "MemoryManager.h" 
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

extern MemoryManager g_memory_manager;

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string trim_quotes(const std::string& str) {
    if (str.length() >= 2 && str.front() == '"' && str.back() == '"') {
        return str.substr(1, str.length() - 2);
    }
    return str;
}

void CPU_Core::execute_command(Process &p)
{
    if (p.commandCounter >= p.commands.size()) return;

    p.has_page_fault = false;
    std::string command_str = p.commands[p.commandCounter];
    std::vector<std::string> parts = split(command_str, ' ');
    std::string command = parts[0];

    std::ofstream outfile(p.name + "_log.txt", std::ios_base::app);

    auto resolve = [&](const std::string& tok,
                       const std::unordered_map<std::string, uint16_t>& vars,
                       bool& ok) -> uint16_t
    {
        // Assume success until something goes wrong
        ok = true;

        // 1. If it's a known variable, return its value
        auto it = vars.find(tok);
        if (it != vars.end())
            return it->second;

        // 2. Otherwise try it as a literal number
        try {
            return static_cast<uint16_t>(std::stoi(tok));
        }
        catch (const std::exception&) {
            ok = false;  // parsing failed
            return 0;
        }
    };

    if (command == "DECLARE") {
        // DECLARE variable_name value
        if (parts.size() == 3) {
            std::string var_name = parts[1];

            if (p.variables.size() >= 32) {
                outfile << "DECLARE failed: Symbol table full (max 32 variables)." << std::endl;
            }
            else if (p.variables.find(var_name) != p.variables.end()) {
                outfile << "DECLARE ignored: Variable '" << var_name << "' already declared." << std::endl;
            }
            else {
                p.variables[var_name] = p.next_offset;
                outfile << "DECLARE: " << var_name 
                        << " assigned offset " << p.next_offset << std::endl;
                p.next_offset++;
            }
        }
    } 
    else if (command == "ADD" || command == "SUBTRACT") {
        if (parts.size() == 3) {
            bool ok1, ok2;
            uint16_t val1 = resolve(parts[1], p.variables, ok1);
            uint16_t val2 = resolve(parts[2], p.variables, ok2);

            if (ok1 && ok2) {
                int result = (command == "ADD")
                    ? int(val1) + int(val2)
                    : int(val1) - int(val2);

                outfile << command << ": " << result
                        << " = " << val1
                        << (command == "ADD" ? " + " : " - ")
                        << val2 << " (Result: " << result << ")\n";
            }
            else {
                outfile << "Executing " << command
                        << " command (raw): " << command_str << '\n';
            }
        }
        else {
            outfile << "Executing " << command
                    << " command (raw): " << command_str << '\n';
        }
    } 
    else if (command == "PRINT") {
        if (parts.size() > 1) {
            std::string output_target = parts[1];
            if (p.variables.count(output_target)) {
                outfile << "Hello World from  " << p.name << " !" << std::endl;
            } else {
                outfile << "Hello World from  " << p.name << " !" << std::endl;
            }
        } 
        else if (command == "SLEEP") {
            if (parts.size() == 2) {
                int sleep_cycles = std::stoi(parts[1]);
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_cycles * 100));
                outfile << "SLEEP: Process slept for " << sleep_cycles << " ticks." << std::endl;
            } else {
                outfile << "Executing SLEEP command: " << command_str << std::endl;
            }
        } 
        else if (command == "READ") {
            if (parts.size() == 2) {
                outfile << "READ: " << parts[1] << std::endl;
            } else {
                outfile << "Executing READ command: " << command_str << std::endl;
            }
        } 
        else if (command == "WRITE") {
            if (parts.size() == 2) {
                outfile << "WRITE: " << parts[1]  << std::endl;
            } else {
                outfile << "Executing WRITE command: " << command_str << std::endl;
            }
        } 
        else  {
            outfile << "Executing Command: " << command_str << std::endl;
        }

        outfile.close();
    } 
    else if (command == "SLEEP") {
        if (parts.size() == 2) {
            int sleep_cycles = std::stoi(parts[1]);
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_cycles * 100));
            outfile << "SLEEP: Process slept for " << sleep_cycles << " ticks." << std::endl;
        } else {
            outfile << "Executing SLEEP command: " << command_str << std::endl;
        }
    } 
    else {
        outfile << "Executing Command: " << command_str << std::endl;
    }

    outfile.close();
}

void CPU_Core::run()
{
    using namespace std::chrono_literals;
    
    while (!shutdown_signal.load(std::memory_order_acquire))
    {
        std::lock_guard<std::mutex> lock(core_mtx);
        
        if (current_process && current_process->status == RUNNING)
        {
            // Check for page fault
            if (current_process->has_page_fault)
            {
                // Handle page fault
                g_memory_manager.handle_page_fault(*current_process, current_process->faulting_page_num);
                current_process->has_page_fault = false;
            }
            else
            {
                // Execute next command
                execute_command(*current_process);
                current_process->commandCounter++;
                
                // Check if process is finished
                if (current_process->commandCounter >= current_process->totalCommands)
                {
                    current_process->status = FINISHED;
                    current_process.reset();
                }
            }
        }
        
        std::this_thread::sleep_for(1ms);
    }
}