#include "Scheduler.h"
#include "ReadyQueue.h"

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
        std::shared_ptr<Process> p = core->get_process();
        if (p && (p->status == FINISHED || p->quantum_remaining <= 0))
        {
            if (p->status != FINISHED)
            {
                p->status = READY;
                g_ready_queue.push(*p);
            }
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
                    next_process.quantum_remaining = g_config.quantum_cycles;
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
