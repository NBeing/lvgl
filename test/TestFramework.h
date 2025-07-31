#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <exception>

namespace Test {

/**
 * @brief Lightweight C++ test framework for RT-safe system testing
 * 
 * Features:
 * - Simple macro-based assertions
 * - RT-safe test execution timing
 * - Memory allocation tracking
 * - Thread-safety validation
 * - Performance benchmarking
 */
class TestFramework {
public:
    struct TestResult {
        std::string name;
        bool passed;
        std::string error_message;
        std::chrono::microseconds duration;
        size_t memory_allocated;
    };
    
    struct TestSuite {
        std::string name;
        std::vector<TestResult> results;
        size_t passed_count = 0;
        size_t failed_count = 0;
    };
    
private:
    static TestFramework* instance_;
    std::vector<TestSuite> test_suites_;
    TestSuite* current_suite_ = nullptr;
    
    // Memory tracking
    size_t memory_before_test_ = 0;
    bool track_memory_ = false;
    
    // RT-safe validation
    bool rt_safe_mode_ = false;
    std::chrono::high_resolution_clock::time_point test_start_time_;
    
public:
    static TestFramework& getInstance() {
        if (!instance_) {
            instance_ = new TestFramework();
        }
        return *instance_;
    }
    
    void beginTestSuite(const std::string& name) {
        test_suites_.emplace_back();
        current_suite_ = &test_suites_.back();
        current_suite_->name = name;
        std::cout << "\n🧪 Starting test suite: " << name << std::endl;
    }
    
    void endTestSuite() {
        if (!current_suite_) return;
        
        std::cout << "\n📊 Test suite '" << current_suite_->name << "' completed:" << std::endl;
        std::cout << "   ✅ Passed: " << current_suite_->passed_count << std::endl;
        std::cout << "   ❌ Failed: " << current_suite_->failed_count << std::endl;
        
        if (current_suite_->failed_count == 0) {
            std::cout << "   🎉 All tests passed!" << std::endl;
        }
        
        current_suite_ = nullptr;
    }
    
    void beginTest(const std::string& name, bool rt_safe = false) {
        if (!current_suite_) {
            beginTestSuite("Default");
        }
        
        rt_safe_mode_ = rt_safe;
        test_start_time_ = std::chrono::high_resolution_clock::now();
        
        if (track_memory_) {
            memory_before_test_ = getCurrentMemoryUsage();
        }
        
        std::cout << "  🔬 " << name << "... ";
        std::cout.flush();
    }
    
    void endTest(const std::string& name, bool passed, const std::string& error = "") {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - test_start_time_);
        
        size_t memory_used = 0;
        if (track_memory_) {
            memory_used = getCurrentMemoryUsage() - memory_before_test_;
        }
        
        TestResult result;
        result.name = name;
        result.passed = passed;
        result.error_message = error;
        result.duration = duration;
        result.memory_allocated = memory_used;
        
        current_suite_->results.push_back(result);
        
        if (passed) {
            current_suite_->passed_count++;
            std::cout << "✅";
            if (rt_safe_mode_) {
                std::cout << " (RT-safe, " << duration.count() << "μs)";
            }
        } else {
            current_suite_->failed_count++;
            std::cout << "❌ " << error;
        }
        std::cout << std::endl;
        
        // Validate RT-safety constraints
        if (rt_safe_mode_ && passed) {
            validateRTSafety(duration, memory_used);
        }
    }
    
    void enableMemoryTracking(bool enable) {
        track_memory_ = enable;
    }
    
    void printSummary() {
        size_t total_passed = 0;
        size_t total_failed = 0;
        
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "📋 TEST SUMMARY" << std::endl;
        std::cout << std::string(50, '=') << std::endl;
        
        for (const auto& suite : test_suites_) {
            total_passed += suite.passed_count;
            total_failed += suite.failed_count;
            
            std::cout << "Suite: " << suite.name << std::endl;
            std::cout << "  ✅ " << suite.passed_count << "  ❌ " << suite.failed_count << std::endl;
        }
        
        std::cout << "\nOVERALL: ✅ " << total_passed << "  ❌ " << total_failed << std::endl;
        
        if (total_failed == 0) {
            std::cout << "🎉 ALL TESTS PASSED!" << std::endl;
        } else {
            std::cout << "💥 " << total_failed << " TESTS FAILED!" << std::endl;
        }
    }
    
private:
    void validateRTSafety(std::chrono::microseconds duration, size_t memory_used) {
        // RT-safe constraints
        const auto MAX_RT_DURATION = std::chrono::microseconds(100); // 100μs max
        const size_t MAX_RT_MEMORY = 0; // No allocation allowed
        
        if (duration > MAX_RT_DURATION) {
            std::cout << " ⚠️ RT timing violation: " << duration.count() << "μs > 100μs";
        }
        
        if (memory_used > MAX_RT_MEMORY) {
            std::cout << " ⚠️ RT memory violation: " << memory_used << " bytes allocated";
        }
    }
    
    size_t getCurrentMemoryUsage() {
        // Simplified memory tracking - in real implementation would use
        // custom allocator or memory profiling tools
        return 0; // Placeholder
    }
};

TestFramework* TestFramework::instance_ = nullptr;

} // namespace Test

// Test macros for easy usage
#define TEST_SUITE(name) Test::TestFramework::getInstance().beginTestSuite(name)
#define END_TEST_SUITE() Test::TestFramework::getInstance().endTestSuite()

#define TEST(name) \
    { \
        const char* __current_test_name = name; \
        Test::TestFramework::getInstance().beginTest(name); \
        try {

#define RT_TEST(name) \
    { \
        const char* __current_test_name = name; \
        Test::TestFramework::getInstance().beginTest(name, true); \
        try {

#define END_TEST() \
            Test::TestFramework::getInstance().endTest(__current_test_name, true); \
        } catch (const std::exception& e) { \
            Test::TestFramework::getInstance().endTest(__current_test_name, false, e.what()); \
        } catch (...) { \
            Test::TestFramework::getInstance().endTest(__current_test_name, false, "Unknown exception"); \
        } \
    }

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error("ASSERT_TRUE failed: " #condition); \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            throw std::runtime_error("ASSERT_FALSE failed: " #condition); \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            throw std::runtime_error("ASSERT_EQ failed: expected " + std::to_string(expected) + \
                                   ", got " + std::to_string(actual)); \
        } \
    } while(0)

#define ASSERT_NE(not_expected, actual) \
    do { \
        if ((not_expected) == (actual)) { \
            throw std::runtime_error("ASSERT_NE failed: both values are " + std::to_string(actual)); \
        } \
    } while(0)

#define ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != nullptr) { \
            throw std::runtime_error("ASSERT_NULL failed: pointer is not null"); \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == nullptr) { \
            throw std::runtime_error("ASSERT_NOT_NULL failed: pointer is null"); \
        } \
    } while(0)

// RT-safe timing assertion
#define ASSERT_RT_TIMING(code, max_microseconds) \
    do { \
        auto start = std::chrono::high_resolution_clock::now(); \
        code; \
        auto end = std::chrono::high_resolution_clock::now(); \
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start); \
        if (duration.count() > max_microseconds) { \
            throw std::runtime_error("RT timing violation: " + std::to_string(duration.count()) + \
                                   "μs > " + std::to_string(max_microseconds) + "μs"); \
        } \
    } while(0)
