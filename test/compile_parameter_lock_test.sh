#!/bin/bash

# Parameter Lock System Test Compilation Script
# Tests the complete parameter lock functionality without UI dependencies

echo "🔒 Compiling Parameter Lock Test Suite..."

# Define source files needed for parameter lock testing
SOURCES=(
    "test_parameter_lock_system.cpp"
    "../src/components/midi/ParameterLockManager.cpp"
    "../src/components/midi/StepSequencer.cpp"
    "../src/components/parameter/ParameterManager.cpp"
    "../src/components/parameter/Parameter.cpp"
    "../src/components/parameter/ParameterRegistry.cpp"
    "../src/components/parameter/DefaultParameters.cpp"
    "../src/components/debug/RTEventTracer.cpp"
    "../src/components/parameter/CommandManager.cpp"
    "../src/components/parameter/Command.cpp"
)

# Define include paths
INCLUDES=(
    "-I../include"
    "-I../src"
    "-I../src/components"
    "-I../src/components/midi"
    "-I../src/components/parameter"
    "-I../src/components/debug"
    "-I../src/components/threading"
    "-I."
)

# Define compiler flags
FLAGS=(
    "-std=c++17"
    "-DDESKTOP_BUILD=1"
    "-DENABLE_EVENT_VISUALIZER=1"
    "-pthread"
    "-Wall"
    "-Wextra"
    "-g"  # Debug info
    "-O0" # No optimization for debugging
)

# Output executable
OUTPUT="parameter_lock_test"

echo "Compiling with sources:"
for src in "${SOURCES[@]}"; do
    echo "  - $src"
done

echo ""
echo "Include paths:"
for inc in "${INCLUDES[@]}"; do
    echo "  - $inc"
done

echo ""
echo "Compiler flags: ${FLAGS[*]}"
echo ""

# Compile
g++ "${FLAGS[@]}" "${INCLUDES[@]}" "${SOURCES[@]}" -o "$OUTPUT"

if [ $? -eq 0 ]; then
    echo "✅ Compilation successful! Executable: $OUTPUT"
    echo ""
    echo "🚀 Running parameter lock tests..."
    ./"$OUTPUT"
else
    echo "❌ Compilation failed!"
    exit 1
fi
