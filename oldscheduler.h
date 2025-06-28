#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>

#include "CPU_Core.h"
#include "Globals.h"
#include "Process.h"


class Scheduler
{
private:
    ReadyQueue &ready_queue;
    ProcessCollection &running_list;
    std::vector<CPU_Core *> &cpu_cores;
    std::atomic<bool> &shutdown_signal;

public:
    Scheduler(ReadyQueue &ready, ProcessCollection &running, std::vector<CPU_Core *> &cores, std::atomic<bool> &shutdown)
        : ready_queue(ready), running_list(running), cpu_cores(cores), shutdown_signal(shutdown) {}

    void run()
    {
        while (!shutdown_signal)
        {
            if (g_config.scheduler == "rr") { // Round Robin Logic
                // Check for finished or quantum-expired processes
                for (auto* core : cpu_cores) {
                    std::shared_ptr<Process> p = core->get_process();
                    if (p && (p->status == FINISHED || p->quantum_remaining <= 0)) {
                        if (p->status != FINISHED) { // Quantum expired
                            p->status = READY;
                            ready_queue.push(*p);
                        }
                        core->release_process();
                    }
                }

                // Assign new processes to idle cores
                if (!ready_queue.isEmpty()) {
                    for (auto* core : cpu_cores) {
                        if (core->is_idle()) {
                            Process next_process;
                            if (ready_queue.pop(next_process)) {
                                next_process.assigned_core_id = core->get_id();
                                next_process.status = RUNNING;
                                next_process.quantum_remaining = g_config.quantum_cycles;
                                core->assign_process(next_process);
                            }
                        }
                    }
                }

            } else { // FCFS Logic (existing logic)
                if (!ready_queue.isEmpty()) {
                    for (auto *core : cpu_cores) {
                        if (core->is_idle()) {
                            Process next_process;
                            if (ready_queue.pop(next_process)) {
                                next_process.assigned_core_id = core->get_id();
                                next_process.status = RUNNING;
                                core->assign_process(next_process);
                            }
                        }
                    }
                }
            }
            
            // Update the global running list for monitoring
            running_list.clear();
            for (auto* core : cpu_cores) {
                std::shared_ptr<Process> p = core->get_process();
                if (p) {
                    running_list.add(*p);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};
