#!/bin/bash

# Unified Test Runner - Automated Testing for All Components
# Replaces all the fragmented compilation scripts with one unified system

echo "🧪 LVGL Synthesizer - Unified Test Suite"
echo "========================================"

# Configuration
BUILD_DIR="build"
INCLUDE_PATHS="-I. -I../include -I../src -I../src/components -I../src/components/midi -I../src/components/parameter -I../src/components/debug"
COMPILER_FLAGS="-std=c++17 -Wall -Wextra -g -O0 -DDESKTOP_BUILD=1 -DDISABLE_EVENT_TRACER=1"

# Create build directory
mkdir -p "$BUILD_DIR"

# Function to compile and run a test
run_test() {
    local test_file="$1"
    local test_name="$2"
    local category="$3"
    
    echo ""
    echo "🔨 Building $test_name..."
    
    local executable="$BUILD_DIR/${test_name}"
    
    # Compile the test
    g++ $COMPILER_FLAGS $INCLUDE_PATHS "$test_file" -o "$executable"
    
    if [ $? -eq 0 ]; then
        echo "✅ Build successful"
        echo "🚀 Running $test_name..."
        echo "----------------------------------------"
        
        # Run the test
        ./"$executable"
        local test_result=$?
        
        echo "----------------------------------------"
        if [ $test_result -eq 0 ]; then
            echo "✅ $test_name PASSED"
            return 0
        else
            echo "❌ $test_name FAILED"
            return 1
        fi
    else
        echo "❌ Build failed for $test_name"
        return 1
    fi
}

# Function to run all tests in a category
run_category() {
    local category="$1"
    local category_display="$2"
    
    echo ""
    echo "📁 Running $category_display Tests"
    echo "=================================="
    
    local passed=0
    local total=0
    local failed_tests=()
    
    # Find all test files in category
    if [ -d "$category" ]; then
        for test_file in "$category"/*.cpp; do
            if [ -f "$test_file" ]; then
                local test_name=$(basename "$test_file" .cpp)
                total=$((total + 1))
                
                if run_test "$test_file" "$test_name" "$category"; then
                    passed=$((passed + 1))
                else
                    failed_tests+=("$category/$test_name")
                fi
            fi
        done
    fi
    
    echo ""
    echo "📊 $category_display Results: $passed/$total passed"
    
    if [ ${#failed_tests[@]} -gt 0 ]; then
        echo "❌ Failed tests:"
        for failed in "${failed_tests[@]}"; do
            echo "  - $failed"
        done
    fi
    
    return $((total - passed))
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTION]"
    echo ""
    echo "Options:"
    echo "  --all              Run all tests (default)"
    echo "  --unit             Run only unit tests"
    echo "  --integration      Run only integration tests"
    echo "  --system           Run only system tests"
    echo "  --examples         Run example tests"
    echo "  --clean            Clean build directory"
    echo "  --list             List available tests"
    echo "  --help             Show this help"
}

# Function to list all available tests
list_tests() {
    echo "📋 Available Tests"
    echo "=================="
    
    for category in unit integration system examples; do
        if [ -d "$category" ]; then
            echo ""
            echo "📁 $category/"
            for test_file in "$category"/*.cpp; do
                if [ -f "$test_file" ]; then
                    local test_name=$(basename "$test_file" .cpp)
                    echo "  🧪 $test_name"
                fi
            done
        fi
    done
}

# Function to clean build directory
clean_build() {
    echo "🧹 Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    echo "✅ Build directory cleaned"
}

# Parse command line arguments
case "${1:-}" in
    --unit)
        run_category "unit" "Unit"
        exit $?
        ;;
    --integration)
        run_category "integration" "Integration"
        exit $?
        ;;
    --system)
        run_category "system" "System"
        exit $?
        ;;
    --examples)
        run_category "examples" "Example"
        exit $?
        ;;
    --clean)
        clean_build
        exit 0
        ;;
    --list)
        list_tests
        exit 0
        ;;
    --help)
        show_usage
        exit 0
        ;;
    --all|"")
        # Default: run all tests
        ;;
    *)
        echo "❌ Unknown option: $1"
        show_usage
        exit 1
        ;;
esac

# Main execution: Run all test categories
echo "🚀 Running Complete Test Suite..."

total_failed=0

# Run each category
run_category "unit" "Unit"
total_failed=$((total_failed + $?))

run_category "integration" "Integration" 
total_failed=$((total_failed + $?))

run_category "system" "System"
total_failed=$((total_failed + $?))

run_category "examples" "Example"
total_failed=$((total_failed + $?))

# Final summary
echo ""
echo "🎯 Final Test Summary"
echo "===================="

if [ $total_failed -eq 0 ]; then
    echo "🎉 ALL TESTS PASSED!"
    echo "✅ Complete test suite successful"
    echo ""
    echo "🏆 Your codebase is working excellently!"
    exit 0
else
    echo "❌ $total_failed test(s) failed"
    echo "🔧 Please review the failed tests above"
    exit 1
fi
