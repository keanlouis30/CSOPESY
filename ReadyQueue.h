#pragma once
class ReadyQueue
{
public:
    std::queue<Process> processes;
    std::mutex mtx;

    void push(const Process &p)
    {
        std::lock_guard<std::mutex> lock(mtx);
        processes.push(p);
    }

    bool pop(Process &p)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (processes.empty())
        {
            return false;
        }
        p = processes.front();
        processes.pop();
        return true;
    }

    bool isEmpty()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return processes.empty();
    }
};