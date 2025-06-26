#pragma once
#include "Status.h"
class Process{
    Config config;
    public:
        std::string name;
        int pid;
        std::vector<std::string> commands;
        int totalCommands;
        Status status;
        int max_ins = config.max_ins;
        int min_ins = config.min_ins;
        std::string creation_timestamp;
        int assigned_core_id;
        int quantum_remaining;
        int quantum_max;
        int commandCounter;

        Process(std::string n, int p, std::vector<std::string> cmds = {}, Status s = READY)
            : name(n), pid(p), commands(cmds), totalCommands(cmds.size()),
            status(s), commandCounter(0), assigned_core_id(-1)
        {
            time_t now = time(nullptr);
            char buf[100];
            strftime(buf, sizeof(buf), "%m/%d/%Y %I:%M:%S%p", localtime(&now));
            creation_timestamp = buf;
        }

        void create(){

        }

};