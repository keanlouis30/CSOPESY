// HEADERS
#include "Main.h"
#include "MemoryManager.h"
#include <unordered_map>
#include <string>
#include <functional>

Config g_config;
ReadyQueue g_ready_queue;
ProcessCollection g_running_list;
ProcessCollection g_finished_list;
MemoryManager g_memory_manager;
std::atomic<bool> g_shutdown(false);
std::atomic<bool> g_generate_processes(false);
std::atomic<int> g_quantum_tick_counter(0);

void generate_memory_report(int tick)
{
    // Create filename
    std::ofstream report_file("memory_stamp_" + std::to_string(tick) + ".txt");
    if (!report_file.is_open())
    {
        // Optional: Log an error if the file can't be created
        std::cerr << "Error: Could not open memory_stamp_" << tick << ".txt for writing." << std::endl;
        return;
    }

    // Get timestamp
    time_t now = time(nullptr);
    char time_buf[100];
    strftime(time_buf, sizeof(time_buf), "%m/%d/%Y %I:%M:%S%p", localtime(&now));

    // Get data
    int process_count = g_memory_manager.get_process_count_in_memory();
    size_t fragmentation_bytes = g_memory_manager.calculate_external_fragmentation();
    // The spec asks for KB, so divide by 1024

    // Write to file, matching the spec format
    report_file << "Timestamp: (" << time_buf << ")\n";
    report_file << "Number of processes in memory: " << process_count << "\n";
    report_file << "Total external fragmentation in B: " << fragmentation_bytes << "\n";
    report_file << g_memory_manager.generate_memory_snapshot(g_running_list.get_all());

    report_file.close();
}

// CURRENTLY: working on flow

