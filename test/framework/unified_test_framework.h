/**
 * @brief Unified Test Framework - Core Implementation
 * 
 * This replaces the fragmented testing approach with a unified,
 * discoverable, and maintainable test system.
 */

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace TestFramework {

/**
 * @brief Test result for individual test
 */
struct TestResult {
    std::string category;
    std::string name;
    bool passed;
    std::string error_message;
    std::chrono::milliseconds duration;
    
    TestResult(const std::string& cat, const std::string& n) 
        : category(cat), name(n), passed(false), duration(0) {}
};

/**
 * @brief Overall test suite results
 */
struct TestSuiteResults {
    std::vector<TestResult> results;
    size_t total_tests = 0;
    size_t passed_tests = 0;
    size_t failed_tests = 0;
    std::chrono::milliseconds total_duration{0};
    
    void addResult(const TestResult& result) {
        results.push_back(result);
        total_tests++;
        if (result.passed) {
            passed_tests++;
        } else {
            failed_tests++;
        }
        total_duration += result.duration;
    }
    
    double getPassRate() const {
        return total_tests > 0 ? (double)passed_tests / total_tests * 100.0 : 0.0;
    }
};

/**
 * @brief Core test runner with automatic test discovery
 */
class TestRunner {
public:
    using TestFunction = std::function<void()>;
    
    static TestRunner& getInstance() {
        static TestRunner instance;
        return instance;
    }
    
    void registerTest(const std::string& category, const std::string& name, TestFunction test) {
        tests_[category][name] = test;
        std::cout << "📝 Registered test: " << category << "::" << name << std::endl;
    }
    
    TestSuiteResults runAllTests() {
        std::cout << "\n🚀 Running All Tests" << std::endl;
        std::cout << "===================" << std::endl;
        
        TestSuiteResults suite_results;
        
        for (const auto& category_pair : tests_) {
            const std::string& category = category_pair.first;
            std::cout << "\n📁 Category: " << category << std::endl;
            
            for (const auto& test_pair : category_pair.second) {
                const std::string& test_name = test_pair.first;
                const TestFunction& test_func = test_pair.second;
                
                TestResult result(category, test_name);
                
                std::cout << "  🧪 Running " << test_name << "... ";
                std::cout.flush();
                
                auto start_time = std::chrono::steady_clock::now();
                
                try {
                    current_test_name_ = category + "::" + test_name;
                    test_func();
                    result.passed = true;
                    std::cout << "✅ PASS";
                } catch (const std::exception& e) {
                    result.passed = false;
                    result.error_message = e.what();
                    std::cout << "❌ FAIL - " << e.what();
                } catch (...) {
                    result.passed = false;
                    result.error_message = "Unknown exception";
                    std::cout << "❌ FAIL - Unknown exception";
                }
                
                auto end_time = std::chrono::steady_clock::now();
                result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                std::cout << " (" << result.duration.count() << "ms)" << std::endl;
                
                suite_results.addResult(result);
            }
        }
        
        printSummary(suite_results);
        return suite_results;
    }
    
    TestSuiteResults runCategory(const std::string& category) {
        std::cout << "\n🎯 Running Category: " << category << std::endl;
        std::cout << "================================" << std::endl;
        
        TestSuiteResults suite_results;
        
        auto category_it = tests_.find(category);
        if (category_it == tests_.end()) {
            std::cout << "❌ Category not found: " << category << std::endl;
            return suite_results;
        }
        
        for (const auto& test_pair : category_it->second) {
            const std::string& test_name = test_pair.first;
            const TestFunction& test_func = test_pair.second;
            
            TestResult result(category, test_name);
            
            std::cout << "🧪 Running " << test_name << "... ";
            std::cout.flush();
            
            auto start_time = std::chrono::steady_clock::now();
            
            try {
                current_test_name_ = category + "::" + test_name;
                test_func();
                result.passed = true;
                std::cout << "✅ PASS";
            } catch (const std::exception& e) {
                result.passed = false;
                result.error_message = e.what();
                std::cout << "❌ FAIL - " << e.what();
            } catch (...) {
                result.passed = false;
                result.error_message = "Unknown exception";
                std::cout << "❌ FAIL - Unknown exception";
            }
            
            auto end_time = std::chrono::steady_clock::now();
            result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            std::cout << " (" << result.duration.count() << "ms)" << std::endl;
            
            suite_results.addResult(result);
        }
        
        printSummary(suite_results);
        return suite_results;
    }
    
    void listTests() const {
        std::cout << "\n📋 Available Tests" << std::endl;
        std::cout << "=================" << std::endl;
        
        for (const auto& category_pair : tests_) {
            std::cout << "\n📁 " << category_pair.first << ":" << std::endl;
            for (const auto& test_pair : category_pair.second) {
                std::cout << "  🧪 " << test_pair.first << std::endl;
            }
        }
    }
    
    // Assertion helpers
    static void assertTrue(const std::string& file, int line, bool condition, const std::string& message = "") {
        if (!condition) {
            std::stringstream ss;
            ss << "Assertion failed at " << file << ":" << line;
            if (!message.empty()) {
                ss << " - " << message;
            }
            throw std::runtime_error(ss.str());
        }
    }
    
    template<typename T>
    static void assertEqual(const std::string& file, int line, const T& expected, const T& actual) {
        if (expected != actual) {
            std::stringstream ss;
            ss << "Assertion failed at " << file << ":" << line 
               << " - Expected: " << expected << ", Actual: " << actual;
            throw std::runtime_error(ss.str());
        }
    }
    
    static void assertNear(const std::string& file, int line, float expected, float actual, float tolerance) {
        if (std::abs(expected - actual) > tolerance) {
            std::stringstream ss;
            ss << "Assertion failed at " << file << ":" << line 
               << " - Expected: " << expected << " (±" << tolerance << "), Actual: " << actual;
            throw std::runtime_error(ss.str());
        }
    }

private:
    std::map<std::string, std::map<std::string, TestFunction>> tests_;
    std::string current_test_name_;
    
    void printSummary(const TestSuiteResults& results) const {
        std::cout << "\n📊 Test Summary" << std::endl;
        std::cout << "===============" << std::endl;
        std::cout << "Total Tests: " << results.total_tests << std::endl;
        std::cout << "✅ Passed: " << results.passed_tests << std::endl;
        std::cout << "❌ Failed: " << results.failed_tests << std::endl;
        std::cout << "📈 Pass Rate: " << std::fixed << std::setprecision(1) << results.getPassRate() << "%" << std::endl;
        std::cout << "⏱️  Total Time: " << results.total_duration.count() << "ms" << std::endl;
        
        if (results.failed_tests > 0) {
            std::cout << "\n❌ Failed Tests:" << std::endl;
            for (const auto& result : results.results) {
                if (!result.passed) {
                    std::cout << "  " << result.category << "::" << result.name 
                              << " - " << result.error_message << std::endl;
                }
            }
        }
    }
};

/**
 * @brief Automatic test registration helper
 */
class TestRegistrar {
public:
    TestRegistrar(const std::string& category, const std::string& name, TestRunner::TestFunction test) {
        TestRunner::getInstance().registerTest(category, name, test);
    }
};

} // namespace TestFramework

