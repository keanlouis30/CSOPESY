#include "Console.h"
#include "ScreenInterface.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

void Console::display(const std::string& process_name, ReadyQueue& ready_queue, ProcessCollection& running_list, ProcessCollection& finished_list) {
    ScreenInterface SI;
    std::string input;

    while (true) {
        system("cls");
        SI.printBorder("top");

        Process p;
        bool found = running_list.find(process_name, p) ||
                     ready_queue.find(process_name, p) ||
                     finished_list.find(process_name, p);

        if (!found) {
            SI.printMessage("\033[31mProcess '" + process_name + "' not found.\033[0m");
            SI.printBorder("bottom");
            std::this_thread::sleep_for(std::chrono::seconds(2));
            break;
        }

        // Display process info (process-smi)
        SI.printMessage("\033[33mScreen: " + p.name + "\033[0m");
        SI.printMessage("\033[32mPID: " + std::to_string(p.pid) + "\033[0m");
        SI.printMessage("\033[32mStatus: " + statusToString(p.status) + "\033[0m");
        SI.printMessage("\033[32mInstructions: " + std::to_string(p.commandCounter) + " / " + std::to_string(p.totalCommands) + "\033[0m");
        if (p.status == RUNNING) {
            SI.printMessage("\033[32mCore: " + std::to_string(p.assigned_core_id) + "\033[0m");
        }
        SI.printMessage(""); // Spacer

        // Display logs
        SI.printMessage("\033[35mLogs:\033[0m");
        std::ifstream log_file(p.name + "_log.txt");
        if (log_file.is_open()) {
            std::string line;
            while (std::getline(log_file, line)) {
                SI.printMessage("  " + line);
            }
            log_file.close();
        } else {
            SI.printMessage("  (No logs yet)");
        }

        if (p.status == FINISHED) {
            SI.printMessage("\n\033[32mFinished!\033[0m");
        }

        SI.printBorder("bottom");

        std::cout << "\nType 'process-smi' to refresh, or 'exit' to return to main menu.\n";
        std::cout << "\033[33mScreen> \033[0m";
        
        std::getline(std::cin, input);

        if (input == "exit") {
            break;
        }
    }
}
