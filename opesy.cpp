#include<iostream>
#include<string>
#include <cstdlib>

void printBanner(){
    std::cout << "\033[33m" << "        ____ ____   ___  ____  _____ ____ __   __" << "\033[0m" << std::endl;
    std::cout << "\033[33m" << "       / ___/ ___| / _ \\|  _ \\| ____/ ___|\\ \\ / /" << "\033[0m" << std::endl;
    std::cout << "\033[33m" << "      | |   \\___ \\| | | | |_) |  _| \\___ \\ \\ V / " << "\033[0m" << std::endl;
    std::cout << "\033[33m" << "      | |___ ___) | |_| |  __/| |___ ___) | | |  " << "\033[0m" << std::endl;
    std::cout << "\033[33m" << "       \\____|____/ \\___/|_|   |_____|____/  |_|  " << "\033[0m" << std::endl;

    std::cout << "\033[1m" << "\033[32m" << "Hello, welcome to CSOPESY S14 Group 10's Command-Line Interface" << "\033[0m" << std::endl;
    std::cout << "Type" << " \033[35m" << "[help]"  << "\033[0m" << " for the basic instructions of using the CLI\n" << std::endl;
    std::cout.flush();
}
int main(){
    printBanner();
    std::string input;

    do{
        std::cout << "\033[36m" << "Command> " << "\033[0m";
        std::getline(std::cin, input);

        //command handling
        if (input == "initialize") {
            std::cout << "initialize command recognized. Doing something." << std::endl;
        }
        else if (input == "screen") {
            std::cout << "screen command recognized. Doing something." << std::endl;
        }
        else if (input == "scheduler-test") {
            std::cout << "scheduler-test command recognized. Doing something." << std::endl;
        }
        else if (input == "scheduler-stop") {
            std::cout << "scheduler-stop command recognized. Doing something." << std::endl;
        }
        else if (input == "report-util") {
            std::cout << "report-util command recognized. Doing something." << std::endl;
        }
        else if (input == "clear") {
            system("cls");
            std::cout.flush();
            printBanner(); 
        }
        else if (input == "help") {
            std::cout << "\033[35m" << "Available commands:" << "\033[0m" << std::endl;
            std::cout << "  initialize    - Initialize the system" << std::endl;
            std::cout << "  screen        - Display screen information" << std::endl;
            std::cout << "  scheduler-test- Test the scheduler" << std::endl;
            std::cout << "  scheduler-stop- Stop the scheduler" << std::endl;
            std::cout << "  report-util   - Report system utilization" << std::endl;
            std::cout << "  clear         - Clear the screen" << std::endl;
            std::cout << "  exit          - Exit the application" << std::endl;
        }
        else if (input != "exit") {
            std::cout << "\033[31m" << "Command not recognized. Type [help] for available commands." << "\033[0m" << std::endl;
        }
    } while (input != "exit");
    return 0;
}