// Convenient macros for test definition
#define TEST_UNIT(category, name) \
    static void test_unit_##category##_##name(); \
    static TestFramework::TestRegistrar reg_unit_##category##_##name("unit/" #category, #name, test_unit_##category##_##name); \
    static void test_unit_##category##_##name()

#define TEST_INTEGRATION(category, name) \
    static void test_integration_##category##_##name(); \
    static TestFramework::TestRegistrar reg_integration_##category##_##name("integration/" #category, #name, test_integration_##category##_##name); \
    static void test_integration_##category##_##name()

#define TEST_SYSTEM(name) \
    static void test_system_##name(); \
    static TestFramework::TestRegistrar reg_system_##name("system", #name, test_system_##name); \
    static void test_system_##name()

// Assertion macros
#define ASSERT_TRUE(condition) \
    TestFramework::TestRunner::assertTrue(__FILE__, __LINE__, condition)

#define ASSERT_FALSE(condition) \
    TestFramework::TestRunner::assertTrue(__FILE__, __LINE__, !(condition))

#define ASSERT_EQ(expected, actual) \
    TestFramework::TestRunner::assertEqual(__FILE__, __LINE__, expected, actual)

#define ASSERT_NEAR(expected, actual, tolerance) \
    TestFramework::TestRunner::assertNear(__FILE__, __LINE__, expected, actual, tolerance)
