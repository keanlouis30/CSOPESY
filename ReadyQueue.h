#pragma once
#include <queue>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>
#include <optional>
#include <memory>
#include "Process.h"
#include "Globals.h"

class ReadyQueue
{
public:
    std::queue<Process> processes;
    mutable std::shared_mutex mtx;

    void push(Process p)
    {
        std::unique_lock<std::shared_mutex> lock(mtx);
        processes.push(std::move(p));
    }

    bool pop(Process &p)
    {
        std::unique_lock<std::shared_mutex> lock(mtx);
        if (processes.empty())
        {
            return false;
        }
        p = std::move(processes.front());
        processes.pop();
        return true;
    }

    std::optional<Process> try_pop()
    {
        std::unique_lock<std::shared_mutex> lock(mtx);
        if (processes.empty())
        {
            return std::nullopt;
        }
        Process p = std::move(processes.front());
        processes.pop();
        return std::move(p);
    }

    bool isEmpty() const
    {
        std::shared_lock<std::shared_mutex> lock(mtx);
        return processes.empty();
    }

    size_t size() const
    {
        std::shared_lock<std::shared_mutex> lock(mtx);
        return processes.size();
    }

    bool exists(const std::string& name) const {
        std::shared_lock<std::shared_mutex> lock(mtx);
        std::queue<Process> temp = processes;
        while(!temp.empty()){
            if(temp.front().name == name){
                return true;
            }
            temp.pop();
        }
        return false;
    }

    std::optional<Process> find(const std::string& name) const {
        std::shared_lock<std::shared_mutex> lock(mtx);
        std::queue<Process> temp = processes;
        while(!temp.empty()){
            if(temp.front().name == name){
                return temp.front();
            }
            temp.pop();
        }
        return std::nullopt;
    }

    bool find(const std::string& name, Process& out_process) const {
        std::shared_lock<std::shared_mutex> lock(mtx);
        std::queue<Process> temp = processes;
        while(!temp.empty()){
            if(temp.front().name == name){
                out_process = temp.front();
                return true;
            }
            temp.pop();
        }
        return false;
    }
};