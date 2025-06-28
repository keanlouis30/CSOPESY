// HEADERS
#include "Main.h"

Config g_config;
ReadyQueue g_ready_queue;
ProcessCollection g_running_list;
ProcessCollection g_finished_list;
std::atomic<bool> g_shutdown(false);
std::atomic<bool> g_generate_processes(false);


// CURRENTLY: working on flow

void generate_report() {
    std::stringstream report;
    report << "---- CPU Utilization Report ----\n";

    // CPU Usage
    int busy_cores = 0;
    for (const auto& p : g_running_list.get_all()) {
        if (p.status == RUNNING) {
            busy_cores++;
        }
    }
    float utilization = (g_config.num_cpu > 0) ? (static_cast<float>(busy_cores) / g_config.num_cpu) * 100.0f : 0.0f;
    report << "CPU Utilization: " << std::fixed << std::setprecision(2) << utilization << "%\n";
    report << "Cores Used: " << busy_cores << " / " << g_config.num_cpu << "\n\n";

    // Running Processes
    report << "Running Processes (" << g_running_list.get_all().size() << "):\n";
    for (const auto& p : g_running_list.get_all()) {
        report << "  - PID: " << p.pid << ", Name: " << p.name 
               << ", Core: " << p.assigned_core_id 
               << ", Progress: " << p.commandCounter << "/" << p.totalCommands << "\n";
    }

    // Finished Processes
    report << "\nFinished Processes (" << g_finished_list.get_all().size() << "):\n";
    for (const auto& p : g_finished_list.get_all()) {
        report << "  - PID: " << p.pid << ", Name: " << p.name << ", Status: Finished\n";
    }

    report << "--------------------------------\n";

    // Print to console
    std::cout << report.str();

    // Save to file
    std::ofstream outfile("csopesy-log.txt");
    outfile << report.str();
    outfile.close();
    std::cout << "Report saved to csopesy-log.txt\n";
}

void process_generator_thread() {
    int process_counter = 1;
    while (!g_shutdown) {
        if (g_generate_processes) {
            std::string name = "p" + std::to_string(process_counter++);
            Process new_process(name, process_counter, g_config);
            g_ready_queue.push(new_process);
            std::cout << "[Generator] Created process " << name << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(g_config.batch_process_freq));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Check flag periodically
    }
}

void printBanner(){

    system("chcp 65001 > nul");

    std::cout << "\033[33m";
    std::cout << R"(

 ██████╗███████╗ ██████╗ ██████╗ ███████╗███████╗██╗   ██╗    ███████╗ ██╗██╗  ██╗     ██████╗  ██╗ ██████╗ 
██╔════╝██╔════╝██╔═══██╗██╔══██╗██╔════╝██╔════╝╚██╗ ██╔╝    ██╔════╝███║██║  ██║    ██╔════╝ ███║██╔═████╗
██║     ███████╗██║   ██║██████╔╝█████╗  ███████╗ ╚████╔╝     ███████╗╚██║███████║    ██║  ███╗╚██║██║██╔██║
██║     ╚════██║██║   ██║██╔═══╝ ██╔══╝  ╚════██║  ╚██╔╝      ╚════██║ ██║╚════██║    ██║   ██║ ██║████╔╝██║
╚██████╗███████║╚██████╔╝██║     ███████╗███████║   ██║       ███████║ ██║     ██║    ╚██████╔╝ ██║╚██████╔╝
 ╚═════╝╚══════╝ ╚═════╝ ╚═╝     ╚══════╝╚══════╝   ╚═╝       ╚══════╝ ╚═╝     ╚═╝     ╚═════╝  ╚═╝ ╚═════╝ 
                                                                                                            
    )" << std::endl;

    std::cout << "\033[1m" << "\033[32m" << "Hello, welcome to CSOPESY S14 Group 10's Command-Line Interface" << "\033[0m" << std::endl;
    std::cout << "Type" << " \033[35m" << "[help]" << "\033[0m" << " for the basic instructions of using the CLI\n"
              << std::endl;
    std::cout.flush();
}

