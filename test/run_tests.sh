#!/bin/bash

# LVGL MIDI Framework Test Runner - Enhanced Edition
# Features: Test filtering, comprehensive error reporting, and WIP test handling

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Test directories
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UNIT_DIR="$TEST_DIR/unit"
INTEGRATION_DIR="$TEST_DIR/integration"
SYSTEM_DIR="$TEST_DIR/system"

# Compiler settings
CXX="g++"
CXXFLAGS="-std=c++17 -I$TEST_DIR -I$TEST_DIR/framework -I$TEST_DIR/fixtures -pthread -O2 -Wall -Wextra"

# Error tracking arrays
declare -a BUILD_ERRORS=()
declare -a RUNTIME_ERRORS=()
declare -a SKIPPED_TESTS=()

# Configuration files for ignoring tests
IGNORE_CONFIG="$TEST_DIR/.test_ignore"

echo -e "${BLUE}🎹 LVGL MIDI Framework Test Runner - Enhanced${NC}"
echo "=============================================="

# Function to load ignore patterns
load_ignore_patterns() {
    declare -g -a IGNORE_PATTERNS=()
    
    if [ -f "$IGNORE_CONFIG" ]; then
        echo -e "${CYAN}📋 Loading test ignore patterns...${NC}"
        while IFS= read -r line; do
            # Skip empty lines and comments
            if [[ ! "$line" =~ ^[[:space:]]*$ ]] && [[ ! "$line" =~ ^[[:space:]]*# ]]; then
                IGNORE_PATTERNS+=("$line")
                echo -e "${CYAN}   🚫 Ignoring: $line${NC}"
            fi
        done < "$IGNORE_CONFIG"
        echo ""
    fi
}

# Function to check if test should be ignored
should_ignore_test() {
    local test_file="$1"
    local test_name=$(basename "$test_file" .cpp)
    local relative_path=$(realpath --relative-to="$TEST_DIR" "$test_file")
    
    for pattern in "${IGNORE_PATTERNS[@]}"; do
        # Check filename match
        if [[ "$test_name" =~ $pattern ]]; then
            return 0
        fi
        # Check relative path match
        if [[ "$relative_path" =~ $pattern ]]; then
            return 0
        fi
        # Check folder match
        if [[ "$(dirname "$relative_path")" =~ $pattern ]]; then
            return 0
        fi
    done
    return 1
}

# Function to build and run a single test with enhanced error tracking
build_and_run_test() {
    local test_file="$1"
    local test_name=$(basename "$test_file" .cpp)
    local test_dir=$(dirname "$test_file")
    local executable="$test_dir/$test_name"
    local relative_path=$(realpath --relative-to="$TEST_DIR" "$test_file")
    
    # Check if test should be ignored
    if should_ignore_test "$test_file"; then
        echo -e "${PURPLE}🚫 SKIPPED: $test_name (matched ignore pattern)${NC}"
        SKIPPED_TESTS+=("$test_name: Matched ignore pattern")
        return 0
    fi
    
    echo -e "${YELLOW}📝 Building: $test_name${NC}"
    
    # Capture build output for error tracking
    local build_output
    local build_errors
    
    if build_output=$($CXX $CXXFLAGS "$test_file" -o "$executable" 2>&1); then
        echo -e "${GREEN}✅ Built: $test_name${NC}"
        
        # Show warnings if any
        if [[ -n "$build_output" ]]; then
            echo -e "${YELLOW}⚠️  Build warnings:${NC}"
            echo "$build_output" | grep -E "(warning|note):" | head -5
        fi
        
        # Run the test with error capture
        echo -e "${YELLOW}🚀 Running: $test_name${NC}"
        local runtime_output
        local runtime_exit_code
        
        if runtime_output=$("$executable" 2>&1); then
            runtime_exit_code=$?
        else
            runtime_exit_code=$?
        fi
        
        if [ $runtime_exit_code -eq 0 ]; then
            echo -e "${GREEN}✅ PASSED: $test_name${NC}"
            return 0
        else
            echo -e "${RED}❌ FAILED: $test_name (exit code: $runtime_exit_code)${NC}"
            RUNTIME_ERRORS+=("$test_name|$runtime_exit_code|$runtime_output")
            return 1
        fi
    else
        build_errors="$build_output"
        echo -e "${RED}❌ BUILD FAILED: $test_name${NC}"
        BUILD_ERRORS+=("$test_name|$relative_path|$build_errors")
        return 1
    fi
}

# Function to display comprehensive error report
show_error_report() {
    local total_errors=$((${#BUILD_ERRORS[@]} + ${#RUNTIME_ERRORS[@]}))
    
    if [ $total_errors -gt 0 ] || [ ${#SKIPPED_TESTS[@]} -gt 0 ]; then
        echo ""
        echo -e "${RED}🚨 COMPREHENSIVE ERROR REPORT${NC}"
        echo "=============================="
        echo ""
    fi
    
    # Show skipped tests
    if [ ${#SKIPPED_TESTS[@]} -gt 0 ]; then
        echo -e "${PURPLE}🚫 SKIPPED TESTS (${#SKIPPED_TESTS[@]})${NC}"
        echo "---------------"
        for skipped in "${SKIPPED_TESTS[@]}"; do
            echo -e "${PURPLE}   • $skipped${NC}"
        done
        echo ""
    fi
    
    # Show build errors with full traces
    if [ ${#BUILD_ERRORS[@]} -gt 0 ]; then
        echo -e "${RED}🔨 BUILD ERRORS (${#BUILD_ERRORS[@]})${NC}"
        echo "-------------"
        local error_count=1
        for error in "${BUILD_ERRORS[@]}"; do
            IFS='|' read -r test_name file_path error_output <<< "$error"
            echo -e "${RED}$error_count. $test_name${NC}"
            echo -e "${CYAN}   File: $file_path${NC}"
            echo -e "${YELLOW}   Error details:${NC}"
            echo "$error_output" | head -10 | sed 's/^/      /'
            if [ $(echo "$error_output" | wc -l) -gt 10 ]; then
                echo -e "${YELLOW}      ... (truncated, first 10 lines shown)${NC}"
            fi
            echo ""
            ((error_count++))
        done
    fi
    
    # Show runtime errors with traces
    if [ ${#RUNTIME_ERRORS[@]} -gt 0 ]; then
        echo -e "${RED}💥 RUNTIME ERRORS (${#RUNTIME_ERRORS[@]})${NC}"
        echo "---------------"
        local error_count=1
        for error in "${RUNTIME_ERRORS[@]}"; do
            IFS='|' read -r test_name exit_code error_output <<< "$error"
            echo -e "${RED}$error_count. $test_name (exit code: $exit_code)${NC}"
            echo -e "${YELLOW}   Runtime output:${NC}"
            if [[ -n "$error_output" ]]; then
                echo "$error_output" | tail -15 | sed 's/^/      /'
                if [ $(echo "$error_output" | wc -l) -gt 15 ]; then
                    echo -e "${YELLOW}      ... (truncated, last 15 lines shown)${NC}"
                fi
            else
                echo -e "${YELLOW}      (No output captured)${NC}"
            fi
            echo ""
            ((error_count++))
        done
    fi
    
    # Summary statistics
    if [ $total_errors -gt 0 ] || [ ${#SKIPPED_TESTS[@]} -gt 0 ]; then
        echo -e "${BLUE}📊 ERROR SUMMARY${NC}"
        echo "=============="
        echo -e "${RED}   Build Errors: ${#BUILD_ERRORS[@]}${NC}"
        echo -e "${RED}   Runtime Errors: ${#RUNTIME_ERRORS[@]}${NC}"
        echo -e "${PURPLE}   Skipped Tests: ${#SKIPPED_TESTS[@]}${NC}"
        echo -e "${YELLOW}   Total Issues: $((total_errors + ${#SKIPPED_TESTS[@]}))${NC}"
        echo ""
    fi
}

# Function to set up ignore configuration
setup_ignore_config() {
    echo -e "${BLUE}🔧 Setting up test ignore configuration${NC}"
    echo "======================================"
    
    if [ -f "$IGNORE_CONFIG" ]; then
        echo -e "${YELLOW}Existing ignore configuration found:${NC}"
        cat -n "$IGNORE_CONFIG"
        echo ""
        read -p "Do you want to edit it? (y/n): " edit_existing
        if [[ ! "$edit_existing" =~ ^[Yy] ]]; then
            return 0
        fi
    else
        echo -e "${CYAN}Creating new test ignore configuration...${NC}"
        cat > "$IGNORE_CONFIG" << 'EOF'
# Test Ignore Configuration
# ========================
# This file contains patterns for tests to ignore during execution.
# Supports regex patterns for filenames, paths, and folders.
#
# Examples:
# event_visualizer.*        # Ignore all event visualizer tests
# integration/wip_.*        # Ignore WIP tests in integration folder
# .*_broken.*               # Ignore any test with 'broken' in name
# system/                   # Ignore entire system folder
# midi_system_test          # Ignore specific test by exact name
#
# Lines starting with # are comments and will be ignored.
# Empty lines are also ignored.

# WIP Tests (Work in Progress)
event_visualizer.*

# Broken/Incomplete Tests
.*_broken.*
.*_incomplete.*

# Platform-specific tests that may not work everywhere
# platform_specific_.*

EOF
    fi
    
    echo -e "${GREEN}✅ Test ignore configuration ready at: $IGNORE_CONFIG${NC}"
    echo ""
    echo -e "${CYAN}Current patterns:${NC}"
    grep -v '^#' "$IGNORE_CONFIG" | grep -v '^[[:space:]]*$' | sed 's/^/  • /'
    echo ""
    echo -e "${YELLOW}You can edit this file manually or run this command again.${NC}"
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

# Main execution logic with enhanced error reporting
load_ignore_patterns

case "${1:-all}" in
    "unit")
        run_category "Unit" "$UNIT_DIR"
        failed_tests=$?
        show_error_report
        exit $failed_tests
        ;;
    "integration")
        run_category "Integration" "$INTEGRATION_DIR"
        failed_tests=$?
        show_error_report
        exit $failed_tests
        ;;
    "system")
        run_category "System" "$SYSTEM_DIR"
        failed_tests=$?
        show_error_report
        exit $failed_tests
        ;;
    "clean")
        clean_tests
        exit 0
        ;;
    "ignore-setup")
        setup_ignore_config
        exit 0
        ;;
    "help"|"-h"|"--help")
        echo "Usage: $0 [category] [options]"
        echo ""
        echo "Categories:"
        echo "  unit           Run unit tests only"
        echo "  integration    Run integration tests only"
        echo "  system         Run system tests only"
        echo "  all            Run all tests (default)"
        echo "  clean          Clean test executables"
        echo "  ignore-setup   Create/edit test ignore configuration"
        echo "  help           Show this help"
        echo ""
        echo "Features:"
        echo "  • Test filtering via .test_ignore file"
        echo "  • Comprehensive error reporting with traces"
        echo "  • Build and runtime error tracking"
        echo "  • Support for regex patterns and folder exclusions"
        echo ""
        echo "Ignore Patterns (.test_ignore):"
        echo "  event_visualizer.*    # Ignore all event visualizer tests"
        echo "  integration/wip_.*    # Ignore WIP tests in integration folder"
        echo "  .*_broken.*           # Ignore any test with 'broken' in name"
        echo "  system/               # Ignore entire system folder"
        echo ""
        echo "Examples:"
        echo "  $0                    # Run all tests with error reporting"
        echo "  $0 unit              # Run only unit tests"
        echo "  $0 ignore-setup      # Set up test filtering"
        echo "  $0 clean             # Clean executables"
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
        
        # Show comprehensive error report
        show_error_report
        
        # Overall summary
        echo ""
        echo -e "${BLUE}🎯 OVERALL TEST SUMMARY${NC}"
        echo "======================="
        if [ $total_failed -eq 0 ]; then
            echo -e "${GREEN}🎉 ALL TESTS PASSED! Framework is ready for MIDI device development!${NC}"
            if [ ${#SKIPPED_TESTS[@]} -gt 0 ]; then
                echo -e "${PURPLE}   (${#SKIPPED_TESTS[@]} tests were skipped)${NC}"
            fi
            exit 0
        else
            echo -e "${RED}❌ $total_failed test categories had failures${NC}"
            echo -e "${YELLOW}   See error report above for detailed traces${NC}"
            exit 1
        fi
        ;;
    *)
        echo -e "${RED}❌ Unknown option: $1${NC}"
        echo "Use '$0 help' for usage information"
        exit 1
        ;;
esac
