#include "Config.h"
#include "ReadyQueue.h"
#include "ProcessCollection.h"
#include "CPU_Core.h"

class RoundRobinScheduler
{
private:
    ReadyQueue &ready_queue;
    ProcessCollection &running_list;
    ProcessCollection &finished_list;
    std::vector<CPU_Core *> &cpu_cores;
    Config &config;
    std::atomic<bool> &shutdown_signal;

public:
    RoundRobinScheduler(ReadyQueue &ready, ProcessCollection &running, ProcessCollection &finished, std::vector<CPU_Core *> &cores, Config &conf, std::atomic<bool> &shutdown)
        : ready_queue(ready), running_list(running), finished_list(finished), cpu_cores(cores), config(conf), shutdown_signal(shutdown) {}

    void run()
    {
        while (!shutdown_signal)
        {
            for (auto *core : cpu_cores)
            {
                std::lock_guard<std::mutex> core_lock(core->core_mtx);
                if (core->current_process != nullptr)
                {
                    if (core->current_process->commandCounter >= core->current_process->totalCommands)
                    {
                        core->current_process->status = FINISHED;
                        finished_list.add(*core->current_process);
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
                        Process next_process("", -1);
                        if (ready_queue.pop(next_process))
                        {
                            
                            next_process.quantum_max = config.quantum_cycles;
                            next_process.quantum_remaining = config.quantum_cycles;
                            next_process.assigned_core_id = core->get_id();

                            core->assign_process(next_process);
                        }
                    }
                }
            }

           
            {
                std::lock_guard<std::mutex> lock(running_list.mtx);
                running_list.processes.clear();
                for (auto *core : cpu_cores)
                {
                  
                    std::lock_guard<std::mutex> core_lock(core->core_mtx);
                    if (core->current_process != nullptr)
                    {
                        running_list.processes.push_back(*core->current_process);
                    }
                }
            }

    
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
};