void generate_report()
{
    std::stringstream report;
    report << "---- CPU Utilization Report ----\n";

    // CPU Usage
    int busy_cores = 0;
    for (const auto &p : g_running_list.get_all())
    {
        if (p.status == RUNNING)
        {
            busy_cores++;
        }
    }

    float utilization = (g_config.num_cpu > 0) ? (static_cast<float>(busy_cores) / g_config.num_cpu) * 100.0f : 0.0f;
    report << "CPU Utilization: " << std::fixed << std::setprecision(2) << utilization << "%\n";
    report << "Cores Used: " << busy_cores << " / " << g_config.num_cpu << "\n\n";

    // Running Processes
    report << "Running Processes (" << g_running_list.get_all().size() << "):\n";
    for (const auto &p : g_running_list.get_all())
    {
        report << "  - PID: " << p.pid << ", Name: " << p.name
               << ", Core: " << p.assigned_core_id
               << ", Progress: " << p.commandCounter << "/" << p.totalCommands << "\n";
    }

    // Finished Processes
    report << "\nFinished Processes (" << g_finished_list.get_all().size() << "):\n";
    for (const auto &p : g_finished_list.get_all())
    {
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

void memory_reporter_thread()
{
    int last_tick = 0;
    while (!g_shutdown)
    {
        int current_tick = g_quantum_tick_counter.load();
        if (current_tick > last_tick)
        {
            for (int i = last_tick + 1; i <= current_tick; ++i)
            {
                generate_memory_report(i);
            }
            last_tick = current_tick;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Check periodically
    }
}

void process_generator_thread()
{
    int process_counter = 1; // This will now be the PID
    while (!g_shutdown)
    {
        if (g_generate_processes)
        {
            // Use the current counter value for both name and PID
            std::string name = "p" + std::to_string(process_counter);
            Process new_process(name, process_counter, g_config); // PID is now process_counter

            g_ready_queue.push(new_process);
            std::cout << "[Generator] Created process " << name << " with PID " << process_counter << std::endl;

            // Increment the counter for the *next* process
            process_counter++;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(g_config.batch_process_freq));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Check flag periodically
    }
}

void printBanner()
{

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

    if (!g_config.loadFromFile("config.txt"))
    {
        std::cerr << "Error loading config. Exiting.\n";
        exit(1);
    }

    g_memory_manager.initialize(g_config.max_overall_mem);

    std::unordered_map<std::string, std::function<void()>> screens; // changed this because Console is static
    std::vector<CPU_Core *> cpu_cores;
    std::vector<std::thread> core_threads;
    Scheduler scheduler(g_ready_queue, g_running_list, cpu_cores, g_shutdown);
    std::thread scheduler_thread(&Scheduler::run, &scheduler);
    std::thread generator_thread(process_generator_thread);
    std::thread reporter_thread(memory_reporter_thread);
    std::string input, screenName;
    bool initialized = false;
    int exit = 0;
    int process_id_counter = 1;

    std::cout << "\033[32mSystem initialized successfully.\033[0m\n";

    do
    {
        std::cout << "\033[36m" << "Command> " << "\033[0m";
        std::getline(std::cin, input);

        if (input == "initialize")
        {
            if (!initialized)
            {

                for (int i = 0; i < g_config.num_cpu; ++i)
                {
                    CPU_Core *core = new CPU_Core(i, g_running_list, g_shutdown);
                    cpu_cores.push_back(core);
                }

                for (auto *core : cpu_cores)
                {
                    core_threads.emplace_back(&CPU_Core::run, core);
                }

                initialized = true;
            }
        }
        else if (input == "help")
        {
            std::cout << "\033[35m" << "Available commands:" << "\033[0m" << std::endl;
            std::cout << "  initialize    - Initialize the scheduler and start the system." << std::endl;
            std::cout << "  help          - Show available commands." << std::endl;
            std::cout << "  clear         - Clear the screen" << std::endl;
            std::cout << "  exit          - Exit the application." << std::endl;
        }
        else if (input == "exit")
        {
            exit = 9;
        }
        else
        {
            std::cout << "\033[31mUnknown Command. Please run 'initialize' first.\033[0m" << std::endl;
        }

    } while (!initialized && exit != 9);

    // now that it has been initialized
    if (exit != 9)
    {
        do
        {
            std::cout << "\033[36m" << "Command> " << "\033[0m";
            std::getline(std::cin, input);

            if (input == "help")
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
            else if (input == "clear")
            {
                system("cls");
                std::cout.flush();
                printBanner();
            }
            else if (input == "initialize")
            {
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
                    for (const auto &p : g_running_list.get_all())
                    {
                        if (p.status == RUNNING)
                        {
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
                    if (g_ready_queue.exists(screenName) || g_running_list.exists(screenName) || g_finished_list.exists(screenName))
                    {
                        std::cout << "Process or screen \"" << screenName << "\" already exists.\n";
                    }
                    else
                    {
                        Process new_process(screenName, process_id_counter++, g_config);
                        g_ready_queue.push(new_process);

                        screens.emplace(screenName, [=]()
                                        { Console::display(screenName, g_ready_queue, g_running_list, g_finished_list); });

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
            else if (input == "exit")
            {
                exit = 1;
            }
            else
            {
                std::cout << "\033[31m" << "Command not recognized. Type [help] for available commands." << "\033[0m" << std::endl;
            }
    } while (exit != 1);
    }

    if (exit != 9)
    {
        std::cout << "\n\033[33m[System] Shutdown initiated. Waiting for all tasks to complete...\033[0m\n";
        g_shutdown = true;

        g_generate_processes = false;
        generator_thread.join();
        std::cout << "[System] Process generator thread has shut down.\n";

        scheduler_thread.join();
        std::cout << "[System] Scheduler thread has shut down.\n";

        reporter_thread.join();
        std::cout << "[System] Memory reporter thread has shut down.\n";

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
    }
    if (exit == 9)
    {
        std::cout << "\n\033[33m[System] Shutdown initiated.\033[0m\n";
    }
    return 0;
};
