#pragma once

#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>
#include <memory>
#include <optional>
#include <concepts>
#include <ranges>
#include <stop_token>

#include "ReadyQueue.h"
#include "ProcessCollection.h"
#include "CPU_Core.h"
#include "Config.h"
#include "Globals.h"
#include "MemoryManager.h"

// Forward declaration
void handle_io_request(Process process);

// ========== FCFS Scheduler ==========
class FCFSScheduler
{
private:
    ReadyQueue &ready_queue;
    ProcessCollection &running_list;
    ProcessCollection &finished_list;
    std::vector<std::unique_ptr<CPU_Core>> &cpu_cores;
    std::atomic<bool> &shutdown_signal;
    std::mutex scheduler_mutex;

    void handleFinishedProcesses()
    {
        std::lock_guard<std::mutex> read_lock(scheduler_mutex);
        
        for (const auto &core : cpu_cores)
        {
            if (!core) continue;
            
            std::lock_guard<std::mutex> core_lock(core->core_mtx);
            if (core->current_process && 
                core->current_process->commandCounter >= core->current_process->totalCommands)
            {
                core->current_process->status = FINISHED;
                g_finished_list.add(*core->current_process);
                core->current_process.reset();
            }
        }
    }

    void assignProcessesToIdleCores()
    {
        std::lock_guard<std::mutex> read_lock(scheduler_mutex);
        
        if (ready_queue.isEmpty()) return;
        
        for (const auto &core : cpu_cores)
        {
            if (!core || !core->is_idle()) continue;
            
            std::optional<Process> next_process = ready_queue.try_pop();
            if (next_process)
            {
                next_process->assigned_core_id = core->get_id();
                core->assign_process(std::move(*next_process));
            }
        }
    }

    void updateRunningList()
    {
        std::lock_guard<std::mutex> read_lock(scheduler_mutex);
        std::lock_guard<std::mutex> lock(running_list.mtx);
        
        running_list.processes.clear();
        running_list.processes.reserve(cpu_cores.size());
        
        for (const auto &core : cpu_cores)
        {
            if (!core) continue;
            
            std::lock_guard<std::mutex> core_lock(core->core_mtx);
            if (core->current_process)
            {
                running_list.processes.push_back(*core->current_process);
            }
        }
    }

public:
    FCFSScheduler(ReadyQueue &ready,
                  ProcessCollection &running,
                  ProcessCollection &finished,
                  std::vector<std::unique_ptr<CPU_Core>> &cores,
                  std::atomic<bool> &shutdown)
        : ready_queue(ready),
          running_list(running),
          finished_list(finished),
          cpu_cores(cores),
          shutdown_signal(shutdown)
    {
    }

    void run()
    {
        using namespace std::chrono_literals;
        
        while (!shutdown_signal.load(std::memory_order_acquire))
        {
            handleFinishedProcesses();
            assignProcessesToIdleCores();
            updateRunningList();
            std::this_thread::sleep_for(100ms);
        }
    }
};

// ========== Round Robin Scheduler ==========
class RoundRobinScheduler
{
private:
    ReadyQueue &ready_queue;
    ProcessCollection &running_list;
    ProcessCollection &finished_list;
    std::vector<std::unique_ptr<CPU_Core>> &cpu_cores;
    Config &config;
    std::atomic<bool> &shutdown_signal;
    std::mutex scheduler_mutex;

    void handleFinishedOrQuantumExpired()
    {
        std::lock_guard<std::mutex> read_lock(scheduler_mutex);
        
        for (const auto &core : cpu_cores)
        {
            if (!core) continue;
            
            std::shared_ptr<Process> p;
            {
                std::lock_guard<std::mutex> core_lock(core->core_mtx);
                p = core->current_process;
            }

            if (!p) continue;

            if (p->has_page_fault)
            {
                std::cout << "[Scheduler] PID " << p->pid << " caused a page fault. Blocking process." << std::endl;

                Process blocked_process = *p;
                blocked_process.status = BLOCKED;
                g_blocked_list.add(blocked_process);
                core->release_process();

                std::jthread(handle_io_request, std::move(blocked_process)).detach();
                continue;
            }

            if (p->commandCounter >= p->totalCommands)
            {
                p->status = FINISHED;
                g_finished_list.add(*p);
                g_memory_manager.remove_process_memory_layout(p->pid);
                core->release_process();
            }
            else if (p->quantum_remaining <= 0)
            {
                Process preempted_process = *p;
                preempted_process.status = READY;
                preempted_process.assigned_core_id = -1;
                ready_queue.push(std::move(preempted_process));
                core->release_process();
                g_quantum_tick_counter.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    void assignProcessesToIdleCores()
    {
        std::lock_guard<std::mutex> read_lock(scheduler_mutex);
        
        if (ready_queue.isEmpty()) return;
        
        for (const auto &core : cpu_cores)
        {
            if (!core || !core->is_idle()) continue;
            
            std::optional<Process> next_process = ready_queue.try_pop();
            if (next_process)
            {
                next_process->quantum_max = config.quantum_cycles;
                next_process->quantum_remaining = config.quantum_cycles;
                next_process->assigned_core_id = core->get_id();
                core->assign_process(std::move(*next_process));
            }
        }
    }

    void updateRunningList()
    {
        std::lock_guard<std::mutex> read_lock(scheduler_mutex);
        std::lock_guard<std::mutex> lock(running_list.mtx);
        
        running_list.processes.clear();
        running_list.processes.reserve(cpu_cores.size());
        
        for (const auto &core : cpu_cores)
        {
            if (!core) continue;
            
            std::lock_guard<std::mutex> core_lock(core->core_mtx);
            if (core->current_process)
            {
                running_list.processes.push_back(*core->current_process);
            }
        }
    }

public:
    RoundRobinScheduler(ReadyQueue &ready,
                        ProcessCollection &running,
                        ProcessCollection &finished,
                        std::vector<std::unique_ptr<CPU_Core>> &cores,
                        Config &conf,
                        std::atomic<bool> &shutdown)
        : ready_queue(ready),
          running_list(running),
          finished_list(finished),
          cpu_cores(cores),
          config(conf),
          shutdown_signal(shutdown)
    {
    }

    void run()
    {
        using namespace std::chrono_literals;
        
        while (!shutdown_signal.load(std::memory_order_acquire))
        {
            handleFinishedOrQuantumExpired();
            assignProcessesToIdleCores();
            updateRunningList();
            std::this_thread::sleep_for(1ms);
        }
    }
};

// ========== Scheduler Dispatcher ==========
class Scheduler
{
private:
    ReadyQueue &ready_queue;
    ProcessCollection &running_list;
    std::vector<std::unique_ptr<CPU_Core>> &cpu_cores;
    std::atomic<bool> &shutdown_signal;
    std::unique_ptr<FCFSScheduler> fcfs_scheduler;
    std::unique_ptr<RoundRobinScheduler> rr_scheduler;

public:
    Scheduler(ReadyQueue &ready,
              ProcessCollection &running,
              std::vector<std::unique_ptr<CPU_Core>> &cores,
              std::atomic<bool> &shutdown);

    void run();
};
