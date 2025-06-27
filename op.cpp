// HEADERS
#include "Main.h"

Config g_config;
ReadyQueue g_ready_queue;
ProcessCollection g_running_list;
std::atomic<bool> g_shutdown(false);
std::atomic<bool> g_generate_processes(false);


// CURRENTLY: working on initializing

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

int init(){
    if (!g_config.loadFromFile("config.txt")) {
            std::cerr << "Error loading config. Exiting.\n";
            exit(1);
        }

        std::vector<CPU_Core*> cpu_cores; 
        std::vector<std::thread> core_threads;

        for (int i = 0; i < g_config.num_cpu; ++i) {
            CPU_Core* core = new CPU_Core(i, g_running_list, g_shutdown);
            cpu_cores.push_back(core);
        }

        for (auto* core : cpu_cores) {
            core_threads.emplace_back(&CPU_Core::run, core);
        }

        Scheduler scheduler(g_ready_queue, g_running_list, cpu_cores, g_shutdown);
        std::thread scheduler_thread(&Scheduler::run, &scheduler);

        std::thread generator_thread(process_generator_thread);

        // If you want, join here or store threads and join later
        // 1️⃣ Check if CPU cores were created
        if (cpu_cores.empty()) {
            std::cerr << "Error: No CPU cores created.\n";
            exit(1);
        }

        // 2️⃣ Check if scheduler thread is joinable (basic check)
        if (!scheduler_thread.joinable()) {
            std::cerr << "Error: Scheduler thread did not start.\n";
            exit(1);
        }

        // 3️⃣ Maybe print what was loaded from config
        std::cout << "Config loaded:\n";
        std::cout << "  num_cpu = " << g_config.num_cpu << "\n";
        std::cout << "  scheduler = " << g_config.scheduler << "\n";
        std::cout << "  quantum_cycles = " << g_config.quantum_cycles << "\n";

        // 4️⃣ Print success banner
        std::cout << "\033[32mSystem initialized successfully.\033[0m\n";

        return 1;
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
    bool initialized = false;
    std::string input;

    do {
        std::cout << "\033[36m" << "Command> " << "\033[0m";
        std::getline(std::cin, input);

    if (input == "initialize") {
        if(!initialized){
            if (init() == 1){
                initialized = true;
            };
        }
        
    } else if (input == "help") {
        std::cout << "\033[35m" << "Available commands:" << "\033[0m" << std::endl;
        std::cout << "  initialize    - Initialize the scheduler and start the system." << std::endl;
        std::cout << "  help          - Show available commands." << std::endl;
        std::cout << "  clear         - Clear the screen" << std::endl;
        std::cout << "  exit          - Exit the application." << std::endl;
    } else {
        std::cout << "\033[31mSystem not initialized. Please run 'initialize' first.\033[0m" << std::endl;
    }

    } while (!initialized);


};
