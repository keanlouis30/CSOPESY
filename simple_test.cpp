#include <iostream>
#include <vector>
#include <string>

// Simple test without threading
int main() {
    std::cout << "Simple compilation test successful!" << std::endl;
    std::vector<int> test_vec = {1, 2, 3, 4, 5};
    std::string test_str = "Hello World";
    
    std::cout << "Vector size: " << test_vec.size() << std::endl;
    std::cout << "String: " << test_str << std::endl;
    
    return 0;
} 