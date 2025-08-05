#include "CPU_Core.h"
#include "MemoryManager.h" 
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

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
    
    auto resolve = [&](const std::string& tok,
                       const std::unordered_map<std::string,uint16_t>& vars,
                       bool& ok) -> uint16_t
    {
        ok = true;
        if (auto it = vars.find(tok); it != vars.end())
            return it->second;
        try {
            return static_cast<uint16_t>(std::stoi(tok));
        } catch (const std::exception&) {
            ok = false;
            return 0;
        }
    };

    while (true)
    {
        std::string command_str = p.commands[p.commandCounter];
        std::vector<std::string> parts = split(command_str, ' ');
        std::string command = parts[0];

        bool page_fault_occurred = false;

        std::ofstream outfile(p.name + "_log.txt", std::ios_base::app);

        
        // if (command == "READ") {
        //     if (parts.size() != 3) {
        //         outfile << "ERROR: Invalid READ command format: " << command_str << std::endl;
        //     } else {
        //         std::string var_name = parts[1];
        //         size_t address = std::stoul(parts[2], nullptr, 16);
        //         int page_num = address / g_config.mem_per_frame;

        //         // page fault check
        //         if (page_num >= p.page_table.size() || !p.page_table[page_num].present) {
        //             g_memory_manager.handle_page_fault(p, page_num);
        //             page_fault_occurred = true;
        //         } else {
        //             // page is present, execute the read
        //             uint16_t value_read = g_memory_manager.read_memory(p, address);
        //             p.variables[var_name] = value_read;
        //             outfile << "READ: Stored value " << value_read << " from address 0x" 
        //                     << std::hex << address << std::dec << " into variable " << var_name << std::endl;
        //         }
        //     }
        // } 
        // else if (command == "WRITE") {
        //      if (parts.size() != 3) {
        //         outfile << "ERROR: Invalid WRITE command format: " << command_str << std::endl;
        //     } else {
        //         size_t address = std::stoul(parts[1], nullptr, 16);
        //         int page_num = address / g_config.mem_per_frame;

        //         // page fault check
        //         if (page_num >= p.page_table.size() || !p.page_table[page_num].present) {
        //             g_memory_manager.handle_page_fault(p, page_num);
        //             page_fault_occurred = true;
        //         } else {
        //             // page is present, resolve value and execute the write
        //             bool ok;
        //             uint16_t value_to_write = resolve(parts[2], p.variables, ok);
        //             if (!ok) {
        //                 outfile << "ERROR: Invalid value for WRITE: " << parts[2] << std::endl;
        //             } else {
        //                 g_memory_manager.write_memory(p, address, value_to_write);
        //                 p.page_table[page_num].dirty = true;
        //                 outfile << "WRITE: Wrote value " << value_to_write << " to address 0x" 
        //                         << std::hex << address << std::dec << std::endl;
        //             }
        //         }
        //     }
        // }
        if (command == "DECLARE") {
            int page_num = 0; // symbol table is in page 0

            // page fault check for symbol table page
            if (!p.page_table[page_num].present) {
                g_memory_manager.handle_page_fault(p, page_num);
                page_fault_occurred = true;
            } else {
                if (parts.size() == 3) {
                    std::string var_name = parts[1];
                    if (p.variables.size() >= 32) {
                        outfile << "ERROR: Symbol table full. Cannot declare '" << var_name << "'." << std::endl;
                    } else {
                        uint16_t value = std::stoi(parts[2]);
                        p.variables[var_name] = value;
                        p.page_table[page_num].dirty = true;
                        outfile << "DECLARE: " << var_name << " set to " << value << std::endl;
                    }
                } else {
                    outfile << "Executing DECLARE command: " << command_str << std::endl;
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
                } else {
                    outfile << "Executing " << command << " command (raw): " << command_str << '\n';
                }
            } else {
                outfile << "Executing " << command << " command (raw): " << command_str << '\n';
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

        // page fault retry logic

        if (page_fault_occurred) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5)); 
            continue; 
        } else {
            break; 
        }
    } 
}

void CPU_Core::run()
{
    while (!shutdown_signal)
    {
        std::shared_ptr<Process> p = nullptr;
        {
            std::lock_guard<std::mutex> lock(core_mtx);
            p = current_process;
        }

        if (p)
        {
            g_cpu_ticks_active++;

            p->has_page_fault = false;
            
            execute_command(*p);
            
            if (!p->has_page_fault) {
                p->commandCounter++;
                p->quantum_remaining--;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(g_config.delays_per_exec));
        }
        else
        {
            g_cpu_ticks_idle++;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}