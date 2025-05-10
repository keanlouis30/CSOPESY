#include<iostream>
#include<string>

int main(){
    std::cout << "\033[33m" << R"(
        ____ ____   ___  ____  _____ ____  __   __
       / ___/ ___| / _ \|  _ \| ____/ ___| \ \ / /
      | |   \___ \| | | | |_) |  _| \___ \  \ V / 
      | |___ ___) | |_| |  __/| |___ ___) |  | |  
       \____|____/ \___/|_|   |_____|____/   |_|  
    )" << "\033[0m" << std::endl;

    std::cout << "\033[1m" << "\033[32m" << "Hello, welcome to CSOPESY S14 Group 10's Command-Line Interface" << "\033[0m" << std::endl;
    std::cout << "Type" << " \033[35m" << "[help]"  << "\033[0m" << " for the basic instructions of using the CLI\n" << std::endl;
    std::string input;

    do{
        std::cout << "\033[36m" << "Command> " << "\033[0m";
        std::getline(std::cin, input);
    } while (input != "exit");
    return 0;
}