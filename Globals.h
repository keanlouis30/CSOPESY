#pragma once

// Only pull in std/atomic
#include <atomic>

// Forward declare your own classes instead of including full headers
class Config;
class ReadyQueue;
class ProcessCollection;

extern Config g_config;
extern ReadyQueue g_ready_queue;
extern ProcessCollection g_running_list;
extern std::atomic<bool> g_shutdown;
