#!/bin/bash

# Test Framework Cleanup Script
# Removes legacy test files that have been successfully migrated to unified framework

echo "🧹 Cleaning up legacy test files..."
echo "======================================="

# Files to remove (successfully migrated)
LEGACY_FILES=(
    "test/test_complete_system_demo.cpp"
    "test/test_bidirectional_bridge.cpp" 
    "test/test_event_visualizer.cpp"
    "test/test_ui_control_integration.cpp"
    "test/priority_queue_test.cpp"
    "test/thread_safety_test.cpp"
    "test/test_rt_safe_events.cpp"
    "test/test_parameter_lock_system.cpp"
    "test/test_parameter_manager.cpp"
    "test/test_main.cpp"
    "test/test_simple_bridge_demo.cpp"
    "test/test_simple_parameter_lock.cpp"
    "test/unit/midi_system_test_fixed.cpp"
    "test/unit/event_tracer_test_fixed.cpp"
)

# Create backup directory
mkdir -p backup/legacy_tests
echo "📁 Created backup directory: backup/legacy_tests"

# Move files to backup (safer than deletion)
for file in "${LEGACY_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "📦 Backing up: $file"
        cp "$file" "backup/legacy_tests/$(basename $file)"
        rm "$file"
        echo "✅ Removed: $file"
    else
        echo "⚠️  Not found: $file"
    fi
done

echo ""
echo "🎉 Cleanup completed!"
echo "✅ Legacy test files backed up to: backup/legacy_tests"
echo "✅ Workspace cleaned - only unified framework tests remain"
echo ""
echo "📊 Current test structure:"
echo "  test/unit/           - Unit tests"
echo "  test/integration/    - Integration tests" 
echo "  test/system/         - System-level tests"
echo "  test/framework/      - Unified test framework"
