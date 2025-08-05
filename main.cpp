// HEADERS
#include "Main.h"
#include "MemoryManager.h"
#include <unordered_map>
#include <string>
#include <functional>
#include <random>
#include <sstream>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <memory>
#include <optional>
#include <array>

Config g_config;
ReadyQueue g_ready_queue;
ProcessCollection g_running_list;
ProcessCollection g_blocked_list;
ProcessCollection g_finished_list;
MemoryManager g_memory_manager;
std::atomic<bool> g_shutdown(false);
std::atomic<bool> g_generate_processes(false);
std::atomic<int> g_quantum_tick_counter(0);
std::atomic<long long> g_cpu_ticks_idle(0);
std::atomic<long long> g_cpu_ticks_active(0);
std::atomic<long long> g_page_ins(0);
std::atomic<long long> g_page_outs(0);

void generate_memory_report(int tick)
{
    std::ofstream report_file("memory_stamp_" + std::to_string(tick) + ".txt");
    if (!report_file.is_open())
    {
        std::cerr << "Error: Could not open memory_stamp_" << tick << ".txt for writing." << std::endl;
        return;
    }

    time_t now = time(nullptr);
    char time_buf[100];
    strftime(time_buf, sizeof(time_buf), "%m/%d/%Y %I:%M:%S%p", localtime(&now));

    int process_count = g_memory_manager.get_active_process_count();
    size_t free_bytes = g_memory_manager.get_free_memory_in_bytes();

    report_file << "Timestamp: (" << time_buf << ")\n";
    report_file << "Number of processes in memory: " << process_count << "\n";
    report_file << "Total free memory: " << free_bytes << " B\n\n";

    report_file.close();
}

