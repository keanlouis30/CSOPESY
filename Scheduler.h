#pragma once

#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>

#include "ReadyQueue.h"
#include "ProcessCollection.h"
#include "CPU_Core.h"
#include "Config.h"
#include "Globals.h"

// ========== FCFS Scheduler ==========
class FCFSScheduler
{
private:
    ReadyQueue& ready_queue;
    ProcessCollection& running_list;
    ProcessCollection& finished_list;
    std::vector<CPU_Core*>& cpu_cores;
    std::atomic<bool>& shutdown_signal;

    void handleFinishedProcesses()
    {
        for (auto* core : cpu_cores)
        {
            std::lock_guard<std::mutex> core_lock(core->core_mtx);
            if (core->current_process != nullptr &&
                core->current_process->commandCounter >= core->current_process->totalCommands)
            {
                //std::cout << "Core " << core->get_id()
                //          << " finished process: " << core->current_process->name << std::endl;

                core->current_process->status = FINISHED;
                g_finished_list.add(*core->current_process);
                core->current_process = nullptr;
            }
        }
    }

    void assignProcessesToIdleCores()
    {
        if (!ready_queue.isEmpty())
        {
            for (auto* core : cpu_cores)
            {
                if (core->is_idle())
                {
                    //std::cout << "Core " << core->get_id()
                    //          << " is idle, assigning process." << std::endl;

                    Process next_process;
                    if (ready_queue.pop(next_process))
                    {
                        next_process.assigned_core_id = core->get_id();
                        core->assign_process(next_process);
                    }
                }
            }
        }
    }

    void updateRunningList()
    {
        std::lock_guard<std::mutex> lock(running_list.mtx);
        running_list.processes.clear();
        for (auto* core : cpu_cores)
        {
            std::lock_guard<std::mutex> core_lock(core->core_mtx);
            if (core->current_process != nullptr)
            {
                running_list.processes.push_back(*core->current_process);
            }
        }
    }

public:
    FCFSScheduler(ReadyQueue& ready,
                  ProcessCollection& running,
                  ProcessCollection& finished,
                  std::vector<CPU_Core*>& cores,
                  std::atomic<bool>& shutdown)
        : ready_queue(ready),
          running_list(running),
          finished_list(finished),
          cpu_cores(cores),
          shutdown_signal(shutdown)
    {}

    void run()
    {
        while (!shutdown_signal)
        {
            handleFinishedProcesses();
            assignProcessesToIdleCores();
            updateRunningList();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

// ========== Round Robin Scheduler ==========
class RoundRobinScheduler
{
private:
    ReadyQueue& ready_queue;
    ProcessCollection& running_list;
    ProcessCollection& finished_list;
    std::vector<CPU_Core*>& cpu_cores;
    Config& config;
    std::atomic<bool>& shutdown_signal;

    void handleFinishedOrQuantumExpired()
    {
        for (auto* core : cpu_cores)
        {
            std::lock_guard<std::mutex> core_lock(core->core_mtx);
            if (core->current_process != nullptr)
            {
                if (core->current_process->commandCounter >= core->current_process->totalCommands)
                {
                    core->current_process->status = FINISHED;
                    g_finished_list.add(*core->current_process);
                    core->current_process = nullptr;
                }
                else if (core->current_process->quantum_remaining <= 0)
                {
                    Process preempted_process = *core->current_process;
                    preempted_process.status = READY;
                    preempted_process.assigned_core_id = -1;
                    ready_queue.push(preempted_process);
                    core->current_process = nullptr;
                }
            }
        }
    }

    void assignProcessesToIdleCores()
    {
        if (!ready_queue.isEmpty())
        {
            for (auto* core : cpu_cores)
            {
                if (core->is_idle())
                {
                    Process next_process;
                    if (ready_queue.pop(next_process))
                    {
                        next_process.quantum_max = config.quantum_cycles;
                        next_process.quantum_remaining = config.quantum_cycles;
                        next_process.assigned_core_id = core->get_id();
                        core->assign_process(next_process);
                    }
                }
            }
        }
    }

    void updateRunningList()
    {
        std::lock_guard<std::mutex> lock(running_list.mtx);
        running_list.processes.clear();
        for (auto* core : cpu_cores)
        {
            std::lock_guard<std::mutex> core_lock(core->core_mtx);
            if (core->current_process != nullptr)
            {
                running_list.processes.push_back(*core->current_process);
            }
        }
    }

public:
    RoundRobinScheduler(ReadyQueue& ready,
                        ProcessCollection& running,
                        ProcessCollection& finished,
                        std::vector<CPU_Core*>& cores,
                        Config& conf,
                        std::atomic<bool>& shutdown)
        : ready_queue(ready),
          running_list(running),
          finished_list(finished),
          cpu_cores(cores),
          config(conf),
          shutdown_signal(shutdown)
    {}

    void run()
    {
        while (!shutdown_signal)
        {
            handleFinishedOrQuantumExpired();
            assignProcessesToIdleCores();
            updateRunningList();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
};

// ========== Scheduler Dispatcher ==========

class Scheduler
{
private:
    ReadyQueue &ready_queue;
    ProcessCollection &running_list;
    std::vector<CPU_Core *> &cpu_cores;
    std::atomic<bool> &shutdown_signal;

public:
    Scheduler(ReadyQueue &ready,
              ProcessCollection &running,
              std::vector<CPU_Core *> &cores,
              std::atomic<bool> &shutdown);

    void run();
};

