#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <array>

// Test modern C++ features compatible with current GCC
int main() {
    std::cout << "Testing C++ features compatible with current GCC..." << std::endl;
    
    // Test std::unique_ptr
    auto ptr = std::unique_ptr<int>(new int(100));
    std::cout << "std::unique_ptr works: " << *ptr << std::endl;
    
    // Test std::mutex
    std::mutex mtx;
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "std::mutex works" << std::endl;
    }
    
    // Test std::atomic
    std::atomic<int> atomic_value{0};
    atomic_value.fetch_add(1, std::memory_order_relaxed);
    std::cout << "std::atomic works: " << atomic_value.load() << std::endl;
    
    // Test std::array
    std::array<int, 5> numbers = {1, 2, 3, 4, 5};
    int sum = 0;
    for (const auto& num : numbers) {
        sum += num;
    }
    std::cout << "std::array works: sum = " << sum << std::endl;
    
    // Test chrono
    auto start = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "chrono works: slept for " << duration.count() << "ms" << std::endl;
    
    // Test std::thread
    std::thread test_thread([]() {
        std::cout << "std::thread works" << std::endl;
    });
    test_thread.join();
    
    std::cout << "All compatible C++ features tested successfully!" << std::endl;
    return 0;
} 