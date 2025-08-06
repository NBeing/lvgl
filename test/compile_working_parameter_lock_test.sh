#!/bin/bash

# Working Parameter Lock Test Compilation Script
echo "🔒 Compiling Working Parameter Lock Tests..."

# Define minimal source files for working test
SOURCES=(
    "working_parameter_lock_test.cpp"
    "../src/components/midi/ParameterLockManager.cpp"
    "../src/components/midi/StepSequencer.cpp"
)

# Define include paths
INCLUDES=(
    "-I../include"
    "-I../src"
    "-I../src/components"
    "-I../src/components/midi"
    "-I../src/components/parameter"
    "-I."
)

# Define compiler flags - disable event tracer to avoid dependencies
FLAGS=(
    "-std=c++17"
    "-DDESKTOP_BUILD=1"
    "-DDISABLE_EVENT_TRACER=1"
    "-pthread"
    "-Wall"
    "-Wextra"
    "-g"
    "-O0"
)

# Output executable
OUTPUT="working_parameter_lock_test"

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
    echo "🚀 Running working parameter lock tests..."
    ./"$OUTPUT"
else
    echo "❌ Compilation failed!"
    exit 1
fi
