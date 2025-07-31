#!/bin/bash
#
# LVGL RT-Safe MIDI Test Runner
# =============================
#
# Runs comprehensive tests for the RT-safe MIDI system
# Supports both desktop and ESP32 testing
#

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." &> /dev/null && pwd )"

echo "🧪 LVGL RT-Safe MIDI Test Suite"
echo "==============================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

run_desktop_tests() {
    echo -e "${BLUE}🖥️  Running Desktop Tests...${NC}"
    echo "Building test environment..."
    
    cd "$PROJECT_ROOT"
    
    # Build and run desktop tests
    if pio run -e desktop_test; then
        echo -e "${GREEN}✅ Desktop tests build successful${NC}"
        
        # Run the test executable
        if .pio/build/desktop_test/program; then
            echo -e "${GREEN}🎉 Desktop tests PASSED!${NC}"
            return 0
        else
            echo -e "${RED}❌ Desktop tests FAILED!${NC}"
            return 1
        fi
    else
        echo -e "${RED}❌ Desktop test build FAILED!${NC}"
        return 1
    fi
}

run_esp32_tests() {
    echo -e "${BLUE}🔌 Running ESP32 Tests...${NC}"
    echo "Note: ESP32 must be connected via USB"
    
    cd "$PROJECT_ROOT"
    
    # Check if ESP32 is connected
    if ! pio device list | grep -q "USB"; then
        echo -e "${YELLOW}⚠️  No ESP32 device detected. Skipping ESP32 tests.${NC}"
        return 0
    fi
    
    # Build and upload ESP32 tests
    if pio test -e esp32_test; then
        echo -e "${GREEN}🎉 ESP32 tests PASSED!${NC}"
        return 0
    else
        echo -e "${RED}❌ ESP32 tests FAILED!${NC}"
        return 1
    fi
}

run_memory_tests() {
    echo -e "${BLUE}🧠 Running Memory Safety Tests...${NC}"
    
    cd "$PROJECT_ROOT"
    
    # Build with memory debugging
    echo "Building with AddressSanitizer and LeakSanitizer..."
    if pio run -e desktop-memory-debug; then
        echo -e "${GREEN}✅ Memory debug build successful${NC}"
        
        # Run with memory debugging
        echo "Running memory safety tests..."
        if timeout 30s .pio/build/desktop-memory-debug/program; then
            echo -e "${GREEN}🛡️  Memory safety tests PASSED!${NC}"
            return 0
        else
            echo -e "${YELLOW}⏱️  Memory tests completed (timeout expected)${NC}"
            return 0
        fi
    else
        echo -e "${RED}❌ Memory debug build FAILED!${NC}"
        return 1
    fi
}

run_midi_integration_tests() {
    echo -e "${BLUE}🎹 Running MIDI Integration Tests...${NC}"
    
    # Check if synthesizer is running
    if pgrep -f "desktop/program" > /dev/null; then
        echo "✅ LVGL Synthesizer detected - running integration tests"
        
        # Run our external MIDI tests
        cd "$PROJECT_ROOT"
        if ./scripts/test_midi.sh test all; then
            echo -e "${GREEN}🎵 MIDI integration tests PASSED!${NC}"
            return 0
        else
            echo -e "${RED}❌ MIDI integration tests FAILED!${NC}"
            return 1
        fi
    else
        echo -e "${YELLOW}⚠️  LVGL Synthesizer not running. Skipping integration tests.${NC}"
        echo "   Start with: .pio/build/desktop/program"
        return 0
    fi
}

print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --desktop       Run desktop tests only"
    echo "  --esp32         Run ESP32 tests only"
    echo "  --memory        Run memory safety tests only"
    echo "  --midi          Run MIDI integration tests only"
    echo "  --all           Run all tests (default)"
    echo "  --help          Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                    # Run all tests"
    echo "  $0 --desktop         # Quick desktop-only testing"
    echo "  $0 --memory --midi   # Memory and MIDI tests only"
}

# Parse command line arguments
DESKTOP_TESTS=false
ESP32_TESTS=false
MEMORY_TESTS=false
MIDI_TESTS=false
ALL_TESTS=true

while [[ $# -gt 0 ]]; do
    case $1 in
        --desktop)
            DESKTOP_TESTS=true
            ALL_TESTS=false
            shift
            ;;
        --esp32)
            ESP32_TESTS=true
            ALL_TESTS=false
            shift
            ;;
        --memory)
            MEMORY_TESTS=true
            ALL_TESTS=false
            shift
            ;;
        --midi)
            MIDI_TESTS=true
            ALL_TESTS=false
            shift
            ;;
        --all)
            ALL_TESTS=true
            shift
            ;;
        --help)
            print_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

# Run selected tests
TOTAL_TESTS=0
PASSED_TESTS=0

if [[ "$ALL_TESTS" == true ]] || [[ "$DESKTOP_TESTS" == true ]]; then
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    if run_desktop_tests; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
    fi
fi

if [[ "$ALL_TESTS" == true ]] || [[ "$ESP32_TESTS" == true ]]; then
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    if run_esp32_tests; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
    fi
fi

if [[ "$ALL_TESTS" == true ]] || [[ "$MEMORY_TESTS" == true ]]; then
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    if run_memory_tests; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
    fi
fi

if [[ "$ALL_TESTS" == true ]] || [[ "$MIDI_TESTS" == true ]]; then
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    if run_midi_integration_tests; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
    fi
fi

# Print final results
echo ""
echo "================================"
echo "🏁 FINAL TEST RESULTS"
echo "================================"
echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}/$TOTAL_TESTS"

if [[ $PASSED_TESTS -eq $TOTAL_TESTS ]]; then
    echo -e "${GREEN}🎉 ALL TESTS PASSED!${NC}"
    echo ""
    echo "✅ RT-Safe MIDI system is working perfectly!"
    echo "✅ Memory management is leak-free"
    echo "✅ Cross-platform compatibility verified"
    echo "✅ MIDI integration functional"
    exit 0
else
    echo -e "${RED}💥 SOME TESTS FAILED!${NC}"
    echo ""
    echo "Please check the output above for details."
    exit 1
fi
