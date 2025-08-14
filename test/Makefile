# LVGL MIDI Framework Test Makefile - Enhanced Edition
# Features: Test filtering, comprehensive error reporting, and easy setup

.PHONY: test test-unit test-integration test-system clean help setup-ignore show-ignore

# Default target
test:
	@./run_tests.sh

# Individual test categories
test-unit:
	@./run_tests.sh unit

test-integration:
	@./run_tests.sh integration

test-system:
	@./run_tests.sh system

# Specific test targets for common tests
test-midi:
	@echo "🎹 Running MIDI tests..."
	@./run_tests.sh unit | grep -A 20 "rt_safe_midi_test\|priority_queue_test"

test-ui:
	@echo "🎛️ Running UI integration tests..."
	@./run_tests.sh integration | grep -A 20 "rt_safe_ui_control_test"

test-threading:
	@echo "🧵 Running thread safety tests..."
	@./run_tests.sh unit | grep -A 20 "thread_safety_test\|rt_safe_events_test"

# Build individual tests without running
build-unit:
	@echo "🔨 Building unit tests..."
	@find unit -name "*_test.cpp" -exec g++ -std=c++17 -I. -Iframework -Ifixtures -pthread -O2 {} -o {:.cpp} \;

build-integration:
	@echo "🔨 Building integration tests..."
	@find integration -name "*_test.cpp" -exec g++ -std=c++17 -I. -Iframework -Ifixtures -pthread -O2 {} -o {:.cpp} \;

build-system:
	@echo "🔨 Building system tests..."
	@find system -name "*_test.cpp" -exec g++ -std=c++17 -I. -Iframework -Ifixtures -pthread -O2 {} -o {:.cpp} \;

# Clean build artifacts
clean:
	@./run_tests.sh clean

# Test filtering and configuration
setup-ignore:
	@./run_tests.sh ignore-setup

show-ignore:
	@echo "Current test ignore patterns:"
	@if [ -f .test_ignore ]; then \
		grep -v '^#' .test_ignore | grep -v '^[[:space:]]*$$' | sed 's/^/  • /'; \
	else \
		echo "  No ignore configuration found. Run 'make setup-ignore' to create one."; \
	fi

# Help with enhanced features
help:
	@echo "LVGL MIDI Framework Test Commands - Enhanced Edition"
	@echo "===================================================="
	@echo ""
	@echo "Main Commands:"
	@echo "  make test              Run all tests with comprehensive error reporting"
	@echo "  make test-unit         Run unit tests only"
	@echo "  make test-integration  Run integration tests only"
	@echo "  make test-system       Run system tests only"
	@echo ""
	@echo "Focused Tests:"
	@echo "  make test-midi         Run MIDI-related tests"
	@echo "  make test-ui           Run UI integration tests"
	@echo "  make test-threading    Run threading/concurrency tests"
	@echo ""
	@echo "Test Management:"
	@echo "  make setup-ignore      Set up test ignore patterns (for WIP tests)"
	@echo "  make show-ignore       Show current ignore patterns"
	@echo "  make clean             Clean test executables"
	@echo ""
	@echo "Enhanced Features:"
	@echo "  • Automatic test filtering via .test_ignore file"
	@echo "  • Comprehensive error traces for build and runtime failures"
	@echo "  • Support for regex patterns and folder exclusions"
	@echo "  • WIP (Work in Progress) test management"
	@echo ""
	@echo "Examples:"
	@echo "  make test              # Run all tests with full error reporting"
	@echo "  make setup-ignore      # Configure tests to skip (like event_visualizer.*)"
	@echo "  make show-ignore       # See what tests are being skipped"
	@echo "  make help              # Show this help"
