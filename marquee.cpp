#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>
#define NOMINMAX
#include <windows.h>
#include <conio.h>

class MarqueeApp {
private:
    // Console handle for positioning and colors
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    
    // Thread control
    std::atomic<bool> running{true};
    std::mutex consoleMutex;
    
    // Marquee variables
    std::atomic<int> bannerX{0};
    std::atomic<int> bannerY{0};
    std::atomic<int> deltaX{1};
    std::atomic<int> deltaY{1};
    std::atomic<int> consoleWidth{80};
    std::atomic<int> consoleHeight{25};
    
    // Performance metrics
    std::atomic<int> refreshRate{60}; // Target FPS
    std::atomic<int> pollingRate{1000}; // Input polling in Hz
    std::atomic<long long> frameCount{0};
    std::atomic<double> actualFPS{0.0};
    std::atomic<bool> screenTearing{false};
    std::atomic<int> inputDelay{0}; // in milliseconds
    
    // Input handling
    std::string currentInput;
    std::mutex inputMutex;
    
    // Banner text
    std::vector<std::string> banner = {
        " ██████╗███████╗ ██████╗ ██████╗ ███████╗███████╗██╗   ██╗",
        "██╔════╝██╔════╝██╔═══██╗██╔══██╗██╔════╝██╔════╝╚██╗ ██╔╝",
        "██║     ███████╗██║   ██║██████╔╝█████╗  ███████╗ ╚████╔╝ ",
        "██║     ╚════██║██║   ██║██╔═══╝ ██╔══╝  ╚════██║  ╚██╔╝  ",
        "╚██████╗███████║╚██████╔╝██║     ███████╗███████║   ██║   ",
        " ╚═════╝╚══════╝ ╚═════╝ ╚═╝     ╚══════╝╚══════╝   ╚═╝   "
    };
    
    int bannerWidth = 59; // Width of the banner
    int bannerHeight = 6; // Height of the banner

public:
    MarqueeApp() {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        SetConsoleOutputCP(CP_UTF8);
        
        // Hide cursor
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = false;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
        
        updateConsoleSize();
    }
    
    ~MarqueeApp() {
        // Show cursor
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = true;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
    }
    
    void updateConsoleSize() {
        CONSOLE_SCREEN_BUFFER_INFO newCsbi;
        GetConsoleScreenBufferInfo(hConsole, &newCsbi);
        consoleWidth = newCsbi.srWindow.Right - newCsbi.srWindow.Left + 1;
        consoleHeight = newCsbi.srWindow.Bottom - newCsbi.srWindow.Top + 1;
    }
    
    void setCursorPosition(int x, int y) {
        COORD coord = {static_cast<SHORT>(x), static_cast<SHORT>(y)};
        SetConsoleCursorPosition(hConsole, coord);
    }
    
    void setColor(int color) {
        SetConsoleTextAttribute(hConsole, color);
    }
    
    void clearScreen() {
        system("cls");
    }
    
    // Thread 1: Marquee Banner
    void marqueeThread() {
        auto lastTime = std::chrono::high_resolution_clock::now();
        auto frameTime = std::chrono::microseconds(1000000 / refreshRate.load());
        
        while (running) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime);
            
            if (elapsed >= frameTime) {
                std::lock_guard<std::mutex> lock(consoleMutex);
                
                // Update console size dynamically
                updateConsoleSize();
                
                // Clear previous banner position
                for (int i = 0; i < bannerHeight; i++) {
                    setCursorPosition(bannerX, bannerY + i);
                    for (int j = 0; j < bannerWidth && j < consoleWidth; j++) {
                        std::cout << " ";
                    }
                }
                
                // Update position
                bannerX += deltaX;
                bannerY += deltaY;
                
                // Bounce off edges
                if (bannerX + bannerWidth >= consoleWidth || bannerX <= 0) {
                    deltaX = -deltaX;
                    int currentX = bannerX.load();
                    int maxX = consoleWidth.load() - bannerWidth;
                    bannerX = (currentX < 0) ? 0 : ((currentX > maxX) ? maxX : currentX);
                }
                
                if (bannerY + bannerHeight >= consoleHeight - 7 || bannerY <= 0) { // Reserve space for stats + input
                    deltaY = -deltaY;
                    int currentY = bannerY.load();
                    int maxY = consoleHeight.load() - bannerHeight - 7;
                    bannerY = (currentY < 0) ? 0 : ((currentY > maxY) ? maxY : currentY);
                }
                
                // Draw banner at new position
                setColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                for (int i = 0; i < bannerHeight; i++) {
                    if (bannerY + i >= 0 && bannerY + i < consoleHeight - 7) {
                        setCursorPosition(bannerX, bannerY + i);
                        std::cout << banner[i];
                    }
                }
                
                frameCount++;
                
                // Calculate actual FPS
                static auto fpsTime = std::chrono::high_resolution_clock::now();
                static long long lastFrameCount = 0;
                auto fpsDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - fpsTime);
                
