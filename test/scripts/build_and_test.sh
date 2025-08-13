#!/bin/bash

# Unified Test Runner Integration for PlatformIO
# This script integrates the unified test framework with your existing build system

echo "🚀 LVGL Synthesizer - Complete Build & Test Integration"
echo "======================================================"

# Configuration
PROJECT_ROOT="../"
TEST_DIR="./test"
BUILD_DIR="./test/build"
PLATFORMIO_ENV="${1:-desktop}"

# Function to run platform-specific build
run_platform_build() {
    local platform="$1"
    echo ""
    echo "🏗️  Building Platform: $platform"
    echo "================================"
    
    cd "$PROJECT_ROOT"
    
    case "$platform" in
        "desktop"|"simulator")
            echo "🖥️  Building desktop simulator..."
            pio run -e desktop
            if [ $? -eq 0 ]; then
                echo "✅ Desktop build successful"
                return 0
            else
                echo "❌ Desktop build failed"
                return 1
            fi
            ;;
        "esp32")
            echo "📡 Building ESP32 firmware..."
            pio run -e rymcu-esp32-s3-arduino3x
            if [ $? -eq 0 ]; then
                echo "✅ ESP32 build successful"
                return 0
            else
                echo "❌ ESP32 build failed"
                return 1
            fi
            ;;
        "all")
            echo "🔄 Building all platforms..."
            pio run -e desktop && pio run -e rymcu-esp32-s3-arduino3x
            if [ $? -eq 0 ]; then
                echo "✅ All platform builds successful"
                return 0
            else
                echo "❌ Some platform builds failed"
                return 1
            fi
            ;;
        *)
            echo "❌ Unknown platform: $platform"
            return 1
            ;;
    esac
}

# Function to run unified tests
run_unified_tests() {
    echo ""
    echo "🧪 Running Unified Test Suite"
    echo "============================="
    
    cd "$TEST_DIR"
    
    # Run the unified test runner
    ./scripts/run_all_tests.sh
    local test_result=$?
    
    if [ $test_result -eq 0 ]; then
        echo "✅ All tests passed!"
        return 0
    else
        echo "❌ Some tests failed"
        return 1
    fi
}

# Function to generate comprehensive report
generate_report() {
    local build_result="$1"
    local test_result="$2"
    
    echo ""
    echo "📊 Complete Build & Test Report"
    echo "==============================="
    
    if [ $build_result -eq 0 ]; then
        echo "✅ Build Status: SUCCESS"
    else
        echo "❌ Build Status: FAILED"
    fi
    
    if [ $test_result -eq 0 ]; then
        echo "✅ Test Status: SUCCESS"
    else
        echo "❌ Test Status: FAILED"
    fi
    
    echo ""
    if [ $build_result -eq 0 ] && [ $test_result -eq 0 ]; then
        echo "🎉 COMPLETE SUCCESS - Ready for deployment!"
        return 0
    else
        echo "🔧 Issues found - Please review above for details"
        return 1
    fi
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [PLATFORM] [OPTIONS]"
    echo ""
    echo "Platforms:"
    echo "  desktop    Build desktop simulator (default)"
    echo "  esp32      Build ESP32 firmware"
    echo "  all        Build all platforms"
    echo ""
    echo "Options:"
    echo "  --tests-only     Run only tests (skip build)"
    echo "  --build-only     Run only build (skip tests)"
    echo "  --upload         Upload to ESP32 after build"
    echo "  --monitor        Monitor ESP32 after upload"
    echo "  --help           Show this help"
}

# Parse command line arguments
TESTS_ONLY=false
BUILD_ONLY=false
UPLOAD=false
MONITOR=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --tests-only)
            TESTS_ONLY=true
            shift
            ;;
        --build-only)
            BUILD_ONLY=true
            shift
            ;;
        --upload)
            UPLOAD=true
            shift
            ;;
        --monitor)
            MONITOR=true
            shift
            ;;
        --help)
            show_usage
            exit 0
            ;;
        desktop|esp32|all)
            PLATFORMIO_ENV="$1"
            shift
            ;;
        *)
            echo "❌ Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Main execution
echo "Platform: $PLATFORMIO_ENV"
echo "Tests only: $TESTS_ONLY"
echo "Build only: $BUILD_ONLY"

# Initialize results
build_result=0
test_result=0

# Run build phase
if [ "$TESTS_ONLY" = false ]; then
    run_platform_build "$PLATFORMIO_ENV"
    build_result=$?
    
    # Handle ESP32 upload
    if [ "$UPLOAD" = true ] && [ "$PLATFORMIO_ENV" = "esp32" ] && [ $build_result -eq 0 ]; then
        echo ""
        echo "📤 Uploading to ESP32..."
        cd "$PROJECT_ROOT"
        pio run -e rymcu-esp32-s3-arduino3x -t upload
        
        if [ "$MONITOR" = true ]; then
            echo "📺 Starting monitor..."
            pio device monitor -e rymcu-esp32-s3-arduino3x
        fi
    fi
fi

# Run test phase
if [ "$BUILD_ONLY" = false ]; then
    run_unified_tests
    test_result=$?
fi

# Generate final report
generate_report $build_result $test_result
final_result=$?

echo ""
echo "🏁 Integration Complete"
echo "======================"

exit $final_result
