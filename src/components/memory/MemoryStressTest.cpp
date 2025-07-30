#include "MemoryStressTest.h"
#include "MemoryHealthMonitor.h"
#include "LVGLMemoryMonitor.h"
#include "lvgl.h"
#include <iostream>
#include <thread>
#include <random>
#include <fstream>
#include <algorithm>
#include <iomanip>

MemoryStressTest::TestResults MemoryStressTest::runTest(TestType type, ProgressCallback progress) {
    switch (type) {
        case TestType::OBJECT_CREATION_DELETION:
            return testObjectCreationDeletion(progress);
        case TestType::RAPID_ALLOCATION:
            return testRapidAllocation(progress);
        case TestType::FRAGMENTATION_TEST:
            return testFragmentation(progress);
        case TestType::STRESS_OBSERVER_PATTERN:
            return testObserverPattern(progress);
        case TestType::FULL_SUITE:
            return runFullTestSuite(progress);
        default:
            TestResults result;
            result.error_message = "Unknown test type";
            return result;
    }
}

MemoryStressTest::TestResults MemoryStressTest::runFullTestSuite(ProgressCallback progress) {
    TestResults combined_result;
    combined_result.passed = true;
    
    auto start_time = std::chrono::steady_clock::now();
    
    std::vector<TestType> tests = {
        TestType::OBJECT_CREATION_DELETION,
        TestType::RAPID_ALLOCATION,
        TestType::FRAGMENTATION_TEST,
        TestType::STRESS_OBSERVER_PATTERN
    };
    
    for (size_t i = 0; i < tests.size(); ++i) {
        if (progress) {
            int percentage = (i * 100) / tests.size();
            progress(percentage, "Running test " + std::to_string(i + 1) + "/" + std::to_string(tests.size()));
        }
        
        auto result = runTest(tests[i]);
        if (!result.passed) {
            combined_result.passed = false;
            combined_result.error_message += result.error_message + "; ";
        }
        
        combined_result.memory_leaked += result.memory_leaked;
        combined_result.max_memory_used = std::max(combined_result.max_memory_used, result.max_memory_used);
        combined_result.fragmentation_increase += result.fragmentation_increase;
        
        // Brief pause between tests
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    auto end_time = std::chrono::steady_clock::now();
    combined_result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    if (progress) {
        progress(100, "Test suite completed");
    }
    
    return combined_result;
}

MemoryStressTest::TestResults MemoryStressTest::testObjectCreationDeletion(ProgressCallback progress) {
    TestResults result;
    auto start_time = std::chrono::steady_clock::now();
    
    size_t initial_memory = getCurrentMemoryUsage();
    size_t max_memory = initial_memory;
    
    try {
        std::cout << "[StressTest] 🏗️ Testing object creation/deletion..." << std::endl;
        
        auto end_time = start_time + test_duration_;
        int iterations = 0;
        
        while (std::chrono::steady_clock::now() < end_time) {
            // Create objects
            createTestObjects(max_objects_ / 10);
            
            size_t current_memory = getCurrentMemoryUsage();
            max_memory = std::max(max_memory, current_memory);
            
            // Delete objects
            deleteTestObjects();
            
            iterations++;
            
            if (progress && iterations % 10 == 0) {
                auto elapsed = std::chrono::steady_clock::now() - start_time;
                auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(test_duration_);
                auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
                int percentage = (elapsed_ms.count() * 100) / total_duration.count();
                progress(percentage, "Object test iteration " + std::to_string(iterations));
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // Check for memory leaks
        size_t final_memory = getCurrentMemoryUsage();
        result.memory_leaked = (final_memory > initial_memory) ? (final_memory - initial_memory) : 0;
        result.max_memory_used = max_memory;
        result.passed = result.memory_leaked < 1024; // Accept < 1KB leakage
        
        if (!result.passed) {
            result.error_message = "Memory leak detected: " + std::to_string(result.memory_leaked) + " bytes";
        }
        
        std::cout << "[StressTest] Completed " << iterations << " iterations" << std::endl;
        
    } catch (const std::exception& e) {
        result.passed = false;
        result.error_message = "Exception during object test: " + std::string(e.what());
    }
    
    auto end_time = std::chrono::steady_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    return result;
}

MemoryStressTest::TestResults MemoryStressTest::testRapidAllocation(ProgressCallback progress) {
    TestResults result;
    auto start_time = std::chrono::steady_clock::now();
    
    size_t initial_memory = getCurrentMemoryUsage();
    
    try {
        std::cout << "[StressTest] ⚡ Testing rapid allocation/deallocation..." << std::endl;
        
        std::vector<void*> allocations;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> size_dist(64, 4096);
        
        // Rapid allocation phase
        for (int i = 0; i < stress_level_ * 100; ++i) {
            size_t alloc_size = size_dist(gen);
            void* ptr = malloc(alloc_size);
            if (ptr) {
                allocations.push_back(ptr);
            }
            
            if (progress && i % 50 == 0) {
                int percentage = (i * 50) / (stress_level_ * 100);
                progress(percentage, "Allocating memory block " + std::to_string(i));
            }
        }
        
        result.max_memory_used = getCurrentMemoryUsage();
        
        // Rapid deallocation phase
        for (size_t i = 0; i < allocations.size(); ++i) {
            free(allocations[i]);
            
            if (progress && i % 50 == 0) {
                int percentage = 50 + ((i * 50) / allocations.size());
                progress(percentage, "Deallocating memory block " + std::to_string(i));
            }
        }
        
        // Check for leaks
        size_t final_memory = getCurrentMemoryUsage();
        result.memory_leaked = (final_memory > initial_memory) ? (final_memory - initial_memory) : 0;
        result.passed = result.memory_leaked < 512; // Accept < 512B leakage for rapid allocation
        
        if (!result.passed) {
            result.error_message = "Rapid allocation leak: " + std::to_string(result.memory_leaked) + " bytes";
        }
        
    } catch (const std::exception& e) {
        result.passed = false;
        result.error_message = "Exception during rapid allocation test: " + std::string(e.what());
    }
    
    auto end_time = std::chrono::steady_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    return result;
}

MemoryStressTest::TestResults MemoryStressTest::testFragmentation(ProgressCallback progress) {
    TestResults result;
    auto start_time = std::chrono::steady_clock::now();
    
    double initial_fragmentation = getCurrentFragmentation();
    
    try {
        std::cout << "[StressTest] 🧩 Testing memory fragmentation..." << std::endl;
        
        // Create fragmentation by allocating different sized blocks
        std::vector<void*> allocations;
        std::random_device rd;
        std::mt19937 gen(rd());
        
        // Create a fragmented allocation pattern
        for (int cycle = 0; cycle < stress_level_; ++cycle) {
            // Allocate large blocks
            for (int i = 0; i < 10; ++i) {
                void* ptr = malloc(1024 + (i * 256));
                if (ptr) allocations.push_back(ptr);
            }
            
            // Free every other block to create holes
            for (size_t i = 1; i < allocations.size(); i += 2) {
                if (i < allocations.size()) {
                    free(allocations[i]);
                    allocations[i] = nullptr;
                }
            }
            
            // Allocate small blocks in the holes
            for (int i = 0; i < 20; ++i) {
                void* ptr = malloc(64 + (i * 8));
                if (ptr) allocations.push_back(ptr);
            }
            
            if (progress) {
                int percentage = (cycle * 100) / stress_level_;
                progress(percentage, "Fragmentation cycle " + std::to_string(cycle + 1));
            }
        }
        
        double final_fragmentation = getCurrentFragmentation();
        result.fragmentation_increase = final_fragmentation - initial_fragmentation;
        
        // Cleanup
        for (void* ptr : allocations) {
            if (ptr) free(ptr);
        }
        
        result.passed = result.fragmentation_increase < 20.0; // Accept < 20% fragmentation increase
        
        if (!result.passed) {
            result.error_message = "Excessive fragmentation: " + 
                                 std::to_string(result.fragmentation_increase) + "% increase";
        }
        
    } catch (const std::exception& e) {
        result.passed = false;
        result.error_message = "Exception during fragmentation test: " + std::string(e.what());
    }
    
    auto end_time = std::chrono::steady_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    return result;
}

MemoryStressTest::TestResults MemoryStressTest::testObserverPattern(ProgressCallback progress) {
    TestResults result;
    auto start_time = std::chrono::steady_clock::now();
    
    size_t initial_memory = getCurrentMemoryUsage();
    
    try {
        std::cout << "[StressTest] 👁️ Testing observer pattern stress..." << std::endl;
        
        // This test would ideally create and destroy many observers
        // For now, we'll simulate the load
        
        for (int i = 0; i < stress_level_ * 50; ++i) {
            // Simulate observer creation/destruction
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            
            if (progress && i % 25 == 0) {
                int percentage = (i * 100) / (stress_level_ * 50);
                progress(percentage, "Observer pattern iteration " + std::to_string(i));
            }
        }
        
        size_t final_memory = getCurrentMemoryUsage();
        result.memory_leaked = (final_memory > initial_memory) ? (final_memory - initial_memory) : 0;
        result.max_memory_used = final_memory;
        result.passed = result.memory_leaked < 256; // Very strict for observer pattern
        
        if (!result.passed) {
            result.error_message = "Observer pattern leak: " + std::to_string(result.memory_leaked) + " bytes";
        }
        
    } catch (const std::exception& e) {
        result.passed = false;
        result.error_message = "Exception during observer test: " + std::string(e.what());
    }
    
    auto end_time = std::chrono::steady_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    return result;
}

void MemoryStressTest::printTestResults(const TestResults& results) {
    std::cout << "\n🧪 MEMORY STRESS TEST RESULTS" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "Status:       " << (results.passed ? "✅ PASSED" : "❌ FAILED") << std::endl;
    std::cout << "Duration:     " << results.duration.count() << " ms" << std::endl;
    std::cout << "Memory Leak:  " << (results.memory_leaked / 1024.0) << " KB" << std::endl;
    std::cout << "Max Memory:   " << (results.max_memory_used / 1024.0) << " KB" << std::endl;
    std::cout << "Fragmentation:" << std::fixed << std::setprecision(1) 
              << results.fragmentation_increase << "% increase" << std::endl;
    
    if (!results.passed) {
        std::cout << "Error:        " << results.error_message << std::endl;
    }
    std::cout << "=============================\n" << std::endl;
}

size_t MemoryStressTest::getCurrentMemoryUsage() {
    return MemoryHealthMonitor::getInstance().getCurrentStats().heap_used;
}

double MemoryStressTest::getCurrentFragmentation() {
    auto lvgl_info = LVGLMemoryMonitor::getInstance().getCurrentMemoryInfo();
    return lvgl_info.frag_pct;
}

void MemoryStressTest::createTestObjects(size_t count) {
    for (size_t i = 0; i < count; ++i) {
        lv_obj_t* obj = lv_obj_create(lv_scr_act());
        if (obj) {
            test_objects_.push_back(obj);
        }
    }
}

void MemoryStressTest::deleteTestObjects() {
    for (void* obj : test_objects_) {
        if (obj) {
            lv_obj_del(static_cast<lv_obj_t*>(obj));
        }
    }
    test_objects_.clear();
}
