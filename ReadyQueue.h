#pragma once
#include <queue>
#include <mutex>
#include <string>
#include "Process.h"
#include "Globals.h"


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

    bool exists(const std::string& name) {
        std::lock_guard<std::mutex> lock(mtx);
        std::queue<Process> temp = processes;
        while(!temp.empty()){
            if(temp.front().name == name){
                return true;
            }
            temp.pop();
        }
        return false;
    }

    bool find(const std::string& name, Process& out_process) {
        std::lock_guard<std::mutex> lock(mtx);
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