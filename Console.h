#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>
#include <atomic>
#include <mutex>

// Forward declarations
class Process;
class ReadyQueue;
class ProcessCollection;

// Screen interface
class Screen {
public:
    virtual ~Screen() = default;
    virtual void render() = 0;
    virtual void handleInput(const std::string& input) = 0;
    virtual void update() = 0;
    virtual std::string getTitle() const = 0;
};

// Main screen implementation
class MainScreen : public Screen {
private:
    std::atomic<bool>& shutdownSignal;
    std::function<void(const std::string&)> commandHandler;
    
public:
    MainScreen(std::atomic<bool>& shutdown, std::function<void(const std::string&)> handler);
    
    void render() override;
    void handleInput(const std::string& input) override;
    void update() override;
    std::string getTitle() const override { return "OPESY OS Emulator - Main Menu"; }
    
private:
    void printBanner() const;
    void printHelp() const;
    void printStatus() const;
};

// Process screen implementation
class ProcessScreen : public Screen {
private:
    std::string processName;
    std::shared_ptr<Process> process;
    ReadyQueue& readyQueue;
    ProcessCollection& runningList;
    ProcessCollection& finishedList;
    
public:
    ProcessScreen(const std::string& name, std::shared_ptr<Process> proc,
                  ReadyQueue& ready, ProcessCollection& running, ProcessCollection& finished);
    
    void render() override;
    void handleInput(const std::string& input) override;
    void update() override;
    std::string getTitle() const override { return "Process: " + processName; }
    
private:
    void printProcessInfo() const;
    void printInstructions() const;
    void printVariables() const;
    void printMemoryInfo() const;
    void executeInstruction(const std::string& instruction);
};

// Console manager singleton
class ConsoleManager {
private:
    // Singleton instance
    static std::unique_ptr<ConsoleManager> instance;
    
    // Screen management
    std::shared_ptr<Screen> currentScreen;
    std::unordered_map<std::string, std::shared_ptr<Screen>> screens;
    std::mutex screenMutex;
    
    // Process management
    std::unordered_map<std::string, std::shared_ptr<Process>> processNameMap;
    std::mutex processMutex;
    
    // Global references
    ReadyQueue& readyQueue;
    ProcessCollection& runningList;
    ProcessCollection& finishedList;
    std::atomic<bool>& shutdownSignal;
    
    // Command handling
    std::function<void(const std::string&)> commandHandler;
    
    // Private constructor for singleton
    ConsoleManager(ReadyQueue& ready, ProcessCollection& running, 
                  ProcessCollection& finished, std::atomic<bool>& shutdown);
    
    // Utility methods
    void initializeMainScreen();
    void switchToScreen(const std::string& screenName);
    void createProcessScreen(const std::string& processName);
    void removeProcessScreen(const std::string& processName);
    
public:
    // Singleton access
    static ConsoleManager& getInstance(ReadyQueue& ready, ProcessCollection& running,
                                     ProcessCollection& finished, std::atomic<bool>& shutdown);
    static ConsoleManager& getInstance();
    
    // Prevent copying
    ConsoleManager(const ConsoleManager&) = delete;
    ConsoleManager& operator=(const ConsoleManager&) = delete;
    
    // Screen management
    void switchToMainScreen();
    void switchToProcessScreen(const std::string& processName);
    void updateCurrentScreen();
    
    // Process management
    void addProcess(std::shared_ptr<Process> process);
    void removeProcess(const std::string& processName);
    std::shared_ptr<Process> getProcess(const std::string& processName);
    std::vector<std::shared_ptr<Process>> getAllProcesses();
    
    // Command handling
    void setCommandHandler(std::function<void(const std::string&)> handler);
    void handleCommand(const std::string& command);
    
    // Display utilities
    static void setColor(int color);
    static void resetColor();
    static void clearScreen();
    static void printBanner();
    
    // Input/output
    std::string getInput(const std::string& prompt = "Command> ");
    void printOutput(const std::string& message);
    void printError(const std::string& error);
    void printSuccess(const std::string& message);
    
    // Status display
    void displayProcessList() const;
    void displaySystemStatus() const;
    void displayMemoryStatus() const;
    void displaySchedulerStatus() const;
    void display(const std::string& process_name, ReadyQueue& ready_queue,
             ProcessCollection& running_list, ProcessCollection& finished_list);
    
    // Utility methods
    bool isProcessExists(const std::string& processName) const;
    std::string getCurrentScreenName() const;
    void refreshDisplay();
};
