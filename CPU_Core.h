class CPU_Core
{
private:
    int core_id;

    ProcessCollection &finished_list;

    std::atomic<bool> &shutdown_signal;

public:
    std::mutex core_mtx;
    std::shared_ptr<Process> current_process;

    CPU_Core(int id, ProcessCollection &finished, std::atomic<bool> &shutdown)
        : core_id(id), current_process(nullptr), finished_list(finished), shutdown_signal(shutdown) {}

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

                    std::this_thread::sleep_for(std::chrono::milliseconds(50));

                    execute_command(*p);
                    p->commandCounter++;
                }

                if (p->commandCounter >= p->totalCommands)
                {
                    p->status = FINISHED;
                    finished_list.add(*p);

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

private:
    void execute_command(const Process &p)
    {

        std::ofstream outfile;
        std::string filename = p.name;

        outfile.open(filename, std::ios_base::app);

        time_t now = time(nullptr);
        char buf[100];
        strftime(buf, sizeof(buf), "%m/%d/%Y %I:%M:%S%p", localtime(&now));

        outfile << "(" << buf << ") Core:" << core_id << " \"Hello world from " << p.name << "\"" << std::endl;
        outfile.close();
    }
};
