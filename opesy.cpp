#include <cstdlib>
#include <iostream>
#include <string>
#include <ctime>
#include <unordered_map>
#include <fstream>
#include <iomanip>
#include <regex>
#include <queue>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>

enum Status
{
    READY,
    RUNNING,
    FINISHED
};

class ScreenInterface
{
    static const int SCREENSIZE = 53;

public:
    void printBorder(std::string pos = "mid")
    {
        std::cout << (pos == "top" ? "╔" : (pos == "bottom" ? "╚" : "╠"));
        for (int i = 0; i < SCREENSIZE; i++)
        {
            std::cout << "═";
        }
        std::cout << (pos == "top" ? "╗" : (pos == "bottom" ? "╝" : "╣")) << std::endl;
    }


    std::string stripAnsiCodes(const std::string &str)
    {
        return std::regex_replace(str, std::regex("\033\\[[0-9;]*m"), "");
    }

    void printMessage(std::string message = "")
    {

        std::string plainText = message;
        message = stripAnsiCodes(message);
        int spaceLen = SCREENSIZE - message.length();

        std::cout << "║ ";
        for (int i = 0; i < spaceLen; i++)
        {

            std::cout << (i == 2 ? plainText : " ");
        }
        std::cout << "║" << std::endl;
    }
};

class Console
{

private:
    ScreenInterface SI = ScreenInterface();

public:
    std::string name;
    int currentLine;
    int totalLines;
    std::string timestamp;

    Console() : name(""), currentLine(0), totalLines(0)
    {
        time_t now = time(nullptr);
        char buf[100];
        strftime(buf, sizeof(buf), "%m/%d/%Y, %I:%M:%S %p", localtime(&now));
        timestamp = buf;
    }

    Console(const std::string &n, int total) : name(n), currentLine(0), totalLines(total)
    {
        time_t now = time(nullptr);
        char buf[100];
        strftime(buf, sizeof(buf), "%m/%d/%Y, %I:%M:%S %p", localtime(&now));
        timestamp = buf;
    }

    void display()
    {
        std::string input;
        while (true)
        {
            system("cls"); // clear screen
            SI.printBorder("top");

            SI.printMessage("\033[33mScreen: " + name + "\033[0m");
            SI.printMessage("\033[32mProcess: " + name + "\033[0m");
            SI.printMessage("\033[32mInstruction: Line " + std::to_string(currentLine + 1) + " / " + std::to_string(totalLines) + "\033[0m");
            SI.printMessage("\033[32mCreated at: " + timestamp + "\033[0m");
            SI.printMessage();
            SI.printMessage();
            SI.printMessage();

            SI.printBorder("bottom");

            std::cout << "\nType 'exit' to stop the process.\n";
            std::cout << "\033[33mScreen> \033[0m";

            std::getline(std::cin, input);
            if (input == "exit")
                break;
            if (currentLine < totalLines - 1)
                currentLine++;
        }
    }
};

class Process
{
public:
    std::string name;
    int pid;
    std::vector<std::string> commands;
    int totalCommands;
    Status status;
    int commandCounter;
    std::string creation_timestamp;
    int assigned_core_id;

    Process(std::string n, int p, std::vector<std::string> cmds = {}, Status s = READY)
        : name(n), pid(p), commands(cmds), totalCommands(cmds.size()),
          status(s), commandCounter(0), assigned_core_id(-1)
    {
        time_t now = time(nullptr);
        char buf[100];
        strftime(buf, sizeof(buf), "%m/%d/%Y %I:%M:%S%p", localtime(&now));
        creation_timestamp = buf;
    }
};

class ReadyQueue
{
public:
    std::queue<Process> processes;
    std::mutex mtx;

    void push(const Process &p)
    {
        std::lock_guard<std::mutex> lock(mtx);
        processes.push(p);
    }

    bool pop(Process &p)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (processes.empty())
        {
            return false;
        }
        p = processes.front();
        processes.pop();
        return true;
    }

    bool isEmpty()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return processes.empty();
    }
};

class ProcessCollection
{
public:
    std::vector<Process> processes;
    std::mutex mtx;

    void add(const Process &p)
    {
        std::lock_guard<std::mutex> lock(mtx);
        processes.push_back(p);
    }

    std::vector<Process> get_all()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return processes;
    }
};

class CPU_Core
{
private:
    int core_id;

    ProcessCollection &finished_list;

    std::atomic<bool> &shutdown_signal;

public:
    std::mutex core_mtx;
    std::shared_ptr<Process> current_process;

    CPU_Core(int id, ProcessCollection &finished, std::atomic<bool> &shutdown)
        : core_id(id), current_process(nullptr), finished_list(finished), shutdown_signal(shutdown) {}

    void run()
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

                if (p->commandCounter < p->totalCommands)
                {

                    std::this_thread::sleep_for(std::chrono::milliseconds(50));

                    execute_command(*p);
                    p->commandCounter++;
                }

