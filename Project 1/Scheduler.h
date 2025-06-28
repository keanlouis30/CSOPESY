#pragma once

#include <vector>
#include <memory>
#include <atomic>

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

    void run_rr();
    void run_fcfs();

public:
    Scheduler(ReadyQueue &ready,
              ProcessCollection &running,
              std::vector<CPU_Core *> &cores,
              std::atomic<bool> &shutdown);

    void run();
};
