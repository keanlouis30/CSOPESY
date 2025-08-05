#pragma once

#include "ProcessCollection.h"
#include <atomic>
#include "Process.h"
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <optional>
#include "Config.h"
#include "Status.h"
#include "Globals.h"

class CPU_Core
{
private:
    int core_id;
    std::atomic<bool> &shutdown_signal;

public:
    mutable std::shared_mutex core_mtx;
    std::shared_ptr<Process> current_process;

    CPU_Core(int id, ProcessCollection &finished, std::atomic<bool> &shutdown)
        : core_id(id), current_process(nullptr), shutdown_signal(shutdown) {}

    void run(); 

    bool assign_process(Process proc)
    {
        std::unique_lock<std::shared_mutex> lock(core_mtx);
        if (!current_process)
        {
            current_process = std::make_shared<Process>(std::move(proc));
            current_process->status = RUNNING;
            return true;
        }
        return false;
    }

    bool is_idle() const
    {
        std::shared_lock<std::shared_mutex> lock(core_mtx);
        return !current_process;
    }

    int get_id() const
    {
        return core_id;
    }

    std::shared_ptr<Process> get_process() const
    {
        std::shared_lock<std::shared_mutex> lock(core_mtx);
        return current_process;
    }

    void release_process()
    {
        std::unique_lock<std::shared_mutex> lock(core_mtx);
        current_process.reset();
    }

    std::optional<Process> steal_process()
    {
        std::unique_lock<std::shared_mutex> lock(core_mtx);
        if (!current_process) {
            return std::nullopt;
        }
        
        Process stolen_process = *current_process;
        current_process.reset();
        return std::move(stolen_process);
    }

private:
    void execute_command(Process &p);
};
