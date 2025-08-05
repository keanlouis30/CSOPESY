#pragma once

#include <vector>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>
#include "Process.h"
#include "Globals.h"
#include <algorithm>

class ProcessCollection
{
public:
    std::vector<Process> processes;
    std::mutex mtx;

    void add(const Process &p)
    {
        std::lock_guard<std::mutex> lock(mtx);
        // Avoid duplicates
        for(auto& proc : processes) {
            if (proc.pid == p.pid) {
                proc = p; // Update existing
                return;
            }
        }
        processes.push_back(p);
    }

    void remove(int pid_to_remove)
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = std::remove_if(processes.begin(), processes.end(),
                                 [pid_to_remove](const Process& p) {
                                     return p.pid == pid_to_remove;
                                 });

        if (it != processes.end()) {
            processes.erase(it, processes.end());
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mtx);
        processes.clear();
    }

    std::vector<Process> get_all()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return processes;
    }

    bool exists(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mtx);
        for (const auto& p : processes) {
            if (p.name == name) {
                return true;
            }
        }
        return false;
    }

    bool find(const std::string& name, Process& out_process) {
        std::lock_guard<std::mutex> lock(mtx);
        for (const auto& p : processes) {
            if (p.name == name) {
                out_process = p;
                return true;
            }
        }
        return false;
    }
};