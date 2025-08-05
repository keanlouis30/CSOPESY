// HEADERS
#include "Main.h"
#include "MemoryManager.h"
#include <unordered_map>
#include <string>
#include <functional>
#include <random>

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

size_t rand_memory()
{
    static const std::array<size_t, 11> valid_sizes = {
        64, 128, 256, 512,
        1024, 2048, 4096, 8192,
        16384, 32768, 65536
    };

    // Step 2: Randomly select a memory size
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, valid_sizes.size() - 1);
    size_t memory_size = valid_sizes[dist(gen)];

    return memory_size;
}

bool is_valid_memory_size(size_t mem_size) {

    // range [2^6, 2^16] = [64, 65536]
    if (mem_size < 64 || mem_size > 65536) {
        return false;
    }

    // power of 2
    if ((mem_size & (mem_size - 1)) != 0) {
        return false;
    }

    return true;
}

// Helper to trim whitespace
std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

// Helper to trim surrounding double quotes
std::string trim_inst(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

// Split by ';'
std::vector<std::string> split(const std::string& input) {
    std::vector<std::string> commands;
    std::stringstream ss(input);
    std::string item;

    while (std::getline(ss, item, ';')) {
        commands.push_back(trim(item));
    }

    return commands;
}

void process_generator_thread()
{
    int process_counter = 1; // This will now be the PID
    while (!g_shutdown)
    {
        if (g_generate_processes)
        {
            // Use the current counter value for both name and PID
            size_t mem = rand_memory();
            std::string name = "p" + std::to_string(process_counter);
            Process new_process(name, mem, process_counter, g_config, true); // PID is now process_counter

            g_ready_queue.push(new_process);
            std::cout << "[Generator] Created process " << name << " with PID " << process_counter << "and Memory Size" << mem << std::endl;

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
                std::cout << "  screen -s <name> <memory size>- Create a new screen/process." << std::endl;
                std::cout << "  screen -r <name> - Resume/view an existing screen." << std::endl;
                std::cout << "  screen -c <process_name> <process_memory_size> <instructions> - Resume/view an existing screen." << std::endl;
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
                    system("cls");  // Use "clear" on Linux/macOS if needed
                    std::stringstream report;

                    int busy_cores = 0;
                    for (const auto& p : g_running_list.get_all())
                    {
                        if (p.status == RUNNING) busy_cores++;
                    }

                    float utilization = (g_config.num_cpu > 0)
                        ? (static_cast<float>(busy_cores) / g_config.num_cpu) * 100.0f
                        : 0.0f;

                    report << "CPU Utilization: " << std::fixed << std::setprecision(2)
                        << utilization << "%\n";
                    report << "Cores Used: " << busy_cores << " / " << g_config.num_cpu << "\n\n";

                    std::cout << report.str();
                    std::cout << "--------------------------------------------------------\n";

                    // Running processes
                    std::cout << "Running processes:\n";
                    auto running = g_running_list.get_all();
                    if (running.empty())
                        std::cout << "  (None)\n";
                    else
                    {
                        for (const auto& p : running)
                        {
                            std::cout << "  " << p.name << "\t(" << p.creation_timestamp << ")\t"
                                    << "Core: " << p.assigned_core_id << "\t"
                                    << p.commandCounter << " / " << p.totalCommands << "\n";
                        }
                    }

                    std::cout << "\nFinished processes:\n";
                    auto finished = g_finished_list.get_all();
                    if (finished.empty())
                        std::cout << "  (None)\n";
                    else
                    {
                        for (const auto& p : finished)
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
                    std::string remainder = input.substr(std::string("screen -s ").length());
                    std::istringstream iss(remainder);
                    std::string screenName, mem_size_str;

                    iss >> screenName >> mem_size_str;

                    try {
                        size_t memory_size = std::stoul(mem_size_str);

                        if (g_ready_queue.exists(screenName) || g_running_list.exists(screenName) || g_finished_list.exists(screenName))
                        {
                            std::cout << "Process or screen \"" << screenName << "\" already exists.\n";
                        }
                        else if (!is_valid_memory_size(memory_size))
                        {
                            std::cout << "Invalid memory size. Must be a power of two between 64 and 65536.\n";
                        }
                        else
                        {
                            std::cout << "Name: " << screenName << "\nMemory: " << memory_size << " bytes\n";
                            Process new_process(screenName, memory_size, process_id_counter++, g_config, true);
                            g_ready_queue.push(new_process);

                            screens.emplace(screenName, [=]() {
                                Console::display(screenName, g_ready_queue, g_running_list, g_finished_list);
                            });

                            screens[screenName]();
                        }
                    } catch (const std::invalid_argument& e) {
                        std::cout << "Invalid memory size: not a number.\n";
                    } catch (const std::out_of_range& e) {
                        std::cout << "Invalid memory size: value too large.\n";
                    }
                }
                else if (input.rfind("screen -r ", 0) == 0)
                {
                    std::string screenName = input.substr(std::string("screen -r ").length());
                    Console::display(screenName, g_ready_queue, g_running_list, g_finished_list);
                }
                else if (input.rfind("screen -c ", 0) == 0)
                {
                    std::istringstream iss(input.substr(10));
                    std::string processName;
                    std::string memoryStr;
                    std::string instructionsRaw;

                    iss >> processName >> memoryStr;

                    // Get everything after memory size as instruction string
                    std::getline(iss, instructionsRaw);
                    instructionsRaw = trim_inst(trim(instructionsRaw));

                    try {
                        size_t memorySize = std::stoul(memoryStr);

                        if (!is_valid_memory_size(memorySize)) {
                            std::cout << "Invalid memory size. Must be a power of two between 64 and 65536.\n";
                        }

                        if (g_ready_queue.exists(processName) || g_running_list.exists(processName) || g_finished_list.exists(processName)) {
                            std::cout << "Process \"" << processName << "\" already exists.\n";
                        }

                        std::vector<std::string> instructions = split(instructionsRaw);

                        for (auto& instr : instructions) {
                            instr = trim(instr);
                        }

                        // Check instruction count
                        if (instructions.size() < 1 || instructions.size() > 50) {
                            std::cout << "Invalid instruction count. Must be between 1 and 50.\n";
                        }

                        Process newProcess(processName, memorySize, process_id_counter++, g_config, false);
                        newProcess.commands = instructions;
                        g_ready_queue.push(newProcess);

                        std::cout << "Created process \"" << processName << "\" with " << instructions.size() << " instructions.\n";

                        screens.emplace(processName, [=]() {
                            Console::display(processName, g_ready_queue, g_running_list, g_finished_list);
                        });

                        screens[processName]();
                    }
                    catch (const std::invalid_argument&) {
                        std::cout << "Invalid memory size: not a number.\n";
                    }
                    catch (const std::out_of_range&) {
                        std::cout << "Invalid memory size: value too large.\n";
                    }
                }
                else
                {
                    std::cout << "\033[31m"
                            << "Usage:\n"
                            << "  screen -ls                     : List processes\n"
                            << "  screen -s <name> <mem_size>    : Create process\n"
                            << "  screen -r <name>               : Resume process\n"
                            << "  screen -c <name>               : Clear screen\n"
                            << "\033[0m\n";
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
            else if (input == "test")
            {
                g_generate_processes = true;
                std::cout << "Automatic process generation started." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(2));
                g_generate_processes = false;
                std::cout << "Automatic process generation stopped." << std::endl;
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
