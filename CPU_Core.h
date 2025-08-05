#pragma once

#include "ProcessCollection.h"
#include <atomic>
#include "Process.h"
#include <mutex>
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include "Config.h"
#include "Status.h"
#include "Globals.h"

class CPU_Core
{
private:
    int core_id;

    std::atomic<bool> &shutdown_signal;

public:
    std::mutex core_mtx;
    std::shared_ptr<Process> current_process;

    CPU_Core(int id, ProcessCollection &finished, std::atomic<bool> &shutdown)
        : core_id(id), current_process(nullptr), shutdown_signal(shutdown) {}

    void run(); 

    bool assign_process(Process proc)
    {
        std::lock_guard<std::mutex> lock(core_mtx);
        if (current_process == nullptr)
        {
            current_process = std::make_shared<Process>(proc);
            current_process->status = RUNNING;
            return true;
        }
        return false;
    }

    bool is_idle()
    {
        std::lock_guard<std::mutex> lock(core_mtx);
        return current_process == nullptr;
    }

    int get_id()
    {
        return core_id;
    }

    std::shared_ptr<Process> get_process()
    {
        std::lock_guard<std::mutex> lock(core_mtx);
        return current_process;
    }

    void release_process()
    {
        std::lock_guard<std::mutex> lock(core_mtx);
        current_process = nullptr;
    }

private:
    void execute_command(Process &p);
};
