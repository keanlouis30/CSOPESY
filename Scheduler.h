#pragma once

#include "CPU_Core.h"
#include "ReadyQueue.h"
#include "ProcessCollection.h"

class FCFSScheduler
{
private:
    ReadyQueue& ready_queue;
    ProcessCollection& running_list;
    ProcessCollection& finished_list;
    std::vector<CPU_Core*>& cpu_cores;
    std::atomic<bool>& shutdown_signal;

public:
    FCFSScheduler(ReadyQueue& ready, ProcessCollection& running, ProcessCollection& finished, std::vector<CPU_Core*>& cores, std::atomic<bool>& shutdown)
        : ready_queue(ready), running_list(running), finished_list(finished), cpu_cores(cores), shutdown_signal(shutdown) {
    }

    void run()
    {
        while (!shutdown_signal)
        {
            //std::cout << "FCFS printing";

            for (auto* core : cpu_cores)
            {
                std::lock_guard<std::mutex> core_lock(core->core_mtx);
                if (core->current_process != nullptr && core->current_process->commandCounter >= core->current_process->totalCommands)
                {
                    core->current_process->status = FINISHED;
                    finished_list.add(*core->current_process);
                    core->current_process = nullptr;
                }
            }


            if (!ready_queue.isEmpty())
            {
                for (auto* core : cpu_cores)
                {
                    if (core->is_idle())
                    {
                        Process next_process("", -1);
                        if (ready_queue.pop(next_process))
                        {
                            next_process.assigned_core_id = core->get_id();
                            core->assign_process(next_process);
                        }

                    }
                }
            }


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

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};