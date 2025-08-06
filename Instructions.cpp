#include "Instructions.h"
#include "Process.h"
#include "MemoryManager.h"
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cctype>

// Global memory manager reference (will be set during initialization)
extern MemoryManager g_memory_manager;

// Arithmetic instruction implementation
void ArithmeticInstruction::execute() {
    // This would interact with the process's variable table
    // For now, we'll just mark it as completed
    setCompleted(true);
}

std::string ArithmeticInstruction::serialize() const {
    std::ostringstream oss;
    oss << "ARITHMETIC " << operation << " " << operand1 << " " << operand2 << " " << result;
    return oss.str();
}

// Print instruction implementation
void PrintInstruction::execute() {
    // This would print the variable value
    // For now, we'll just mark it as completed
    setCompleted(true);
}

std::string PrintInstruction::serialize() const {
    std::ostringstream oss;
    oss << "PRINT " << variable;
    return oss.str();
}

// Read instruction implementation
void ReadInstruction::execute() {
    // This would read from memory address and store in variable
    // For now, we'll just mark it as completed
    setCompleted(true);
}

std::string ReadInstruction::serialize() const {
    std::ostringstream oss;
    oss << "READ " << variable << " " << address;
    return oss.str();
}

// Write instruction implementation
void WriteInstruction::execute() {
    // This would write variable value to memory address
    // For now, we'll just mark it as completed
    setCompleted(true);
}

std::string WriteInstruction::serialize() const {
    std::ostringstream oss;
    oss << "WRITE " << variable << " " << address;
    return oss.str();
}

// Declare instruction implementation
void DeclareInstruction::execute() {
    // This would declare a variable with the given value
    // For now, we'll just mark it as completed
    setCompleted(true);
}

std::string DeclareInstruction::serialize() const {
    std::ostringstream oss;
    oss << "DECLARE " << variable << " " << value;
    return oss.str();
}

// Sleep instruction implementation
void SleepInstruction::execute() {
    // This would put the process to sleep for the specified duration
    // For now, we'll just mark it as completed
    setCompleted(true);
}

std::string SleepInstruction::serialize() const {
    std::ostringstream oss;
    oss << "SLEEP " << duration;
    return oss.str();
}

// For loop instruction implementation
void ForInstruction::execute() {
    if (currentIteration < iterations) {
        // Execute the loop body
        for (auto& instruction : loopBody) {
            if (!instruction->isComplete()) {
                instruction->execute();
                break; // Execute one instruction per cycle
            }
        }
        
        // Check if loop body is complete
        bool bodyComplete = true;
        for (const auto& instruction : loopBody) {
            if (!instruction->isComplete()) {
                bodyComplete = false;
                break;
            }
        }
        
        if (bodyComplete) {
            currentIteration++;
            // Reset loop body for next iteration
            for (auto& instruction : loopBody) {
                instruction->setCompleted(false);
            }
        }
    }
    
    if (currentIteration >= iterations) {
        setCompleted(true);
    }
}

std::string ForInstruction::serialize() const {
    std::ostringstream oss;
    oss << "FOR " << iterations << " " << currentIteration;
    return oss.str();
}

// Instruction factory implementation
std::shared_ptr<Instruction> InstructionFactory::parseInstructionString(const std::string& instrStr, int processID) {
    std::string trimmed = trim(instrStr);
    std::vector<std::string> tokens = tokenize(trimmed, ' ');
    
    if (tokens.empty()) {
        return nullptr;
    }
    
    std::string opCode = tokens[0];
    std::transform(opCode.begin(), opCode.end(), opCode.begin(), ::toupper);
    
    if (opCode == "DECLARE" && tokens.size() >= 3) {
        std::string variable = tokens[1];
        int value = std::stoi(tokens[2]);
        return std::make_shared<DeclareInstruction>(processID, variable, value);
    }
    else if (opCode == "PRINT" && tokens.size() >= 2) {
        std::string variable = tokens[1];
        return std::make_shared<PrintInstruction>(processID, variable);
    }
    else if (opCode == "READ" && tokens.size() >= 3) {
        std::string variable = tokens[1];
        int address = std::stoi(tokens[2]);
        return std::make_shared<ReadInstruction>(processID, variable, address);
    }
    else if (opCode == "WRITE" && tokens.size() >= 3) {
        std::string variable = tokens[1];
        int address = std::stoi(tokens[2]);
        return std::make_shared<WriteInstruction>(processID, variable, address);
    }
    else if (opCode == "SLEEP" && tokens.size() >= 2) {
        int duration = std::stoi(tokens[1]);
        return std::make_shared<SleepInstruction>(processID, duration);
    }
    else if ((opCode == "ADD" || opCode == "SUB" || opCode == "MUL" || opCode == "DIV") && tokens.size() >= 4) {
        std::string operation = tokens[0];
        std::string operand1 = tokens[1];
        std::string operand2 = tokens[2];
        std::string result = tokens[3];
        return std::make_shared<ArithmeticInstruction>(processID, operation, operand1, operand2, result);
    }
    
    return nullptr;
}

std::vector<std::shared_ptr<Instruction>> InstructionFactory::parseInstructionSequence(const std::string& sequence, int processID) {
    std::vector<std::shared_ptr<Instruction>> instructions;
    std::vector<std::string> lines = tokenize(sequence, ';');
    
    for (const auto& line : lines) {
        std::string trimmed = trim(line);
        if (!trimmed.empty()) {
            auto instruction = parseInstructionString(trimmed, processID);
            if (instruction) {
                instructions.push_back(instruction);
            }
        }
    }
    
    return instructions;
}

std::vector<std::string> InstructionFactory::tokenize(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        std::string trimmed = trim(token);
        if (!trimmed.empty()) {
            tokens.push_back(trimmed);
        }
    }
    
    return tokens;
}

std::string InstructionFactory::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
} 