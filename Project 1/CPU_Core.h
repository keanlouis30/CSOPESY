#pragma once

#include "ProcessCollection.h" 
#include <atomic>
#include "Process.h"
#include <mutex>
#include <memory>
#include <thread>
#include <chrono>
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

    void run()
    {
        while (!shutdown_signal)
        {
            std::shared_ptr<Process> p = nullptr;

            {
                std::lock_guard<std::mutex> lock(core_mtx);
                p = current_process;
            }

            if (p)
            {

                if (p->commandCounter < p->totalCommands)
                {
                    // Execute a command
                    execute_command(*p);
                    p->commandCounter++;
                    p->quantum_remaining--; // Decrement quantum for RR

                    // Simulate execution delay
                    std::this_thread::sleep_for(std::chrono::milliseconds(g_config.delays_per_exec));
                }

                if (p->commandCounter >= p->totalCommands)
                {
                    p->status = FINISHED;
                    g_finished_list.add(*p);

                    {
                        std::lock_guard<std::mutex> lock(core_mtx);
                        current_process = nullptr;
                    }
                }
            }
            else
            {

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

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

    int get_id() {
        return core_id;
    }

    std::shared_ptr<Process> get_process() {
        std::lock_guard<std::mutex> lock(core_mtx);
        return current_process;
    }

    void release_process() {
        std::lock_guard<std::mutex> lock(core_mtx);
        current_process = nullptr;
    }

private:
    void execute_command(Process &p);
};
