#include "Main.h"

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
    std::vector<CPU_Core *> cpu_cores;
    std::vector<std::thread> core_threads;
    std::string input, screenName;
    Scheduler scheduler(g_ready_queue, g_running_list, cpu_cores, g_shutdown);
    std::thread scheduler_thread(&Scheduler::run, &scheduler);
    int process_id_counter = 1;

    do
    {
        std::cout << "\033[36m" << "Command> " << "\033[0m";
        std::getline(std::cin, input);

        // Commands available before initialization
        if (input == "help")
        {
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
                std::cout << "  scheduler-start - Start the scheduler." << std::endl;
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
                if (!g_config.loadFromFile("config.txt"))
                {
                    std::cerr << "Warning: Could not load config file, using default values" << std::endl;
                }
                g_config.loadFromFile("config.txt ");
                const int NUM_CORES = g_config.num_cpu;

                for (int i = 0; i < NUM_CORES; ++i)
                {
                    cpu_cores.push_back(new CPU_Core(i, g_finished_list, g_shutdown));
                }

                for (auto *core : cpu_cores)
                {
                    core_threads.emplace_back(&CPU_Core::run, core);
                }
                std::cout << "\033[32m[System] FCFS Scheduler and " << NUM_CORES << " CPU cores are now running in the background.\033[0m\n\n";
                            g_initialized = true;
                std::cout << "\033[32m[System] System initialized successfully.\033[0m" << std::endl;
            }
        }
        // Commands available only after initialization
        else if (!g_initialized)
        {
            std::cout << "\033[31mSystem not initialized. Please run 'initialize' first.\033[0m" << std::endl;
        }
        else if (input.rfind("screen", 0) == 0)
        {
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
                                << "Core: " << p.assigned_core_id << "\t"
                                << p.commandCounter << " / " << p.totalCommands << "\n";
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
        else if (input == "scheduler-start")
        {
            std::cout << "Scheduler start command executed." << std::endl;
            void startProcessGeneration();
        }
        else if (input == "scheduler-stop")
        {
            std::cout << "Scheduler stop command executed." << std::endl; 
            void endProcessGeneration();
        }
        else if (input == "report-util")
        {
            std::cout << "report-util command recognized. Generating report..." << std::endl;
            // Add your report utility logic here
        }
        else if (input != "exit")
        {
            std::cout << "\033[31m" << "Command not recognized. Type [help] for available commands." << "\033[0m" << std::endl;
        }
        
    } while (input != "exit");

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

    std::cout << "\033[32m[System] All threads terminated. Goodbye!\033[0m\n";
    return 0;
}
