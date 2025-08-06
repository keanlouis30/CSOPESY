#!/bin/bash

# OPESY OS Emulator Build Script
# This script compiles the cleaned-up OPESY OS Emulator system

echo "=== OPESY OS Emulator Build Script ==="

# Check if we're in the right directory
if [ ! -f "main.cpp" ]; then
    echo "Error: main.cpp not found. Please run this script from the CSOPESY directory."
    exit 1
fi

# Set compiler flags
CXX_FLAGS="-std=c++23 -Wall -Wextra -O2 -pthread"
INCLUDE_FLAGS="-I."

# Source files
SOURCE_FILES=(
    "main.cpp"
    "Config.cpp"
    "Instructions.cpp"
    "MemoryManager.cpp"
    "Process.cpp"
    "Scheduler.cpp"
    "CPU_Core.cpp"
    "Console.cpp"
    "Utils.cpp"
)

# Create output directory
mkdir -p build

# Compile the system
echo "Compiling OPESY OS Emulator..."

# Try to compile with available files
available_files=()
for file in "${SOURCE_FILES[@]}"; do
    if [ -f "$file" ]; then
        available_files+=("$file")
    fi
done

if [ ${#available_files[@]} -eq 0 ]; then
    echo "Error: No source files found to compile."
    exit 1
fi

# Compile command
g++ $CXX_FLAGS $INCLUDE_FLAGS "${available_files[@]}" -o build/opesy_os_emulator

if [ $? -eq 0 ]; then
    echo "✅ Compilation successful!"
    echo "Executable created: build/opesy_os_emulator"
    echo ""
    echo "To run the emulator:"
    echo "  cd build"
    echo "  ./opesy_os_emulator"
else
    echo "❌ Compilation failed!"
    echo "Please check the error messages above and fix any issues."
    exit 1
fi

echo ""
echo "=== Build Complete ==="
echo "The OPESY OS Emulator has been successfully compiled!" 