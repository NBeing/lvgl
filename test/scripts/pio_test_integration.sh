#!/bin/bash

# PlatformIO Unified Test Integration Script
# This script provides seamless integration between PlatformIO and the unified test framework

echo "🔗 PlatformIO + Unified Test Framework Integration"
echo "================================================="

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
TEST_DIR="$(dirname "$SCRIPT_DIR")"

# Function to show usage
show_usage() {
    echo "Usage: $0 [COMMAND] [OPTIONS]"
    echo ""
    echo "Commands:"
    echo "  test             Run unified tests only"
    echo "  build            Build project for testing"
    echo "  build-and-test   Build project and run tests (default)"
    echo "  simulator        Build and run desktop simulator"
    echo "  esp32            Build for ESP32"
    echo "  clean            Clean all build artifacts"
    echo ""
    echo "Options:"
    echo "  --env ENV        Specify PlatformIO environment (desktop, esp32, unified_tests)"
    echo "  --verbose        Enable verbose output"
    echo "  --coverage       Generate test coverage report"
    echo "  --help           Show this help"
}

# Function to run PlatformIO build
run_pio_build() {
    local env="$1"
    local verbose="$2"
    
    echo "🏗️  Building with PlatformIO (env: $env)"
    echo "========================================"
    
    cd "$PROJECT_ROOT"
    
    local pio_cmd="pio run -e $env"
    if [ "$verbose" = true ]; then
        pio_cmd="$pio_cmd -v"
    fi
    
    eval $pio_cmd
    
    if [ $? -eq 0 ]; then
        echo "✅ PlatformIO build successful"
        return 0
    else
        echo "❌ PlatformIO build failed"
        return 1
    fi
}

# Function to run unified tests
run_unified_tests() {
    local verbose="$1"
    
    echo ""
    echo "🧪 Running Unified Test Framework"
    echo "================================="
    
    cd "$TEST_DIR"
    
    if [ "$verbose" = true ]; then
        ./scripts/run_all_tests.sh --verbose
    else
        ./scripts/run_all_tests.sh
    fi
    
    return $?
}

# Function to run desktop simulator
run_simulator() {
    local env="$1"
    
    echo ""
    echo "🖥️  Starting Desktop Simulator"
    echo "=============================="
    
    cd "$PROJECT_ROOT"
    
    # Build first
    pio run -e $env
    
    if [ $? -eq 0 ]; then
        echo "🚀 Starting simulator..."
        ./.pio/build/$env/program
    else
        echo "❌ Failed to build simulator"
        return 1
    fi
}

# Function to generate coverage report
generate_coverage() {
    echo ""
    echo "📊 Generating Test Coverage Report"
    echo "=================================="
    
    cd "$TEST_DIR"
    
    # Check if coverage tools are available
    if ! command -v gcov &> /dev/null || ! command -v lcov &> /dev/null; then
        echo "❌ Coverage tools not available. Install gcov and lcov."
        return 1
    fi
    
    # Run tests with coverage
    echo "Running tests with coverage instrumentation..."
    
    # Modify compiler flags to include coverage
    export COVERAGE_FLAGS="-fprofile-arcs -ftest-coverage -O0 -g"
    
    # Run the tests
    ./scripts/run_all_tests.sh
    
    # Generate coverage report
    echo "Generating coverage report..."
    lcov --capture --directory . --output-file coverage.info
    lcov --remove coverage.info '/usr/*' '*/test/*' --output-file coverage_filtered.info
    genhtml coverage_filtered.info --output-directory coverage_html
    
    echo "✅ Coverage report generated in test/coverage_html/index.html"
    
    return 0
}

# Function to clean build artifacts
clean_all() {
    echo "🧹 Cleaning Build Artifacts"
    echo "==========================="
    
    cd "$PROJECT_ROOT"
    pio run -t clean
    
    cd "$TEST_DIR"
    rm -rf build/
    rm -rf coverage_html/
    rm -f *.info
    rm -f *.gcov
    rm -f *.gcda
    rm -f *.gcno
    
    echo "✅ Clean complete"
}

# Parse command line arguments
COMMAND="build-and-test"
ENV="desktop"
VERBOSE=false
COVERAGE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        test|build|build-and-test|simulator|esp32|clean)
            COMMAND="$1"
            shift
            ;;
        --env)
            ENV="$2"
            shift 2
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --coverage)
            COVERAGE=true
            shift
            ;;
        --help)
            show_usage
            exit 0
            ;;
        *)
            echo "❌ Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Execute command
case $COMMAND in
    "test")
        echo "Command: Run unified tests only"
        echo "Environment: $ENV"
        run_unified_tests $VERBOSE
        exit $?
        ;;
    "build")
        echo "Command: Build project for testing"
        echo "Environment: $ENV"
        run_pio_build $ENV $VERBOSE
        exit $?
        ;;
    "build-and-test")
        echo "Command: Build project and run tests"
        echo "Environment: $ENV"
        
        # Build first
        run_pio_build $ENV $VERBOSE
        build_result=$?
        
        if [ $build_result -eq 0 ]; then
            # Run tests
            run_unified_tests $VERBOSE
            test_result=$?
            
            # Generate coverage if requested
            if [ "$COVERAGE" = true ]; then
                generate_coverage
            fi
            
            # Final result
            if [ $test_result -eq 0 ]; then
                echo ""
                echo "🎉 BUILD AND TEST SUCCESS!"
                echo "========================="
                exit 0
            else
                echo ""
                echo "❌ Tests failed"
                exit 1
            fi
        else
            echo ""
            echo "❌ Build failed - skipping tests"
            exit 1
        fi
        ;;
    "simulator")
        echo "Command: Build and run desktop simulator"
        run_simulator $ENV
        exit $?
        ;;
    "esp32")
        echo "Command: Build for ESP32"
        run_pio_build "rymcu-esp32-s3-arduino3x" $VERBOSE
        exit $?
        ;;
    "clean")
        echo "Command: Clean all build artifacts"
        clean_all
        exit 0
        ;;
    *)
        echo "❌ Unknown command: $COMMAND"
        show_usage
        exit 1
        ;;
esac
