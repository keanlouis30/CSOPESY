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

#include "Config.h"
#include "Process.h"
#include "Process.cpp"
#include "ScreenInterface.h"
#include "Console.h"
#include "ReadyQueue.h"
#include "ProcessCollection.h"
#include "Scheduler.h"


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
Config g_config;
bool g_initialized = false;