                if (fpsDuration.count() >= 1000) {
                    actualFPS = (frameCount - lastFrameCount) * 1000.0 / fpsDuration.count();
                    lastFrameCount = frameCount;
                    fpsTime = currentTime;
                    
                    // Detect screen tearing (simple heuristic)
                    screenTearing = (actualFPS > refreshRate * 1.1);
                }
                
                lastTime = currentTime;
                frameTime = std::chrono::microseconds(1000000 / refreshRate.load());
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
    
    // Thread 2: Stats Display
    void statsThread() {
        while (running) {
            std::lock_guard<std::mutex> lock(consoleMutex);
            
            // Display stats at bottom of screen (leave space for input)
            setColor(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            
            setCursorPosition(0, consoleHeight - 6);
            std::cout << "┌─ MARQUEE STATS ─────────────────────────────────────────────────┐";
            
            setCursorPosition(0, consoleHeight - 5);
            printf("│ Refresh Rate: %3d Hz │ Polling Rate: %4d Hz │ Actual FPS: %6.1f │",
                   refreshRate.load(), pollingRate.load(), actualFPS.load());
            
            setCursorPosition(0, consoleHeight - 4);
            printf("│ Screen Tearing: %-3s │ Input Delay: %3d ms │ Frames: %8lld │",
                   screenTearing.load() ? "YES" : "NO", inputDelay.load(), frameCount.load());
            
            setCursorPosition(0, consoleHeight - 3);
            std::cout << "│ Console: " << consoleWidth.load() << "x" << consoleHeight.load() 
                      << " │ Banner Pos: (" << bannerX.load() << "," << bannerY.load() 
                      << ") │ Type 'help' for commands │";
            
            setCursorPosition(0, consoleHeight - 2);
            std::cout << "└────────────────────────────────────────────────────────────────┘";
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    // Thread 3: Input Display (separate from input handling)
    void inputDisplayThread() {
        while (running) {
            std::lock_guard<std::mutex> consoleLock(consoleMutex);
            std::lock_guard<std::mutex> inputLock(inputMutex);
            
            // Display input prompt and current input at the bottom
            setColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            setCursorPosition(0, consoleHeight - 1);
            
            std::string displayText = "Command> " + currentInput;
            // Pad with spaces to clear previous text
            displayText.resize(std::min(consoleWidth.load() - 1, 80), ' ');
            std::cout << displayText;
            
            // Show cursor at input position
            setCursorPosition(9 + currentInput.length(), consoleHeight - 1);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
    // Thread 4: Input Handling
    void inputThread() {
        auto pollInterval = std::chrono::microseconds(1000000 / pollingRate.load());
        
        while (running) {
            auto inputStart = std::chrono::high_resolution_clock::now();
            
            if (_kbhit()) {
                char ch = _getch();
                
                if (ch == '\r') { // Enter key
                    auto inputEnd = std::chrono::high_resolution_clock::now();
                    inputDelay = std::chrono::duration_cast<std::chrono::milliseconds>(inputEnd - inputStart).count();
                    
                    std::string command;
                    {
                        std::lock_guard<std::mutex> lock(inputMutex);
                        command = currentInput;
                        currentInput.clear();
                    }
                    
                    processCommand(command);
                } else if (ch == '\b') { // Backspace
                    std::lock_guard<std::mutex> lock(inputMutex);
                    if (!currentInput.empty()) {
                        currentInput.pop_back();
                    }
                } else if (ch >= 32 && ch <= 126) { // Printable characters
                    std::lock_guard<std::mutex> lock(inputMutex);
                    if (currentInput.length() < 50) { // Limit input length
                        currentInput += ch;
                    }
                }
            }
            
            std::this_thread::sleep_for(pollInterval);
            pollInterval = std::chrono::microseconds(1000000 / pollingRate.load());
        }
    }
    
    void processCommand(const std::string& command) {
        std::lock_guard<std::mutex> lock(consoleMutex);
        
        if (command == "help") {
            setCursorPosition(0, 1);
            setColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            std::cout << "Available commands:                                               ";
            setCursorPosition(0, 2);
            std::cout << "  help        - Show this help message                           ";
            setCursorPosition(0, 3);
            std::cout << "  exit        - Exit the application                             ";
            setCursorPosition(0, 4);
            std::cout << "  refresh <n> - Set refresh rate (1-200 Hz)                     ";
            setCursorPosition(0, 5);
            std::cout << "  polling <n> - Set polling rate (100-5000 Hz)                  ";
            setCursorPosition(0, 6);
            std::cout << "  clear       - Clear command output                             ";
        } else if (command == "exit") {
            running = false;
        } else if (command == "clear") {
            // Clear the command output area
            for (int i = 1; i <= 10; i++) {
                setCursorPosition(0, i);
                for (int j = 0; j < consoleWidth; j++) {
                    std::cout << " ";
                }
            }
        } else if (command.substr(0, 7) == "refresh") {
            try {
                int rate = std::stoi(command.substr(8));
                if (rate >= 1 && rate <= 200) {
                    refreshRate = rate;
                    setCursorPosition(0, 1);
                    setColor(FOREGROUND_GREEN);
                    std::cout << "Refresh rate set to " << rate << " Hz                            ";
                } else {
                    setCursorPosition(0, 1);
                    setColor(FOREGROUND_RED);
                    std::cout << "Refresh rate must be between 1-200 Hz                          ";
                }
            } catch (...) {
                setCursorPosition(0, 1);
                setColor(FOREGROUND_RED);
                std::cout << "Invalid refresh rate. Use: refresh <1-200>                     ";
            }
        } else if (command.substr(0, 7) == "polling") {
            try {
                int rate = std::stoi(command.substr(8));
                if (rate >= 100 && rate <= 5000) {
                    pollingRate = rate;
                    setCursorPosition(0, 1);
                    setColor(FOREGROUND_GREEN);
                    std::cout << "Polling rate set to " << rate << " Hz                           ";
                } else {
                    setCursorPosition(0, 1);
                    setColor(FOREGROUND_RED);
                    std::cout << "Polling rate must be between 100-5000 Hz                       ";
                }
            } catch (...) {
                setCursorPosition(0, 1);
                setColor(FOREGROUND_RED);
                std::cout << "Invalid polling rate. Use: polling <100-5000>                  ";
            }
        } else if (!command.empty()) {
            setCursorPosition(0, 1);
            setColor(FOREGROUND_RED);
            std::cout << "Unknown command: '" << command << "'. Type 'help' for available commands.";
        }
    }
    
    void run() {
        clearScreen();
        
        // Show initial help
        setColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        setCursorPosition(0, 0);
        std::cout << "CSOFESXY Marquee Display - Type 'help' for commands";
        
        // Start threads
        std::thread marquee(&MarqueeApp::marqueeThread, this);
        std::thread stats(&MarqueeApp::statsThread, this);
        std::thread inputDisplay(&MarqueeApp::inputDisplayThread, this);
        std::thread input(&MarqueeApp::inputThread, this);
        
        // Wait for threads to finish
        marquee.join();
        stats.join();
        inputDisplay.join();
        input.join();
        
        clearScreen();
        std::cout << "Marquee application terminated." << std::endl;
    }
};

int main() {
    MarqueeApp app;
    app.run();
    return 0;
}