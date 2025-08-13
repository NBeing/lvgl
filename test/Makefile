# LVGL MIDI Framework Test Makefile
# Simple commands for building and running tests

.PHONY: test test-unit test-integration test-system clean help

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

# Help
help:
	@echo "LVGL MIDI Framework Test Commands"
	@echo "================================="
	@echo ""
	@echo "Main Commands:"
	@echo "  make test              Run all tests"
	@echo "  make test-unit         Run unit tests only"
	@echo "  make test-integration  Run integration tests only"
	@echo "  make test-system       Run system tests only"
	@echo ""
	@echo "Focused Tests:"
	@echo "  make test-midi         Run MIDI-related tests"
	@echo "  make test-ui           Run UI integration tests"
	@echo "  make test-threading    Run threading/concurrency tests"
	@echo ""
	@echo "Build Only:"
	@echo "  make build-unit        Build unit tests without running"
	@echo "  make build-integration Build integration tests without running"
	@echo "  make build-system      Build system tests without running"
	@echo ""
	@echo "Maintenance:"
	@echo "  make clean             Clean test executables"
	@echo "  make help              Show this help"
