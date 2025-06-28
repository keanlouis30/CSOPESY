#include "Scheduler.h"
#include "ReadyQueue.h"
#include "Globals.h"

// The constructor stays the same
Scheduler::Scheduler(ReadyQueue &ready,
                     ProcessCollection &running,
                     std::vector<CPU_Core *> &cores,
                     std::atomic<bool> &shutdown)
    : ready_queue(ready), running_list(running), cpu_cores(cores), shutdown_signal(shutdown)
{
}

void Scheduler::run()
{
    if (g_config.scheduler == "rr")
    {
        RoundRobinScheduler rr_sched(g_ready_queue, g_running_list, g_finished_list, cpu_cores, g_config, shutdown_signal);
        rr_sched.run(); // This handles its own loop until shutdown_signal is true
    }
    else
    {
        FCFSScheduler fcfs_sched(g_ready_queue, g_running_list, g_finished_list, cpu_cores, shutdown_signal);
        fcfs_sched.run(); // This handles its own loop until shutdown_signal is true
    }
}
