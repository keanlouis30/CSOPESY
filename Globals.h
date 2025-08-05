#pragma once

#include <atomic> 

class Config;
class ReadyQueue;
class ProcessCollection;
class MemoryManager;

extern Config g_config;
extern ReadyQueue g_ready_queue;
extern ProcessCollection g_running_list;
extern ProcessCollection g_blocked_list; 
extern ProcessCollection g_finished_list;
extern MemoryManager g_memory_manager;
extern std::atomic<bool> g_shutdown;
extern std::atomic<bool> g_generate_processes;

extern std::atomic<long long> g_cpu_ticks_idle;
extern std::atomic<long long> g_cpu_ticks_active;
extern std::atomic<long long> g_page_ins; 
extern std::atomic<long long> g_page_outs;

extern std::atomic<int> g_quantum_tick_counter;
void memory_reporter_thread();