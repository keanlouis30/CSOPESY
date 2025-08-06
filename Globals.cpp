#include "Globals.h"
#include "Config.h"
#include "ReadyQueue.h"
#include "ProcessCollection.h"
#include "MemoryManager.h"

// Define the actual instances of the global variables
Config g_config;
ReadyQueue g_ready_queue;
ProcessCollection g_running_list;
ProcessCollection g_finished_list;
MemoryManager g_memory_manager;

std::atomic<bool> g_shutdown{false};
std::atomic<bool> g_generate_processes{true};

std::atomic<int> g_quantum_tick_counter{0};
