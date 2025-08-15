#!/bin/bash
# filepath: test/compile_midi_controller_tests.sh

set -e  # Exit on any error

echo "🎛️ Compiling MIDI Controller Tests..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

# Test files to compile
MIDI_CONTROLLER_TESTS=(
    "sequencer_midi_controller_test.cpp"
)

# Compilation flags
CXXFLAGS="-std=c++17 -g -O2 -Wall -Wextra -pthread"
INCLUDES="-I../src -I../lib/gtest/include -I../lib/gmock/include"
LIBS="-lgtest -lgmock -lpthread"
DEFINES="-DDESKTOP_BUILD -DUSE_THREADED_ARCHITECTURE=1 -DDEBUG_SEQUENCER_MIDI=1"

# Create build directory
BUILD_DIR="build/midi_controller_tests"
mkdir -p "$BUILD_DIR"

echo -e "${BLUE}📁 Build directory: $BUILD_DIR${NC}"

# Compile each test
for test_file in "${MIDI_CONTROLLER_TESTS[@]}"; do
    echo -e "${YELLOW}🔨 Compiling $test_file...${NC}"
    
    # Extract test name (remove .cpp extension)
    test_name="${test_file%.cpp}"
    executable="$BUILD_DIR/$test_name"
    
    # Compile with mock objects
    if g++ $CXXFLAGS $INCLUDES $DEFINES \
           "$test_file" \
           ../src/components/midi/SequencerMidiController.cpp \
           ../src/components/parameter/ParameterManager.cpp \
           ../src/components/parameter/MidiParameterBridge.cpp \
           ../src/components/threading/ThreadingAbstraction.cpp \
           -o "$executable" \
           $LIBS; then
        echo -e "${GREEN}✅ Successfully compiled $test_name${NC}"
    else
        echo -e "${RED}❌ Failed to compile $test_name${NC}"
        exit 1
    fi
done

echo -e "${GREEN}🎉 All MIDI controller tests compiled successfully!${NC}"

# Run tests if requested
if [[ "$1" == "--run" ]]; then
    echo -e "${BLUE}🚀 Running MIDI controller tests...${NC}"
    
    for test_file in "${MIDI_CONTROLLER_TESTS[@]}"; do
        test_name="${test_file%.cpp}"
        executable="$BUILD_DIR/$test_name"
        
        echo -e "${YELLOW}🧪 Running $test_name...${NC}"
        if "$executable"; then
            echo -e "${GREEN}✅ $test_name passed${NC}"
        else
            echo -e "${RED}❌ $test_name failed${NC}"
            exit 1
        fi
    done
    
    echo -e "${GREEN}🎉 All MIDI controller tests passed!${NC}"
fi

# Generate test report if requested
if [[ "$1" == "--report" ]]; then
    echo -e "${PURPLE}📊 Generating test report...${NC}"
    
    REPORT_FILE="$BUILD_DIR/midi_controller_test_report.txt"
    echo "MIDI Controller Test Report" > "$REPORT_FILE"
    echo "Generated: $(date)" >> "$REPORT_FILE"
    echo "=================================" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    
    for test_file in "${MIDI_CONTROLLER_TESTS[@]}"; do
        test_name="${test_file%.cpp}"
        executable="$BUILD_DIR/$test_name"
        
        echo "Running $test_name..." >> "$REPORT_FILE"
        if "$executable" --gtest_output=xml:"$BUILD_DIR/${test_name}_results.xml" >> "$REPORT_FILE" 2>&1; then
            echo "✅ $test_name: PASSED" >> "$REPORT_FILE"
        else
            echo "❌ $test_name: FAILED" >> "$REPORT_FILE"
        fi
        echo "" >> "$REPORT_FILE"
    done
    
    echo -e "${PURPLE}📋 Test report saved to: $REPORT_FILE${NC}"
fi

echo -e "${BLUE}📋 Test executables created in $BUILD_DIR/${NC}"
echo -e "${BLUE}💡 Usage:${NC}"
echo -e "${BLUE}   Run tests: $0 --run${NC}"
echo -e "${BLUE}   Generate report: $0 --report${NC}"
echo -e "${BLUE}   Manual execution: ./$BUILD_DIR/sequencer_midi_controller_test${NC}"
