
class ProcessCollection
{
public:
    std::vector<Process> processes;
    std::mutex mtx;

    void add(const Process &p)
    {
        std::lock_guard<std::mutex> lock(mtx);
        processes.push_back(p);
    }

    std::vector<Process> get_all()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return processes;
    }
};