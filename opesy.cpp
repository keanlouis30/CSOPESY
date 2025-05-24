#include <cstdlib>
#include <iostream>
#include <string>
#include <ctime>
#include <unordered_map>
#include <iomanip>
#include <regex>

class ScreenInterface{
    static const int SCREENSIZE = 53;

public:
    void printBorder(std::string pos="mid"){
        std::cout << (pos == "top"? "╔" : (pos == "bottom"? "╚" : "╠") );
        for(int i=0; i< SCREENSIZE; i++){
            std::cout << "═";
        }
        std::cout << (pos == "top" ? "╗" : (pos == "bottom" ? "╝": "╣") ) << std::endl;
    }

    // Helper function to strip ANSI codes
    std::string stripAnsiCodes(const std::string& str) {
        return std::regex_replace(str, std::regex("\033\\[[0-9;]*m"), "");
    }

    void printMessage(std::string message=""){

        std:: string plainText = message;
        message = stripAnsiCodes(message);
        int spaceLen = SCREENSIZE - message.length();

        std::cout << "║ ";
        for (int i=0; i < spaceLen; i++){

            std::cout << (i == 2 ? plainText : " ");

        }
        std::cout << "║" << std::endl;

    }

};

class Console {

private:
    ScreenInterface SI = ScreenInterface();

public:
    std::string name;
    int currentLine;
    int totalLines;
    std::string timestamp;

    
    Console() : name(""), currentLine(0), totalLines(0) { 
        time_t now = time(nullptr);
        char buf[100];
        strftime(buf, sizeof(buf), "%m/%d/%Y, %I:%M:%S %p", localtime(&now));
        timestamp = buf;
    }

    Console(const std::string& n, int total) : name(n), currentLine(0), totalLines(total) { 
        time_t now = time(nullptr);
        char buf[100];
        strftime(buf, sizeof(buf), "%m/%d/%Y, %I:%M:%S %p", localtime(&now));
        timestamp = buf;
    }

    void display() {
        std::string input;
        while (true) {
            system("cls");  // clear screen
            SI.printBorder("top");

            SI.printMessage("\033[33mScreen: " + name + "\033[0m");
            SI.printMessage("\033[32mProcess: " + name + "\033[0m");
            SI.printMessage("\033[32mInstruction: Line " + std::to_string(currentLine + 1) + " / " + std::to_string(totalLines) + "\033[0m");
            SI.printMessage("\033[32mCreated at: " + timestamp + "\033[0m");
            SI.printMessage();
            SI.printMessage();
            SI.printMessage();
            
            SI.printBorder("bottom");

            
            std::cout << "\nType 'exit' to stop the process.\n";
            std::cout << "\033[33mScreen> \033[0m";

            std::getline(std::cin, input);
            if (input == "exit") break;
            if (currentLine < totalLines - 1) currentLine++;
        }
    }
};


void printBanner(){

    system("chcp 65001 > nul");

    
    std::cout << "\033[33m";
    std::cout << R"(

 ██████╗███████╗ ██████╗ ██████╗ ███████╗███████╗██╗   ██╗    ███████╗ ██╗██╗  ██╗     ██████╗  ██╗ ██████╗ 
██╔════╝██╔════╝██╔═══██╗██╔══██╗██╔════╝██╔════╝╚██╗ ██╔╝    ██╔════╝███║██║  ██║    ██╔════╝ ███║██╔═████╗
██║     ███████╗██║   ██║██████╔╝█████╗  ███████╗ ╚████╔╝     ███████╗╚██║███████║    ██║  ███╗╚██║██║██╔██║
██║     ╚════██║██║   ██║██╔═══╝ ██╔══╝  ╚════██║  ╚██╔╝      ╚════██║ ██║╚════██║    ██║   ██║ ██║████╔╝██║
╚██████╗███████║╚██████╔╝██║     ███████╗███████║   ██║       ███████║ ██║     ██║    ╚██████╔╝ ██║╚██████╔╝
 ╚═════╝╚══════╝ ╚═════╝ ╚═╝     ╚══════╝╚══════╝   ╚═╝       ╚══════╝ ╚═╝     ╚═╝     ╚═════╝  ╚═╝ ╚═════╝ 
                                                                                                            
    )" << std::endl;




    std::cout << "\033[1m" << "\033[32m" << "Hello, welcome to CSOPESY S14 Group 10's Command-Line Interface" << "\033[0m" << std::endl;
    std::cout << "Type" << " \033[35m" << "[help]"  << "\033[0m" << " for the basic instructions of using the CLI\n" << std::endl;
    std::cout.flush();
}

std::unordered_map<std::string, Console> screens;

int main(){
    printBanner();
    std::string input, screenName;

    do{
        std::cout << "\033[36m" << "Command> " << "\033[0m";
        std::getline(std::cin, input);

        //command handling
        if (input == "initialize") {
            std::cout << "initialize command recognized. Doing something." << std::endl;
        }
        else if (input.rfind("screen", 0) == 0) {
            if (input.rfind("screen -s ", 0) == 0) {
                screenName = input.substr(10); 
                if (screens.find(screenName) != screens.end()) {
                    std::cout << "Screen \"" << screenName << "\" already exists.\n";
                } else {
                    screens.emplace(screenName, Console(screenName, 20));
                    screens[screenName].display();
                }
            } else if (input.rfind("screen -r ", 0) == 0) {
                screenName = input.substr(10); 
                if (screens.find(screenName) != screens.end()) {
                    screens[screenName].display();  
                } else {
                    std::cout << "\033[31mNo screen found with name \"" << screenName << "\". \033[0m\n";
                }
            } else {
                std::cout << "\033[31m" << "Enter 'screen -s <name>' to create a new screen or 'screen -r <name>' to resume a screen" << "\033[0m" << std::endl;
            }
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