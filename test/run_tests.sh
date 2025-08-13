#!/bin/bash

# LVGL MIDI Framework Test Runner
# Simple, user-friendly test execution system

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test directories
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UNIT_DIR="$TEST_DIR/unit"
INTEGRATION_DIR="$TEST_DIR/integration"
SYSTEM_DIR="$TEST_DIR/system"

# Compiler settings
CXX="g++"
CXXFLAGS="-std=c++17 -I$TEST_DIR -I$TEST_DIR/framework -I$TEST_DIR/fixtures -pthread -O2 -Wall -Wextra"

echo -e "${BLUE}🎹 LVGL MIDI Framework Test Runner${NC}"
echo "====================================="

# Function to build and run a single test
build_and_run_test() {
    local test_file="$1"
    local test_name=$(basename "$test_file" .cpp)
    local test_dir=$(dirname "$test_file")
    local executable="$test_dir/$test_name"
    
    echo -e "${YELLOW}📝 Building: $test_name${NC}"
    
    # Build the test
    if $CXX $CXXFLAGS "$test_file" -o "$executable"; then
        echo -e "${GREEN}✅ Built: $test_name${NC}"
        
        # Run the test
        echo -e "${YELLOW}🚀 Running: $test_name${NC}"
        if "$executable"; then
            echo -e "${GREEN}✅ PASSED: $test_name${NC}"
            return 0
        else
            echo -e "${RED}❌ FAILED: $test_name${NC}"
            return 1
        fi
    else
        echo -e "${RED}❌ BUILD FAILED: $test_name${NC}"
        return 1
    fi
}

# Function to run all tests in a category
run_category() {
    local category="$1"
    local test_dir="$2"
    
    echo -e "\n${BLUE}🎯 Running $category Tests${NC}"
    echo "================================"
    
    if [ ! -d "$test_dir" ]; then
        echo -e "${YELLOW}⚠️  No $category tests found in $test_dir${NC}"
        return 0
    fi
    
    local total=0
    local passed=0
    local failed=0
    
    # Find all .cpp test files
    while IFS= read -r -d '' test_file; do
        total=$((total + 1))
        if build_and_run_test "$test_file"; then
            passed=$((passed + 1))
        else
            failed=$((failed + 1))
        fi
        echo ""
    done < <(find "$test_dir" -name "*_test.cpp" -print0)
    
    # Summary for this category
    echo -e "${BLUE}📊 $category Summary: $passed/$total passed${NC}"
    if [ $failed -eq 0 ]; then
        echo -e "${GREEN}✅ All $category tests passed!${NC}"
    else
        echo -e "${RED}❌ $failed $category tests failed${NC}"
    fi
    
    return $failed
}

# Function to clean build artifacts
clean_tests() {
    echo -e "${YELLOW}🧹 Cleaning test executables...${NC}"
    find "$TEST_DIR" -name "*_test" -type f -executable -delete
    echo -e "${GREEN}✅ Cleaned test executables${NC}"
}

# Main execution logic
case "${1:-all}" in
    "unit")
        run_category "Unit" "$UNIT_DIR"
        exit $?
        ;;
    "integration")
        run_category "Integration" "$INTEGRATION_DIR"
        exit $?
        ;;
    "system")
        run_category "System" "$SYSTEM_DIR"
        exit $?
        ;;
    "clean")
        clean_tests
        exit 0
        ;;
    "help"|"-h"|"--help")
        echo "Usage: $0 [category]"
        echo ""
        echo "Categories:"
        echo "  unit         Run unit tests only"
        echo "  integration  Run integration tests only"
        echo "  system       Run system tests only"
        echo "  all          Run all tests (default)"
        echo "  clean        Clean test executables"
        echo "  help         Show this help"
        echo ""
        echo "Examples:"
        echo "  $0           # Run all tests"
        echo "  $0 unit      # Run only unit tests"
        echo "  $0 clean     # Clean executables"
        exit 0
        ;;
    "all"|"")
        echo -e "${BLUE}🚀 Running ALL Tests${NC}"
        echo "==================="
        
        total_failed=0
        
        # Run each category
        run_category "Unit" "$UNIT_DIR"
        total_failed=$((total_failed + $?))
        
        run_category "Integration" "$INTEGRATION_DIR"
        total_failed=$((total_failed + $?))
        
        run_category "System" "$SYSTEM_DIR"
        total_failed=$((total_failed + $?))
        
        # Overall summary
        echo ""
        echo -e "${BLUE}🎯 OVERALL TEST SUMMARY${NC}"
        echo "======================="
        if [ $total_failed -eq 0 ]; then
            echo -e "${GREEN}🎉 ALL TESTS PASSED! Framework is ready for MIDI device development!${NC}"
            exit 0
        else
            echo -e "${RED}❌ $total_failed test categories had failures${NC}"
            exit 1
        fi
        ;;
    *)
        echo -e "${RED}❌ Unknown option: $1${NC}"
        echo "Use '$0 help' for usage information"
        exit 1
        ;;
esac
