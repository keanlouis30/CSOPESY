#pragma once

// Only pull in std/atomic
#include <atomic> 

// Forward declare your own classes instead of including full headers
class Config;
class ReadyQueue;
class ProcessCollection;
class MemoryManager;

extern Config g_config;
extern ReadyQueue g_ready_queue;
extern ProcessCollection g_running_list;
extern ProcessCollection g_finished_list;
extern MemoryManager g_memory_manager;
extern std::atomic<bool> g_shutdown;
extern std::atomic<bool> g_generate_processes;

extern std::atomic<int> g_quantum_tick_counter;
void memory_reporter_thread();