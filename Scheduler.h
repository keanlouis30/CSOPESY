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
#include "MemoryManager.h"

// ========== FCFS Scheduler ==========
class FCFSScheduler
{
private:
    ReadyQueue &ready_queue;
    ProcessCollection &running_list;
    ProcessCollection &finished_list;
    std::vector<CPU_Core *> &cpu_cores;
    std::atomic<bool> &shutdown_signal;

    void handleFinishedProcesses()
    {
        for (auto *core : cpu_cores)
        {
            std::lock_guard<std::mutex> core_lock(core->core_mtx);
            if (core->current_process != nullptr &&
                core->current_process->commandCounter >= core->current_process->totalCommands)
            {
                // std::cout << "Core " << core->get_id()
                //           << " finished process: " << core->current_process->name << std::endl;

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
            for (auto *core : cpu_cores)
            {
                if (core->is_idle())
                {
                    // std::cout << "Core " << core->get_id()
                    //           << " is idle, assigning process." << std::endl;

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

    void generate_memory_report(int tick)
    {
        // Create filename
        std::ofstream report_file("memory_stamp_" + std::to_string(tick) + ".txt");
        if (!report_file.is_open())
            return;

        // Get timestamp
        time_t now = time(nullptr);
        char time_buf[100];
        strftime(time_buf, sizeof(time_buf), "%m/%d/%Y %I:%M:%S%p", localtime(&now));

        // Get data
        int process_count = g_memory_manager.get_process_count_in_memory();
        size_t fragmentation_bytes = g_memory_manager.calculate_external_fragmentation();
        size_t fragmentation_kb = fragmentation_bytes / 1024;

        // Write to file
        report_file << "Timestamp: (" << time_buf << ")\n";
        report_file << "Number of processes in memory: " << process_count << "\n";
        report_file << "Total external fragmentation in KB: " << fragmentation_kb << "\n";
        report_file << g_memory_manager.generate_memory_snapshot(g_running_list.get_all());

        report_file.close();
    }

    // This thread waits for the tick counter to change.
    void memory_reporter_thread()
    {
        int last_tick = 0;
        while (!g_shutdown)
        {
            int current_tick = g_quantum_tick_counter.load();
            if (current_tick > last_tick)
            {
                for (int i = last_tick + 1; i <= current_tick; ++i)
                {
                    generate_memory_report(i);
                }
                last_tick = current_tick;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Check periodically
        }
    }

    void updateRunningList()
    {
        std::lock_guard<std::mutex> lock(running_list.mtx);
        running_list.processes.clear();
        for (auto *core : cpu_cores)
        {
            std::lock_guard<std::mutex> core_lock(core->core_mtx);
            if (core->current_process != nullptr)
            {
                running_list.processes.push_back(*core->current_process);
            }
        }
    }

public:
    FCFSScheduler(ReadyQueue &ready,
                  ProcessCollection &running,
                  ProcessCollection &finished,
                  std::vector<CPU_Core *> &cores,
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
    ReadyQueue &ready_queue;
    ProcessCollection &running_list;
    ProcessCollection &finished_list;
    std::vector<CPU_Core *> &cpu_cores;
    Config &config;
    std::atomic<bool> &shutdown_signal;

    void handleFinishedOrQuantumExpired()
    {
        for (auto *core : cpu_cores)
        {
            // Use a temporary shared_ptr to avoid holding the lock for too long
            std::shared_ptr<Process> p;
            {
                std::lock_guard<std::mutex> core_lock(core->core_mtx);
                p = core->current_process;
            }

            if (p != nullptr)
            {
                // Case 1: Process finished its work completely.
                if (p->commandCounter >= p->totalCommands)
                {
                    p->status = FINISHED;
                    g_finished_list.add(*p);
                    g_memory_manager.deallocate(p->pid);

                    // Release the process from the core
                    core->release_process();
                }
                // Case 2: Process ran out of its time slice (quantum).
                else if (p->quantum_remaining <= 0)
                {
                    Process preempted_process = *p;
                    preempted_process.status = READY;
                    preempted_process.assigned_core_id = -1;

                    // Put it back in the ready queue to run again later
                    ready_queue.push(preempted_process);

                    // Release the process from the core
                    core->release_process();
                    g_quantum_tick_counter++;
                }
            }
        }
    }

    void assignProcessesToIdleCores()
    {
        if (!ready_queue.isEmpty())
        {
            for (auto *core : cpu_cores)
            {
                if (core->is_idle())
                {
                    Process next_process;
                    if (ready_queue.pop(next_process))
                    {
                        // *** NEW MEMORY ALLOCATION LOGIC ***

                        // If a process is returning to the ready queue, it already has memory.
                        // A simple check is to see if its memory address is non-zero.
                        bool has_memory = (next_process.memory_size > 0);

                        if (!has_memory)
                        {
                            // This is a new process, try to allocate memory for it.
                            has_memory = g_memory_manager.allocate(next_process, g_config.mem_per_proc);
                        }

                        if (has_memory)
                        {
                            // ALLOCATION SUCCEEDED (or it already had memory)
                            next_process.quantum_max = config.quantum_cycles;
                            next_process.quantum_remaining = config.quantum_cycles;
                            next_process.assigned_core_id = core->get_id();
                            core->assign_process(next_process);
                        }
                        else
                        {
                            // ALLOCATION FAILED: Memory is full.
                            // Revert back to the TAIL of the ready queue.
                            //std::cout << "[Scheduler] Memory full. Re-queueing " << next_process.name << std::endl;
                            ready_queue.push(next_process);
                        }
                    }
                }
            }
        }
    }

    void updateRunningList()
    {
        std::lock_guard<std::mutex> lock(running_list.mtx);
        running_list.processes.clear();
        for (auto *core : cpu_cores)
        {
            std::lock_guard<std::mutex> core_lock(core->core_mtx);
            if (core->current_process != nullptr)
            {
                running_list.processes.push_back(*core->current_process);
            }
        }
    }

public:
    RoundRobinScheduler(ReadyQueue &ready,
                        ProcessCollection &running,
                        ProcessCollection &finished,
                        std::vector<CPU_Core *> &cores,
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
