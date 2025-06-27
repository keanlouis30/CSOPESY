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


void printBanner();

std::unordered_map<std::string, Console> screens;
ReadyQueue g_ready_queue;
Process process("P1", 0);
ProcessCollection g_running_list;
ProcessCollection g_finished_list;
std::atomic<bool> g_shutdown = false;
Config g_config;
bool g_initialized = false;