                if (p->commandCounter >= p->totalCommands)
                {
                    p->status = FINISHED;
                    finished_list.add(*p);

                    {
                        std::lock_guard<std::mutex> lock(core_mtx);
                        current_process = nullptr;
                    }
                }
            }
            else
            {

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    bool assign_process(Process proc)
    {
        std::lock_guard<std::mutex> lock(core_mtx);
        if (current_process == nullptr)
        {
            current_process = std::make_shared<Process>(proc);
            current_process->status = RUNNING;
            return true;
        }
        return false;
    }

    bool is_idle()
    {
        std::lock_guard<std::mutex> lock(core_mtx);
        return current_process == nullptr;
    }

    int get_id() {
        return core_id;
    }

private:
    void execute_command(const Process &p)
    {

        std::ofstream outfile;
        std::string filename = p.name + ".txt";

        outfile.open(filename, std::ios_base::app);

        time_t now = time(nullptr);
        char buf[100];
        strftime(buf, sizeof(buf), "%m/%d/%Y %I:%M:%S%p", localtime(&now));

        outfile << "(" << buf << ") Core:" << core_id << " \"Hello world from " << p.name << "\"" << std::endl;
        outfile.close();
    }
};

class Scheduler
{
private:
    ReadyQueue &ready_queue;
    ProcessCollection &running_list;
    std::vector<CPU_Core *> &cpu_cores;
    std::atomic<bool> &shutdown_signal;

public:
    Scheduler(ReadyQueue &ready, ProcessCollection &running, std::vector<CPU_Core *> &cores, std::atomic<bool> &shutdown)
        : ready_queue(ready), running_list(running), cpu_cores(cores), shutdown_signal(shutdown) {}

    void run()
    {
        while (!shutdown_signal)
        {
            auto current_running = running_list.get_all();
            running_list.processes.clear();

            for (auto *core : cpu_cores)
            {
                std::lock_guard<std::mutex> lock(core->core_mtx);
                if (core->current_process != nullptr)
                {
                    Process updated_proc = *(core->current_process);
                    updated_proc.status = RUNNING;
                    running_list.add(updated_proc);
                }
            }

            for (auto *core : cpu_cores)
            {
                if (core->is_idle())
                {
                    Process p_to_run("", -1);
                    if (ready_queue.pop(p_to_run))
                    {
                        p_to_run.assigned_core_id = core->get_id();
                        if (core->assign_process(p_to_run))
                        {
                            running_list.add(p_to_run);
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
};

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

std::unordered_map<std::string, Console> screens;
ReadyQueue g_ready_queue;
ProcessCollection g_running_list;
ProcessCollection g_finished_list;
std::atomic<bool> g_shutdown = false;

int main()
{
    printBanner();

    const int NUM_CORES = 4;
    std::vector<CPU_Core *> cpu_cores;
    std::vector<std::thread> core_threads;

    for (int i = 0; i < NUM_CORES; ++i)
    {

        cpu_cores.push_back(new CPU_Core(i, g_finished_list, g_shutdown));
    }

    Scheduler scheduler(g_ready_queue, g_running_list, cpu_cores, g_shutdown);

    std::thread scheduler_thread(&Scheduler::run, &scheduler);
    for (auto *core : cpu_cores)
    {
        core_threads.emplace_back(&CPU_Core::run, core);
    }
    std::cout << "\033[32m[System] FCFS Scheduler and " << NUM_CORES << " CPU cores are now running in the background.\033[0m\n\n";

    std::string input, screenName;
    int process_id_counter = 1;

    do
    {
        std::cout << "\033[36m" << "Command> " << "\033[0m";
        std::getline(std::cin, input);

        if (input == "initialize")
        {

            std::cout << "Initializing test case: Creating 10 processes with 100 print commands each...\n";
            for (int i = 0; i < 10; ++i)
            {
                std::string proc_name = std::string("process") + (i < 9 ? "0" : "") + std::to_string(i + 1);
                std::vector<std::string> commands(100, "print");
                Process new_proc(proc_name, process_id_counter++, commands);
                g_ready_queue.push(new_proc);
            }
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
        else if (input == "scheduler-test")
        {

            std::cout << "Please use the 'initialize' command to start the test case.\n";
        }
        else if (input == "scheduler-stop")
        {

            std::cout << "Please use the 'exit' command to stop the scheduler and the application.\n";
        }
        else if (input == "report-util")
        {
            std::cout << "report-util command recognized. Doing something." << std::endl;
        }
        else if (input == "clear")
        {
            system("cls");
            std::cout.flush();
            printBanner();
        }
        else if (input == "help")
        {
            std::cout << "\033[35m" << "Available commands:" << "\033[0m" << std::endl;
            std::cout << "  initialize    - Creates 10 processes to test the FCFS scheduler." << std::endl;
            std::cout << "  screen -ls    - Lists all running and finished processes." << std::endl;
            std::cout << "  clear         - Clear the screen" << std::endl;
            std::cout << "  exit          - Stops all threads and exits the application." << std::endl;
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