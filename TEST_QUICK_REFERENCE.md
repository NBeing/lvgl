# 🎹 LVGL MIDI Framework - Quick Test Reference

## 🚀 **Easiest Commands (Recommended)**

```bash
cd test

# Run everything
./run_tests.sh          # All tests with beautiful output

# Run by category  
./run_tests.sh unit     # Unit tests only
./run_tests.sh integration  # Integration tests only
./run_tests.sh system   # System tests only
```

## 🎯 **Make Commands (Even Easier)**

```bash
cd test

make test               # Run all tests
make test-unit          # Unit tests
make test-integration   # Integration tests  
make test-system        # System tests

# Focused testing
make test-midi          # MIDI-related tests only
make test-ui            # UI integration tests only
make test-threading     # Thread safety tests only
```

## 🧹 **Maintenance**

```bash
./run_tests.sh clean    # Remove executables
make clean              # Same thing via make
```

## 📊 **What You'll See**

```
🎹 LVGL MIDI Framework Test Runner
=====================================

🎯 Running Unit Tests
================================
📝 Building: priority_queue_test
✅ Built: priority_queue_test
🚀 Running: priority_queue_test
✅ PASSED: priority_queue_test

📊 Unit Summary: 5/5 passed
✅ All Unit tests passed!
```

## 🎛️ **Test Categories Explained**

- **Unit Tests**: Individual components (MIDI parsing, queues, threading)
- **Integration Tests**: Component interactions (UI ↔ MIDI, parameter systems)  
- **System Tests**: Complete workflows (full MIDI device simulation)

## ⚡ **Quick Start**

```bash
cd test && make test
```

**That's it!** One command runs everything with beautiful colored output! 🎉
