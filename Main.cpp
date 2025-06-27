#include "Main.h"

int main()
{
    printBanner();

    // 1. Load configuration FIRST.
    if (!g_config.loadFromFile("config.txt"))
    {
        std::cerr << "Warning: Could not load config file, using default values" << std::endl;
    }

    // 2. Now create the cores using the loaded configuration.
    std::vector<CPU_Core *> cpu_cores;
    std::vector<std::thread> core_threads;
    for (int i = 0; i < g_config.num_cpu; ++i)
    {
        cpu_cores.push_back(new CPU_Core(i, g_finished_list, g_shutdown));
    }

    // 3. Start the appropriate scheduler.
    std::thread scheduler_thread;
    if (g_config.scheduler == "rr")
    {
        RoundRobinScheduler *rr_scheduler = new RoundRobinScheduler(g_ready_queue, g_running_list, g_finished_list, cpu_cores, g_config, g_shutdown);
        scheduler_thread = std::thread(&RoundRobinScheduler::run, rr_scheduler);
        std::cout << "\033[32m[System] Round Robin Scheduler and " << g_config.num_cpu << " CPU cores are now running.\033[0m\n\n";
    }
    else // Default to FCFS
    {
        FCFSScheduler *fcfs_scheduler = new FCFSScheduler(g_ready_queue, g_running_list, g_finished_list, cpu_cores, g_shutdown);
        scheduler_thread = std::thread(&FCFSScheduler::run, fcfs_scheduler);
        std::cout << "\033[32m[System] FCFS Scheduler and " << g_config.num_cpu << " CPU cores are now running.\033[0m\n\n";
    }

    for (auto *core : cpu_cores)
    {
        core_threads.emplace_back(&CPU_Core::run, core);
    }


    std::string input, screenName;
    int process_id_counter = 1;

    do
    {
        std::cout << "\033[36m" << "Command> " << "\033[0m";
        std::getline(std::cin, input);

        if (input == "help")
        {
            // (Help logic is fine, no changes needed here)
            std::cout << "\033[35m" << "Available commands:" << "\033[0m" << std::endl;
            if (!g_initialized)
            {
                std::cout << "  initialize    - Initialize the scheduler and start the system." << std::endl;
                std::cout << "  help          - Show available commands." << std::endl;
                std::cout << "  clear         - Clear the screen" << std::endl;
                std::cout << "  exit          - Exit the application." << std::endl;
            }
            else
            {
                std::cout << "  screen -ls    - Lists all running and finished processes." << std::endl;
                std::cout << "  screen -s <name> - Create a new screen/process." << std::endl;
                std::cout << "  screen -r <name> - Resume/view an existing screen." << std::endl;
                std::cout << "  scheduler-test - Test the scheduler." << std::endl;
                std::cout << "  scheduler-stop - Stop the scheduler." << std::endl;
                std::cout << "  report-util   - Generate utilization report." << std::endl;
                std::cout << "  clear         - Clear the screen" << std::endl;
                std::cout << "  exit          - Stops all threads and exits the application." << std::endl;
            }
        }
        else if (input == "clear")
        {
            system("cls");
            std::cout.flush();
            printBanner();
        }
        else if (input == "initialize")
        {
            if (g_initialized)
            {
                std::cout << "System already initialized." << std::endl;
            }
            else
            {
                // 4. REMOVE the redundant config load from here.
                g_initialized = true;
                std::cout << "\033[32m[System] System initialized successfully.\033[0m" << std::endl;

                for (int i = 1; i <= 5; i++)
                {
                    std::vector<std::string> commands;
                    int num_commands = g_config.min_ins + (rand() % (g_config.max_ins - g_config.min_ins + 1));

                    for (int j = 0; j < num_commands; j++)
                    {
                        commands.push_back("Hello world from process_" + std::to_string(i));
                    }

                    Process new_process("process_" + std::to_string(i), process_id_counter++, commands);
                    g_ready_queue.push(new_process);
                }
                std::cout << "\033[32m[System] Created 5 test processes.\033[0m" << std::endl;
            }
        }
        else if (!g_initialized)
        {
            std::cout << "\033[31mSystem not initialized. Please run 'initialize' first.\033[0m" << std::endl;
        }
        else if (input.rfind("screen", 0) == 0)
        {
            // (screen logic is fine, no changes needed here)
             if (input == "screen -ls")
            {
                system("cls");

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
                                  << "Core: " << p.assigned_core_id << "\t";
                        if (g_config.scheduler == "rr") {
                             std::cout << "Quantum: " << p.quantum_remaining << "/" << p.quantum_max << "\t";
                        }
                        std::cout << p.commandCounter << " / " << p.totalCommands << "\n";
                    }
                }
                std::cout << "--------------------------------------------------------\n";

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
                if (screens.find(screenName) != screens.end())
                {
                    std::cout << "Screen \"" << screenName << "\" already exists.\n";
                }
                else
                {
                    screens.emplace(screenName, Console(screenName, 20));
                    screens[screenName].display();
                }
            }
            else if (input.rfind("screen -r ", 0) == 0)
            {
                screenName = input.substr(10);
                if (screens.find(screenName) != screens.end())
                {
                    screens[screenName].display();
                }
                else
                {
                    std::cout << "\033[31mNo screen found with name \"" << screenName << "\". \033[0m\n";
                }
            }
            else
            {
                std::cout << "\033[31m" << "Enter 'screen -ls' to list processes, 'screen -s <name>' to create, or 'screen -r <name>' to resume." << "\033[0m" << std::endl;
            }
        }
        else if (input == "scheduler-test")
        {
            std::cout << "Scheduler test command executed." << std::endl;
        }
        else if (input == "scheduler-stop")
        {
            std::cout << "Scheduler stop command executed." << std::endl;
        }
        else if (input == "report-util")
        {
            std::cout << "report-util command recognized. Generating report..." << std::endl;
        }
        else if (input != "exit")
        {
            std::cout << "\033[31m" << "Command not recognized. Type [help] for available commands." << "\033[0m" << std::endl;
        }
        
    } while (input != "exit");


    // (Shutdown logic is fine, no changes needed here)
    std::cout << "\n\033[33m[System] Shutdown initiated. Waiting for all tasks to complete...\033[0m\n";
    g_shutdown = true;

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

    std::cout << "\032[32m[System] All threads terminated. Goodbye!\033[0m\n";
    return 0;
}