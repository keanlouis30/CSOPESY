#pragma once
#include "CPU_Core.h"
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
        bool assigned;
        if (!ready_queue.isEmpty())
        {
            // Wait until a core is idle
            for (auto *core : cpu_cores)
            {
                if (core->is_idle()) // if there is a core
                {
                    Process next_process("", -1);
                    if (ready_queue.pop(next_process))
                    {
                        next_process.assigned_core_id = core->get_id();
                        next_process.status = RUNNING;
                        if (core->assign_process(next_process))
                        {
                            running_list.add(next_process);
                            assigned = true;
                        }
                    }
                    break; // one process at a time for FCFS
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        {
            std::lock_guard<std::mutex> lock(running_list.mtx);
            running_list.processes.clear(); // clear running processes list

            for (auto *core : cpu_cores)
            {
                std::lock_guard<std::mutex> core_lock(core->core_mtx);
                if (core->current_process != nullptr && core->current_process->status != FINISHED)
                {
                    running_list.processes.push_back(*core->current_process);
                }
            }
        }
    }
};
