#!/bin/bash

# Event Visualizer Test Compilation Script
# Includes all necessary source files for the RTEventTracer system

echo "🎵 Compiling Event Visualizer Test Suite..."

# Define source files needed for RTEventTracer
SOURCES=(
    "run_event_visualizer_tests.cpp"
    "../src/components/debug/RTEventTracer.cpp"
    "../src/components/debug/EventVisualizerIntegration.cpp"
    "../src/components/debug/EventFlowVisualizer.cpp"
    "../src/components/debug/ExampleEventVisualizerApp.cpp"
)

# Define include paths
INCLUDES=(
    "-I../include"
    "-I../src"
    "-I../src/components"
    "-I../src/components/debug"
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
OUTPUT="event_visualizer_test"

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
