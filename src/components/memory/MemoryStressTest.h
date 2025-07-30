#pragma once

#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <string>

/**
 * @brief Memory stress testing system to detect leaks early
 * 
 * Performs controlled stress tests to identify memory leaks,
 * fragmentation issues, and performance bottlenecks.
 */
class MemoryStressTest {
public:
    struct TestResults {
        bool passed = false;
        size_t memory_leaked = 0;
        size_t max_memory_used = 0;
        double fragmentation_increase = 0.0;
        std::chrono::milliseconds duration{0};
        std::string error_message;
    };

    enum class TestType {
        OBJECT_CREATION_DELETION,
        RAPID_ALLOCATION,
        FRAGMENTATION_TEST,
        STRESS_OBSERVER_PATTERN,
        FULL_SUITE
    };

    using ProgressCallback = std::function<void(int percentage, const std::string& status)>;

    static MemoryStressTest& getInstance() {
        static MemoryStressTest instance;
        return instance;
    }

    // Test execution
    TestResults runTest(TestType type, ProgressCallback progress = nullptr);
    TestResults runFullTestSuite(ProgressCallback progress = nullptr);
    
    // Configuration
    void setTestDuration(std::chrono::seconds duration) { test_duration_ = duration; }
    void setMaxObjects(size_t count) { max_objects_ = count; }
    void setStressLevel(int level) { stress_level_ = std::clamp(level, 1, 10); }

    // Reporting
    void printTestResults(const TestResults& results);
    void saveTestReport(const TestResults& results, const std::string& filename);

private:
    MemoryStressTest() = default;

    // Individual test implementations
    TestResults testObjectCreationDeletion(ProgressCallback progress);
    TestResults testRapidAllocation(ProgressCallback progress);
    TestResults testFragmentation(ProgressCallback progress);
    TestResults testObserverPattern(ProgressCallback progress);

    // Utilities
    size_t getCurrentMemoryUsage();
    double getCurrentFragmentation();
    void createTestObjects(size_t count);
    void deleteTestObjects();

    std::chrono::seconds test_duration_{30};
    size_t max_objects_ = 1000;
    int stress_level_ = 5;
    std::vector<void*> test_objects_;
};

#ifdef MEMORY_STRESS_TEST
#define STRESS_TEST_POINT(name) MemoryStressTest::getInstance().recordCheckpoint(name)
#else
#define STRESS_TEST_POINT(name)
#endif