int main()
{
    printBanner();
    std::unordered_map<std::string, std::function<void()>> screens; //changed this because Console is static
    std::vector<CPU_Core*> cpu_cores; 
    std::vector<std::thread> core_threads;
    Scheduler scheduler(g_ready_queue, g_running_list, cpu_cores, g_shutdown);
    std::thread scheduler_thread(&Scheduler::run, &scheduler);
    std::thread generator_thread(process_generator_thread);
    std::string input, screenName;
    bool initialized = false;
    int process_id_counter = 1;

    do {
        std::cout << "\033[36m" << "Command> " << "\033[0m";
        std::getline(std::cin, input);

    if (input == "initialize") {
        if(!initialized){
            if (!g_config.loadFromFile("config.txt")) {
                std::cerr << "Error loading config. Exiting.\n";
                exit(1);
            }

            for (int i = 0; i < g_config.num_cpu; ++i) {
                CPU_Core* core = new CPU_Core(i, g_running_list, g_shutdown);
                cpu_cores.push_back(core);
            }

            for (auto* core : cpu_cores) {
                core_threads.emplace_back(&CPU_Core::run, core);
            }

            // If you want, join here or store threads and join later
            
            std::cout << "\033[32mSystem initialized successfully.\033[0m\n";
            initialized = true;
        }
        
    } else if (input == "help") {
        std::cout << "\033[35m" << "Available commands:" << "\033[0m" << std::endl;
        std::cout << "  initialize    - Initialize the scheduler and start the system." << std::endl;
        std::cout << "  help          - Show available commands." << std::endl;
        std::cout << "  clear         - Clear the screen" << std::endl;
        std::cout << "  exit          - Exit the application." << std::endl;
    } else {
        std::cout << "\033[31mUnknown Command. Please run 'initialize' first.\033[0m" << std::endl;
    }

    } while (!initialized);

    //now that it has been initialized
    bool exit = false;
    do {
        std::cout << "\033[36m" << "Command> " << "\033[0m";
        std::getline(std::cin, input);

        if(input == "help") {
            std::cout << "  screen -ls    - Lists all running and finished processes." << std::endl;
            std::cout << "  screen -s <name> - Create a new screen/process." << std::endl;
            std::cout << "  screen -r <name> - Resume/view an existing screen." << std::endl;
            std::cout << "  scheduler-start - Start the scheduler." << std::endl;
            std::cout << "  scheduler-stop - Stop the scheduler." << std::endl;
            std::cout << "  report-util   - Generate utilization report." << std::endl;
            std::cout << "  clear         - Clear the screen" << std::endl;
            std::cout << "  exit          - Stops all threads and exits the application." << std::endl;
        }
        else if (input == "clear") {
            system("cls");
            std::cout.flush();
            printBanner();
        }
        else if (input == "initialize") {
            std::cout << "System already initialized." << std::endl;
        }
        else if (input.rfind("screen", 0) == 0)
        {
            if (input == "screen -ls")
            {
                system("cls");
                // system("clear");
                std::stringstream report;
                // CPU Usage
                int busy_cores = 0;
                for (const auto& p : g_running_list.get_all()) {
                    if (p.status == RUNNING) {
                        busy_cores++;
                    }
                }
                float utilization = (g_config.num_cpu > 0) ? (static_cast<float>(busy_cores) / g_config.num_cpu) * 100.0f : 0.0f;
                report << "CPU Utilization: " << std::fixed << std::setprecision(2) << utilization << "%\n";
                report << "Cores Used: " << busy_cores << " / " << g_config.num_cpu << "\n\n";

                std::cout << report.str();
                std::cout << "\n";
                std::cout << "--------------------------------------------------------\n";
                
                std::cout << "Running processes:\n";
                auto running = g_running_list.get_all();
                if (running.empty())
                {
                    std::cout << "  (None)\n";
                }
                else
                {
                    for (const auto &p : running)
                    {
                        std::cout << "  " << p.name << "\t(" << p.creation_timestamp << ")\t"
                                << "Core: " << p.assigned_core_id << "\t"
                                << p.commandCounter << " / " << p.totalCommands;

                        std::cout << "\n";
                    }
                }

                std::cout << "\n";
                std::cout << "\n";
                std::cout << "Finished processes:\n";
                auto finished = g_finished_list.get_all();
                if (finished.empty())
                {
                    std::cout << "  (None)\n";
                }
                else
                {
                    for (const auto &p : finished)
                    {
                        std::cout << "  " << p.name << "\t(" << p.creation_timestamp << ")\t"
                                << "Finished\t"
                                << p.commandCounter << " / " << p.totalCommands << "\n";
                    }
                }
                std::cout << "--------------------------------------------------------\n\n";
            }
            else if (input.rfind("screen -s ", 0) == 0)
            {
                screenName = input.substr(10);
                // Check if a process with this name already exists in any list
                if (g_ready_queue.exists(screenName) || g_running_list.exists(screenName) || g_finished_list.exists(screenName)) {
                    std::cout << "Process or screen \"" << screenName << "\" already exists.\n";
                } else {
                    Process new_process(screenName, process_id_counter++, g_config);
                    g_ready_queue.push(new_process);

                    screens.emplace(screenName, [=]() {
                        Console::display(screenName, g_ready_queue, g_running_list, g_finished_list);
                    });

                    screens[screenName]();
                }

            }
            else if (input.rfind("screen -r ", 0) == 0)
            {
                screenName = input.substr(10);
                Console::display(screenName, g_ready_queue, g_running_list, g_finished_list);
            }
            else
            {
                std::cout << "\033[31m" << "Enter 'screen -ls' to list processes, 'screen -s <name>' to create, or 'screen -r <name>' to resume." << "\033[0m" << std::endl;
            }
        }
        else if (input == "scheduler-start")
        {
            g_generate_processes = true;
            std::cout << "Automatic process generation started." << std::endl;
        }
        else if (input == "scheduler-stop")
        {
            g_generate_processes = false;
            std::cout << "Automatic process generation stopped." << std::endl;
        }
        else if (input == "report-util")
        {
            generate_report();
        }
        else if (input == "exit"){
            exit = true;
        }
        else
        {
            std::cout << "\033[31m" << "Command not recognized. Type [help] for available commands." << "\033[0m" << std::endl;
        }

    } while (!exit);

    std::cout << "\n\033[33m[System] Shutdown initiated. Waiting for all tasks to complete...\033[0m\n";
    g_shutdown = true;

    g_generate_processes = false;
    generator_thread.join();
    std::cout << "[System] Process generator thread has shut down.\n";

    scheduler_thread.join();
    std::cout << "[System] Scheduler thread has shut down.\n";

    for (size_t i = 0; i < core_threads.size(); ++i)
    {
        core_threads[i].join();
        std::cout << "[System] Core " << i << " thread has shut down.\n";
    }

    for (auto *core : cpu_cores)
    {
        delete core;
    }

    std::cout << "\033[32m[System] All threads terminated. Goodbye!\033[0m\n";
    return 0;

};
