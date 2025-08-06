#!/bin/bash

# Simple Event Visualizer Test Compilation Script
# Only tests RT-safe core functionality without LVGL dependencies

echo "🎵 Compiling Simple Event Visualizer Test Suite..."

# Define source files (only test files, no LVGL dependencies)
SOURCES=(
    "run_event_visualizer_tests.cpp"
)

# Define include paths
INCLUDES=(
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
)

# Output executable
OUTPUT="simple_event_visualizer_test"

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
    echo "🚀 Running tests..."
    ./"$OUTPUT"
else
    echo "❌ Compilation failed!"
    exit 1
fi