void display_vmstat() {
    long long total_cpu_ticks = g_cpu_ticks_active.load() + g_cpu_ticks_idle.load();
    
    size_t total_mem_bytes = g_config.max_overall_mem;
    size_t free_mem_bytes = g_memory_manager.get_free_memory_in_bytes();
    size_t used_mem_bytes = total_mem_bytes - free_mem_bytes;

    long long pages_in = g_page_ins.load();
    long long pages_out = g_page_outs.load();

    system("clear"); 
    std::cout << "--- Virtual Memory Statistics ---\n\n";

    std::cout << "-- Memory (Bytes) --\n";
    std::cout << std::setw(12) << "Total: " << total_mem_bytes << "\n";
    std::cout << std::setw(12) << "Used: " << used_mem_bytes << "\n";
    std::cout << std::setw(12) << "Free: " << free_mem_bytes << "\n\n";

    std::cout << "-- CPU Ticks --\n";
    std::cout << std::setw(12) << "Active: " << g_cpu_ticks_active.load() << " (Executing Instructions)\n";
    std::cout << std::setw(12) << "Idle: " << g_cpu_ticks_idle.load() << " (Cores free)\n";
    std::cout << std::setw(12) << "Total: " << total_cpu_ticks << "\n\n";

    std::cout << "-- Paging --\n";
    std::cout << std::setw(12) << "Paged In: " << pages_in << " (From Backing Store)\n";
    std::cout << std::setw(12) << "Paged Out: " << pages_out << " (Evicted to Backing Store)\n\n";
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

void display_process_smi()
{
    int busy_cores = 0;
    auto running_processes = g_running_list.get_all();
    for (const auto &p : running_processes)
    {
        if (p.status == RUNNING)
            busy_cores++;
    }
    float cpu_utilization = (g_config.num_cpu > 0) ? (static_cast<float>(busy_cores) / g_config.num_cpu) * 100.0f : 0.0f;

    size_t total_mem_bytes = g_config.max_overall_mem;
    size_t free_mem_bytes = g_memory_manager.get_free_memory_in_bytes();
    size_t used_mem_bytes = total_mem_bytes - free_mem_bytes;
    float mem_utilization = (total_mem_bytes > 0) ? (static_cast<float>(used_mem_bytes) / total_mem_bytes) * 100.0f : 0.0f;

    float used_mem_kib = static_cast<float>(used_mem_bytes) / 1024;
    float total_mem_kib = static_cast<float>(total_mem_bytes) / 1024;

    system("clear");

    std::cout << "+------------------------------------------------------+\n";
    std::cout << "| PROCESS-SMI v01.00      Driver Version: 01.00        |\n";
    std::cout << "+-------------------------+----------------------------+\n";

    std::cout << "| CPU-Util: " << std::fixed << std::setprecision(2) << std::setw(6) << cpu_utilization << "%"
              << "       | Memory Usage: " << std::setw(7) << used_mem_kib << "KiB / "
              << std::setw(7) << total_mem_kib << "KiB    |\n";

    std::cout << "|                         | Memory-Util: " << std::setw(6) << mem_utilization << "%"
              << "                      |\n";

    std::cout << "+------------------------------------------------------+\n";
    std::cout << "| Running processes and physical memory usage:         |\n";
    std::cout << "+-------------------------+----------------------------+\n";
    std::cout << "| PID      | Process Name   | Physical Memory (KiB)    |\n";
    std::cout << "+----------+----------------+--------------------------+\n";

    if (running_processes.empty())
    {
        std::cout << "| (No processes are currently running on a core)       |\n";
    }
    else
    {
        for (const auto &p : running_processes)
        {
            int frames_used = g_memory_manager.get_frame_count_for_process(p.pid);
            size_t bytes_used = frames_used * g_config.mem_per_frame;
            float kib_used = static_cast<float>(bytes_used) / 1024;

            std::cout << "| " << std::left << std::setw(8) << p.pid
                      << " | " << std::setw(14) << p.name
                      << " | " << std::right << std::setw(21) << std::fixed << std::setprecision(2) << kib_used << " KiB |\n";
        }
    }
    std::cout << "+----------+----------------+--------------------------+\n\n";
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

    g_memory_manager.initialize(g_config.max_overall_mem, g_config.mem_per_frame);

    std::unordered_map<std::string, std::function<void()>> screens; // changed this because Console is static
    std::vector<std::unique_ptr<CPU_Core>> cpu_cores;
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
                    cpu_cores.push_back(std::make_unique<CPU_Core>(i, g_running_list, g_shutdown));
                }

                for (const auto &core : cpu_cores)
                {
                    core_threads.emplace_back(&CPU_Core::run, core.get());
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
                std::cout << "  process-smi   - Show memory and process information." << std::endl;
                std::cout << "  vmstat        - Show detailed memory statistics." << std::endl;
                std::cout << "  test-paging   - Test the demand paging system." << std::endl;
                std::cout << "  test-separate-pages - Test separate instruction/variable pages." << std::endl;
                std::cout << "  scheduler-start - Start the scheduler." << std::endl;
                std::cout << "  scheduler-stop - Stop the scheduler." << std::endl;
                std::cout << "  report-util   - Generate utilization report." << std::endl;
                std::cout << "  process-smi   - Prints summarized view of GPU and CPU usage." << std::endl;
                std::cout << "  vmstat        - Provides a detailed view of the active/inactive processes, available/used memory, and pages." << std::endl;
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
                                    << p.commandCounter << " / " << p.totalCommands;

                            std::cout << "\n";
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
                        newProcess.totalCommands = instructions.size();  // <- Fix!
                        g_ready_queue.push(newProcess);

                        std::cout << "Loaded instructions for " << processName << ":\n";
                        for (const auto& instr : instructions) {
                            std::cout << instr << std::endl;
                        }
                        std::cout << "Created process \"" << processName << "\" with " << instructions.size() << " instructions.\n";
                        std::cout << "[DEBUG] Commands size for process " << processName << ": " << newProcess.commands.size() << "\n";


                        screens.emplace(processName, [&]() {
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
                            << "  screen -c <name> <mem_size> "<<"<" << "Instructions" << ">" << ": Clear screen\n"
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
            else if (input == "process-smi")
            {
            //     display_process_smi();
            // }
            // else if (input == "vmstat") 
            // {
            //     display_vmstat();
                std::cout << "=== Process and Memory Information ===\n";
                
                // Memory summary
                size_t total_memory = g_config.max_overall_mem;
                size_t used_memory = g_memory_manager.get_used_frames() * g_config.mem_per_frame;
                size_t free_memory = g_memory_manager.get_free_frames() * g_config.mem_per_frame;
                
                std::cout << "Memory Summary:\n";
                std::cout << "  Total Memory: " << total_memory << " bytes\n";
                std::cout << "  Used Memory: " << used_memory << " bytes\n";
                std::cout << "  Free Memory: " << free_memory << " bytes\n";
                std::cout << "  Utilization: " << std::fixed << std::setprecision(1) 
                          << (static_cast<double>(used_memory) / total_memory) * 100.0 << "%\n\n";
                
                // Running processes
                std::cout << "Running Processes:\n";
                auto running = g_running_list.get_all();
                if (running.empty())
                {
                    std::cout << "  (None)\n";
                }
                else
                {
                    for (const auto& p : running)
                    {
                        std::cout << "  " << p.name << " (PID: " << p.pid << ")\n";
                        std::cout << "    Memory: " << p.mem_size << " bytes\n";
                        std::cout << "    Status: " << (p.status == RUNNING ? "Running" : "Blocked") << "\n";
                        std::cout << "    Core: " << p.assigned_core_id << "\n";
                    }
                }
                
                std::cout << "\n";
            }
            else if (input == "vmstat")
            {
                std::cout << g_memory_manager.generate_vmstat_report();
                std::cout << "\nNote: Page fault rate is a key metric for evaluation.\n";
            }
            else if (input == "test")
            {
                g_generate_processes = true;
                std::cout << "Automatic process generation started." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(2));
                g_generate_processes = false;
                std::cout << "Automatic process generation stopped." << std::endl;
            }
            else if (input == "test-paging")
            {
                std::cout << "=== Testing Demand Paging System ===\n";
                
                // Create a test process with page table
                int test_pid = 999;
                size_t test_memory_size = 1024; // 1KB
                
                if (g_memory_manager.create_page_table(test_pid, test_memory_size)) {
                    std::cout << "Created page table for test process " << test_pid << "\n";
                    
                    // Test memory access to trigger page faults
                    std::cout << "Testing memory access (should trigger page faults):\n";
                    
                    // Access different pages to trigger page faults
                    for (size_t addr = 0; addr < test_memory_size; addr += g_config.mem_per_frame) {
                        bool success = g_memory_manager.access_memory(test_pid, addr, false); // Read access
                        if (success) {
                            std::cout << "  Access to address 0x" << std::hex << addr << std::dec 
                                      << " successful (page " << addr / g_config.mem_per_frame << ")\n";
                        } else {
                            std::cout << "  Access to address 0x" << std::hex << addr << std::dec 
                                      << " failed\n";
                        }
                    }
                    
                    // Test write access
                    std::cout << "Testing write access:\n";
                    bool write_success = g_memory_manager.access_memory(test_pid, 0, true);
                    std::cout << "  Write to address 0x0: " << (write_success ? "successful" : "failed") << "\n";
                    
                    // Show statistics
                    std::cout << "\nPaging Statistics:\n";
                    std::cout << "  Pages paged in: " << g_memory_manager.get_pages_paged_in() << "\n";
                    std::cout << "  Pages paged out: " << g_memory_manager.get_pages_paged_out() << "\n";
                    
                    // Clean up
                    g_memory_manager.remove_page_table(test_pid);
                    std::cout << "Test completed.\n";
                } else {
                    std::cout << "Failed to create page table for test.\n";
                }
            }
            else if (input == "test-separate-pages")
            {
                std::cout << "=== Testing Separate Instruction and Variable Pages ===\n";
                
                // Create a test process with separate instruction and variable pages
                int test_pid = 888;
                size_t instruction_count = 50;  // 50 instructions
                size_t variable_count = 32;     // 32 variables (uint16_t)
                
                if (g_memory_manager.create_process_memory_layout(test_pid, instruction_count, variable_count)) {
                    std::cout << "Created memory layout for test process " << test_pid << "\n";
                    std::cout << "  Instructions: " << instruction_count << "\n";
                    std::cout << "  Variables: " << variable_count << "\n";
                    
                    // Test instruction access
                    std::cout << "\nTesting instruction access:\n";
                    for (int i = 0; i < 10; i++) {
                        bool success = g_memory_manager.access_instruction(test_pid, i, false);
                        std::cout << "  Access instruction " << i << ": " << (success ? "successful" : "failed") << "\n";
                    }
                    
                    // Test variable access
                    std::cout << "\nTesting variable access:\n";
                    for (int i = 0; i < 5; i++) {
                        bool success = g_memory_manager.access_variable(test_pid, i, true); // Write access
                        std::cout << "  Access variable " << i << ": " << (success ? "successful" : "failed") << "\n";
                    }
                    
                    // Show detailed statistics
                    std::cout << "\nDetailed Statistics:\n";
                    std::cout << "  Pages paged in: " << g_memory_manager.get_pages_paged_in() << "\n";
                    std::cout << "  Pages paged out: " << g_memory_manager.get_pages_paged_out() << "\n";
                    std::cout << "  Page faults: " << g_memory_manager.get_page_faults() << "\n";
                    std::cout << "  Page fault rate: " << std::fixed << std::setprecision(4) 
                              << g_memory_manager.get_page_fault_rate() * 100.0 << "%\n";
                    
                    // Show process memory layout
                    std::cout << "\nProcess Memory Layout:\n";
                    std::cout << g_memory_manager.generate_process_memory_report(test_pid);
                    
                    // Show page tables
                    std::cout << "\nPage Tables:\n";
                    std::cout << g_memory_manager.generate_page_table_report(test_pid);
                    
                    // Clean up
                    g_memory_manager.remove_process_memory_layout(test_pid);
                    std::cout << "Test completed.\n";
                } else {
                    std::cout << "Failed to create memory layout for test.\n";
                }
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

        // CPU cores are automatically cleaned up by unique_ptr

        std::cout << "\033[32m[System] All threads terminated. Goodbye!\033[0m\n";
    }
    if (exit == 9)
    {
        std::cout << "\n\033[33m[System] Shutdown initiated.\033[0m\n";
    }
    return 0;
};
