#!/bin/bash

# Simple Parameter Lock Test Compilation Script
# Standalone test without external dependencies

echo "🔒 Compiling Simple Parameter Lock Test..."

# Define source files (only the test file)
SOURCES=(
    "test_simple_parameter_lock.cpp"
)

# Define compiler flags
FLAGS=(
    "-std=c++17"
    "-pthread"
    "-Wall"
    "-Wextra"
    "-g"  # Debug info
    "-O0" # No optimization for debugging
)

# Output executable
OUTPUT="simple_parameter_lock_test"

echo "Compiling standalone test: test_simple_parameter_lock.cpp"
echo "Compiler flags: ${FLAGS[*]}"
echo ""

# Compile
g++ "${FLAGS[@]}" "${SOURCES[@]}" -o "$OUTPUT"

if [ $? -eq 0 ]; then
    echo "✅ Compilation successful! Executable: $OUTPUT"
    echo ""
    echo "🚀 Running simple parameter lock tests..."
    ./"$OUTPUT"
else
    echo "❌ Compilation failed!"
    exit 1
fi
