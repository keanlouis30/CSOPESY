#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>

// Forward declarations
class Process;

// Base instruction class
class Instruction {
protected:
    int lineCount;
    int pid;
    std::string opCode;
    bool completed;
    
public:
    Instruction(int processID, const std::string& op) 
        : lineCount(0), pid(processID), opCode(op), completed(false) {}
    
    virtual ~Instruction() = default;
    
    // Core interface
    virtual void execute() = 0;
    virtual std::string serialize() const = 0;
    virtual bool isComplete() const { return completed; }
    
    // Getters
    int getPID() const { return pid; }
    const std::string& getOpCode() const { return opCode; }
    int getLineCount() const { return lineCount; }
    
    // Utility methods
    void setCompleted(bool comp) { completed = comp; }
    void setLineCount(int count) { lineCount = count; }
};

// Arithmetic instruction
class ArithmeticInstruction : public Instruction {
private:
    std::string operation;
    std::string operand1, operand2, result;
    
public:
    ArithmeticInstruction(int processID, const std::string& op, 
                         const std::string& op1, const std::string& op2, 
                         const std::string& res)
        : Instruction(processID, "ARITHMETIC"), operation(op), 
          operand1(op1), operand2(op2), result(res) {}
    
    void execute() override;
    std::string serialize() const override;
};

// Print instruction
class PrintInstruction : public Instruction {
private:
    std::string variable;
    
public:
    PrintInstruction(int processID, const std::string& var)
        : Instruction(processID, "PRINT"), variable(var) {}
    
    void execute() override;
    std::string serialize() const override;
};

// Read instruction
class ReadInstruction : public Instruction {
private:
    std::string variable;
    int address;
    
public:
    ReadInstruction(int processID, const std::string& var, int addr)
        : Instruction(processID, "READ"), variable(var), address(addr) {}
    
    void execute() override;
    std::string serialize() const override;
};

// Write instruction
class WriteInstruction : public Instruction {
private:
    std::string variable;
    int address;
    
public:
    WriteInstruction(int processID, const std::string& var, int addr)
        : Instruction(processID, "WRITE"), variable(var), address(addr) {}
    
    void execute() override;
    std::string serialize() const override;
};

// Declare instruction
class DeclareInstruction : public Instruction {
private:
    std::string variable;
    int value;
    
public:
    DeclareInstruction(int processID, const std::string& var, int val)
        : Instruction(processID, "DECLARE"), variable(var), value(val) {}
    
    void execute() override;
    std::string serialize() const override;
};

// Sleep instruction
class SleepInstruction : public Instruction {
private:
    int duration;
    
public:
    SleepInstruction(int processID, int dur)
        : Instruction(processID, "SLEEP"), duration(dur) {}
    
    void execute() override;
    std::string serialize() const override;
};

// For loop instruction
class ForInstruction : public Instruction {
private:
    int iterations;
    int currentIteration;
    std::vector<std::shared_ptr<Instruction>> loopBody;
    
public:
    ForInstruction(int processID, int iter, const std::vector<std::shared_ptr<Instruction>>& body)
        : Instruction(processID, "FOR"), iterations(iter), currentIteration(0), loopBody(body) {}
    
    void execute() override;
    std::string serialize() const override;
    bool isComplete() const override { return currentIteration >= iterations; }
};

// Instruction factory
class InstructionFactory {
public:
    static std::shared_ptr<Instruction> parseInstructionString(const std::string& instrStr, int processID);
    static std::vector<std::shared_ptr<Instruction>> parseInstructionSequence(const std::string& sequence, int processID);
    
private:
    static std::vector<std::string> tokenize(const std::string& str, char delimiter);
    static std::string trim(const std::string& str);
};