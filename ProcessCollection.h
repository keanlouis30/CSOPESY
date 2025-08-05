#pragma once

#include <vector>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <chrono>
#include <optional>
#include <algorithm>
#include <ranges>
#include "Process.h"
#include "Globals.h"

class ProcessCollection
{
public:
    std::vector<Process> processes;
    mutable std::shared_mutex mtx;

    void add(Process p)
    {
        std::unique_lock<std::shared_mutex> lock(mtx);
        // Avoid duplicates
        auto it = std::ranges::find_if(processes, [&p](const Process& proc) {
            return proc.pid == p.pid;
        });
        
        if (it != processes.end()) {
            *it = std::move(p); // Update existing
        } else {
            processes.push_back(std::move(p));
        }
    }

    void remove(int pid_to_remove)
    {
        std::unique_lock<std::shared_mutex> lock(mtx);
        auto it = std::ranges::remove_if(processes, [pid_to_remove](const Process& p) {
            return p.pid == pid_to_remove;
        });

        if (it.begin() != processes.end()) {
            processes.erase(it.begin(), it.end());
        }
    }

    void clear() {
        std::unique_lock<std::shared_mutex> lock(mtx);
        processes.clear();
    }

    std::vector<Process> get_all() const
    {
        std::shared_lock<std::shared_mutex> lock(mtx);
        return processes;
    }

    size_t size() const
    {
        std::shared_lock<std::shared_mutex> lock(mtx);
        return processes.size();
    }

    bool exists(const std::string& name) const
    {
        std::shared_lock<std::shared_mutex> lock(mtx);
        return std::ranges::any_of(processes, [&name](const Process& p) {
            return p.name == name;
        });
    }

    std::optional<Process> find(const std::string& name) const {
        std::shared_lock<std::shared_mutex> lock(mtx);
        auto it = std::ranges::find_if(processes, [&name](const Process& p) {
            return p.name == name;
        });
        
        if (it != processes.end()) {
            return *it;
        }
        return std::nullopt;
    }

    bool find(const std::string& name, Process& out_process) const {
        std::shared_lock<std::shared_mutex> lock(mtx);
        auto it = std::ranges::find_if(processes, [&name](const Process& p) {
            return p.name == name;
        });
        
        if (it != processes.end()) {
            out_process = *it;
            return true;
        }
        return false;
    }

    std::optional<Process> find_by_pid(int pid) const {
        std::shared_lock<std::shared_mutex> lock(mtx);
        auto it = std::ranges::find_if(processes, [pid](const Process& p) {
            return p.pid == pid;
        });
        
        if (it != processes.end()) {
            return *it;
        }
        return std::nullopt;
    }
};