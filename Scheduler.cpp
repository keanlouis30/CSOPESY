#include "Scheduler.h"
#include "Globals.h"
#include <thread>
#include <chrono>

Scheduler::Scheduler(ReadyQueue &ready,
                     ProcessCollection &running,
                     std::vector<CPU_Core *> &cores,
                     std::atomic<bool> &shutdown)
    : ready_queue(ready), running_list(running), cpu_cores(cores), shutdown_signal(shutdown)
{
}

void Scheduler::run()
{
    // Empty for now
}

void Scheduler::run_rr()
{
    // Empty for now
}

void Scheduler::run_fcfs()
{
    // Empty for now
}
