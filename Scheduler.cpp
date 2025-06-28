#include "Scheduler.h"
#include "ReadyQueue.h"
#include "Globals.h"

Scheduler::Scheduler(ReadyQueue &ready,
                     ProcessCollection &running,
                     std::vector<CPU_Core *> &cores,
                     std::atomic<bool> &shutdown)
    : ready_queue(ready), running_list(running), cpu_cores(cores), shutdown_signal(shutdown)
{
}

void Scheduler::run()
{
    while (!shutdown_signal)
    {
        if (g_config.scheduler == "rr")
        {
            run_rr();
        }
        else
        {
            run_fcfs();
        }

        // Update global running list for monitoring
        running_list.clear();
        for (auto *core : cpu_cores)
        {
            std::shared_ptr<Process> p = core->get_process();
            if (p)
            {
                running_list.add(*p);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void Scheduler::run_rr()
{
    for (auto *core : cpu_cores)
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
           
            if (!ready_queue.isEmpty())
            {
                for (auto *core : cpu_cores)
                {
                    if (core->is_idle())
                    {
                        Process next_process;
                        if (ready_queue.pop(next_process))
                        {
                            
                            next_process.quantum_max = g_config.quantum_cycles;
                            next_process.quantum_remaining = g_config.quantum_cycles;
                            next_process.assigned_core_id = core->get_id();

                            core->assign_process(next_process);
                        }
                    }
                }
            }
}

void Scheduler::run_fcfs()
{
    for (auto *core : cpu_cores)
    {
        std::shared_ptr<Process> p = core->get_process();
        if (p && p->status == FINISHED)
        {
            g_finished_list.add(*p); 
            core->release_process();
        }
    }

    if (!ready_queue.isEmpty())
    {
        for (auto *core : cpu_cores)
        {
            if (core->is_idle())
            {
                Process next_process;
                if (ready_queue.pop(next_process))
                {
                    next_process.assigned_core_id = core->get_id();
                    next_process.status = RUNNING;
                    core->assign_process(next_process);
                }
            }
        }
    }
}
