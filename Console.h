#pragma once

#include <string>
#include "ReadyQueue.h"
#include "ProcessCollection.h"

class Console {
public:
    static void display(const std::string& process_name,
                        ReadyQueue& ready_queue,
                        ProcessCollection& running_list,
                        ProcessCollection& finished_list);
};
