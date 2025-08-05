#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>

int main() {
    std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << "Basic compilation test successful!" << std::endl;
    return 0;
} 