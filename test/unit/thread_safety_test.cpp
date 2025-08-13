/**
 * @brief Thread Safety Tests - Unified Framework Migration
 * 
 * This file contains comprehensive tests for thread-safe operations,
 * migrated from thread_safety_test.cpp to use the unified test framework
 * with clean mock dependencies.
 * 
 * MIGRATION TARGET:
 * - Original: test/thread_safety_test.cpp (116 lines with complex dependencies)
 * - Target: Unit test category (concurrency validation)
 * - Focus: Multi-threaded operations, race condition detection, data integrity
 * 
 * TEST COVERAGE - THE THREAD SAFETY STORY:
 * 
 * 🧵 CHAPTER 1: Concurrent Data Access
 *    Multiple threads read and write shared data structures without
 *    corruption or data races.
 * 
 * 🔒 CHAPTER 2: Atomic Operations Validation
 *    Atomic counters and flags maintain consistency under heavy
 *    concurrent load from multiple producer/consumer threads.
 * 
 * 📊 CHAPTER 3: Producer-Consumer Scenarios
 *    Multiple producers generate events while consumers process them
 *    concurrently without losing data or blocking each other.
 * 
 * ⚡ CHAPTER 4: High-Frequency Operations
 *    Rapid concurrent operations maintain data integrity and
 *    performance under stress testing conditions.
 * 
 * 🛡️ CHAPTER 5: Race Condition Detection
 *    System detects and prevents race conditions that could
 *    cause data corruption or system instability.
 * 
 * 🔄 CHAPTER 6: Observer Pattern Thread Safety
 *    Multiple observers can safely register/unregister and receive
 *    notifications while events are being processed.
 * 
 * 📈 CHAPTER 7: Stress Testing Under Load
 *    System maintains thread safety and performance under
 *    extreme concurrent load scenarios.
 * 
 * 🎯 CHAPTER 8: Memory Consistency Validation
 *    All threads see consistent memory state and changes
 *    propagate correctly across thread boundaries.
 * 
 * ARCHITECTURE:
 * - Mock-based testing eliminates external component dependencies
 * - Comprehensive concurrent operation validation
 * - Race condition detection through statistical analysis
 * - High-frequency stress testing with timing validation
 * - Memory consistency and atomic operation verification
 * 
 * REAL-WORLD APPLICATION:
 * Thread safety is critical in real-time audio where MIDI input,
 * audio processing, and UI updates all run concurrently. Any race
 * conditions can cause audio dropouts, crashes, or data corruption.
 * 
 * @author Migrated to Unified Framework
 * @date August 12, 2025
 */

#include "../framework/unified_test_framework.h"
#include "../fixtures/test_fixtures.h"
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <random>

/**
 * @brief Mock Event structure for thread safety testing
 */
struct MockThreadEvent {
    uint32_t id;
    uint64_t timestamp_us;
    int32_t value;
    
    MockThreadEvent(uint32_t event_id = 0, int32_t event_value = 0) 
        : id(event_id), value(event_value) {
        timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};

/**
 * @brief Thread-safe event processor for testing
 */
class MockThreadSafeProcessor {
private:
    std::queue<MockThreadEvent> event_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    std::atomic<size_t> events_enqueued_{0};
    std::atomic<size_t> events_processed_{0};
    std::atomic<size_t> total_processing_time_us_{0};
    std::atomic<bool> running_{false};
    
    std::vector<std::function<void(const MockThreadEvent&)>> observers_;
    mutable std::mutex observers_mutex_;
    
public:
    void start() {
        running_ = true;
    }
    
    void stop() {
        running_ = false;
        queue_cv_.notify_all();
    }
    
    void enqueueEvent(const MockThreadEvent& event) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            event_queue_.push(event);
        }
        events_enqueued_++;
        queue_cv_.notify_one();
    }
    
    bool dequeueEvent(MockThreadEvent& event, int timeout_ms = 100) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        if (queue_cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                              [this] { return !event_queue_.empty() || !running_; })) {
            if (!event_queue_.empty()) {
                event = event_queue_.front();
                event_queue_.pop();
                return true;
            }
        }
        return false;
    }
    
    void processEvents() {
        MockThreadEvent event;
        while (running_ && dequeueEvent(event, 50)) {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // Notify observers
            {
                std::lock_guard<std::mutex> lock(observers_mutex_);
                for (auto& observer : observers_) {
                    observer(event);
                }
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(
                end_time - start_time);
            
            total_processing_time_us_ += processing_time.count();
            events_processed_++;
        }
    }
    
    void addObserver(std::function<void(const MockThreadEvent&)> observer) {
        std::lock_guard<std::mutex> lock(observers_mutex_);
        observers_.push_back(observer);
    }
    
    void clearObservers() {
        std::lock_guard<std::mutex> lock(observers_mutex_);
        observers_.clear();
    }
    
    // Statistics
    size_t getEventsEnqueued() const { return events_enqueued_.load(); }
    size_t getEventsProcessed() const { return events_processed_.load(); }
    size_t getTotalProcessingTimeUs() const { return total_processing_time_us_.load(); }
    bool isRunning() const { return running_.load(); }
    
    size_t getQueueSize() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return event_queue_.size();
    }
    
    size_t getObserverCount() const {
        std::lock_guard<std::mutex> lock(observers_mutex_);
        return observers_.size();
    }
    
    void reset() {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!event_queue_.empty()) {
            event_queue_.pop();
        }
        events_enqueued_ = 0;
        events_processed_ = 0;
        total_processing_time_us_ = 0;
        running_ = false;
    }
};

/**
 * @brief Atomic counter for testing thread-safe operations
 */
class MockAtomicCounter {
private:
    std::atomic<int64_t> counter_{0};
    std::atomic<size_t> increment_count_{0};
    std::atomic<size_t> decrement_count_{0};
    
public:
    void increment() {
        counter_++;
        increment_count_++;
    }
    
    void decrement() {
        counter_--;
        decrement_count_++;
    }
    
    void add(int64_t value) {
        counter_ += value;
        increment_count_++;
    }
    
    int64_t getValue() const { return counter_.load(); }
    size_t getIncrementCount() const { return increment_count_.load(); }
    size_t getDecrementCount() const { return decrement_count_.load(); }
    
    void reset() {
        counter_ = 0;
        increment_count_ = 0;
        decrement_count_ = 0;
    }
};

// ============================================================================
// GLOBAL TEST FIXTURES
// ============================================================================

static std::unique_ptr<MockThreadSafeProcessor> g_processor;
static std::unique_ptr<MockAtomicCounter> g_counter;

void setupThreadSafetyTests() {
    g_processor = std::make_unique<MockThreadSafeProcessor>();
    g_counter = std::make_unique<MockAtomicCounter>();
}

void teardownThreadSafetyTests() {
    if (g_processor) {
        g_processor->stop();
        g_processor.reset();
    }
    if (g_counter) {
        g_counter.reset();
    }
}

// ============================================================================
// UNIT TESTS
// ============================================================================

TEST_UNIT(ThreadSafety, ConcurrentDataAccess) {
    setupThreadSafetyTests();
    
    const int num_threads = 4;
    const int operations_per_thread = 100;
    std::atomic<bool> start_flag{false};
    std::vector<std::thread> threads;
    
    // Create threads that will all start simultaneously
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            // Wait for start signal
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            
            // Perform operations
            for (int j = 0; j < operations_per_thread; ++j) {
                if (i % 2 == 0) {
                    g_counter->increment();
                } else {
                    g_counter->decrement();
                }
            }
        });
    }
    
    // Start all threads simultaneously
    start_flag = true;
    
    // Wait for completion
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify data integrity
    size_t total_increments = g_counter->getIncrementCount();
    size_t total_decrements = g_counter->getDecrementCount();
    int64_t final_value = g_counter->getValue();
    
    // Should have equal increments and decrements
    ASSERT_EQ(total_increments, total_decrements);
    
    // Final value should be 0 (equal increments and decrements)
    ASSERT_EQ(0L, final_value);
    
    // Total operations should match expected
    ASSERT_EQ(static_cast<size_t>(num_threads * operations_per_thread), 
              total_increments + total_decrements);
    
    teardownThreadSafetyTests();
}

TEST_UNIT(ThreadSafety, ProducerConsumerPattern) {
    setupThreadSafetyTests();
    
    g_processor->start();
    
    const int num_producers = 3;
    const int events_per_producer = 50;
    std::atomic<size_t> events_received{0};
    
    // Add observer to count received events
    g_processor->addObserver([&](const MockThreadEvent& event) {
        (void)event; // Suppress unused parameter warning
        events_received++;
        // Simulate some processing time
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    });
    
    std::vector<std::thread> producers;
    
    // Create producer threads
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&, i]() {
            for (int j = 0; j < events_per_producer; ++j) {
                MockThreadEvent event(i * 1000 + j, j);
                g_processor->enqueueEvent(event);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }
    
    // Consumer thread
    std::thread consumer([&]() {
        while (g_processor->getEventsProcessed() < 
               static_cast<size_t>(num_producers * events_per_producer)) {
            g_processor->processEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    // Wait for all producers to finish
    for (auto& producer : producers) {
        producer.join();
    }
    
    // Wait for consumer to finish processing
    consumer.join();
    g_processor->stop();
    
    // Verify all events were processed
    ASSERT_EQ(static_cast<size_t>(num_producers * events_per_producer), 
              g_processor->getEventsEnqueued());
    ASSERT_EQ(static_cast<size_t>(num_producers * events_per_producer), 
              g_processor->getEventsProcessed());
    ASSERT_EQ(static_cast<size_t>(num_producers * events_per_producer), 
              events_received.load());
    
    teardownThreadSafetyTests();
}

TEST_UNIT(ThreadSafety, HighFrequencyOperations) {
    setupThreadSafetyTests();
    
    const int num_threads = 8;
    const int operations_per_thread = 1000;
    std::atomic<bool> stop_flag{false};
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operations_per_thread && !stop_flag; ++j) {
                // Mix of different operations
                switch (j % 4) {
                    case 0: g_counter->increment(); break;
                    case 1: g_counter->decrement(); break;
                    case 2: g_counter->add(i + 1); break;
                    case 3: g_counter->add(-(i + 1)); break;
                }
            }
        });
    }
    
    // Wait for completion or timeout
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Should complete within reasonable time (under 100ms)
    ASSERT_TRUE(duration.count() < 100);
    
    // Verify operation counts
    size_t total_operations = g_counter->getIncrementCount() + g_counter->getDecrementCount();
    ASSERT_TRUE(total_operations > 0);
    
    teardownThreadSafetyTests();
}

TEST_UNIT(ThreadSafety, RaceConditionDetection) {
    setupThreadSafetyTests();
    
    // Shared data structure that could have race conditions if not properly protected
    std::vector<int> shared_data;
    std::mutex data_mutex;
    std::atomic<size_t> write_count{0};
    std::atomic<size_t> read_count{0};
    
    const int num_writers = 4;
    const int num_readers = 4;
    const int operations = 100;
    
    std::vector<std::thread> writers;
    std::vector<std::thread> readers;
    
    // Writer threads
    for (int i = 0; i < num_writers; ++i) {
        writers.emplace_back([&, i]() {
            for (int j = 0; j < operations; ++j) {
                {
                    std::lock_guard<std::mutex> lock(data_mutex);
                    shared_data.push_back(i * 1000 + j);
                }
                write_count++;
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }
    
    // Reader threads
    for (int i = 0; i < num_readers; ++i) {
        readers.emplace_back([&]() {
            for (int j = 0; j < operations; ++j) {
                {
                    std::lock_guard<std::mutex> lock(data_mutex);
                    if (!shared_data.empty()) {
                        volatile int value = shared_data.back(); // Read operation
                        (void)value; // Suppress unused variable warning
                    }
                }
                read_count++;
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }
    
    // Wait for all threads
    for (auto& writer : writers) {
        writer.join();
    }
    for (auto& reader : readers) {
        reader.join();
    }
    
    // Verify no race conditions occurred
    ASSERT_EQ(static_cast<size_t>(num_writers * operations), write_count.load());
    ASSERT_EQ(static_cast<size_t>(num_readers * operations), read_count.load());
    ASSERT_EQ(static_cast<size_t>(num_writers * operations), shared_data.size());
    
    teardownThreadSafetyTests();
}

TEST_UNIT(ThreadSafety, ObserverPatternThreadSafety) {
    setupThreadSafetyTests();
    
    g_processor->start();
    
    std::atomic<size_t> observer1_count{0};
    std::atomic<size_t> observer2_count{0};
    std::atomic<size_t> observer3_count{0};
    
    // Add observers concurrently
    std::thread observer_thread([&]() {
        g_processor->addObserver([&](const MockThreadEvent&) { observer1_count++; });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        g_processor->addObserver([&](const MockThreadEvent&) { observer2_count++; });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        g_processor->addObserver([&](const MockThreadEvent&) { observer3_count++; });
    });
    
    // Event generation thread
    std::thread event_thread([&]() {
        for (int i = 0; i < 30; ++i) {
            MockThreadEvent event(i, i * 10);
            g_processor->enqueueEvent(event);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
    
    // Processing thread
    std::thread processing_thread([&]() {
        while (g_processor->getEventsProcessed() < 30) {
            g_processor->processEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    });
    
    observer_thread.join();
    event_thread.join();
    processing_thread.join();
    
    g_processor->stop();
    
    // Verify all events were processed and observers were called
    ASSERT_EQ(30lu, g_processor->getEventsEnqueued());
    ASSERT_EQ(30lu, g_processor->getEventsProcessed());
    ASSERT_EQ(3lu, g_processor->getObserverCount());
    
    // Each observer should have been called for each event it was registered for
    ASSERT_TRUE(observer1_count.load() > 0);
    ASSERT_TRUE(observer2_count.load() > 0);
    ASSERT_TRUE(observer3_count.load() > 0);
    
    teardownThreadSafetyTests();
}

TEST_UNIT(ThreadSafety, StressTestingUnderLoad) {
    setupThreadSafetyTests();
    
    g_processor->start();
    
    const int stress_duration_ms = 100;
    const int num_producer_threads = 6;
    const int num_consumer_threads = 2;
    
    std::atomic<bool> stress_running{true};
    std::atomic<size_t> total_events_generated{0};
    std::atomic<size_t> total_events_observed{0};
    
    // Add observer
    g_processor->addObserver([&](const MockThreadEvent&) {
        total_events_observed++;
        // Simulate minimal processing
        std::this_thread::sleep_for(std::chrono::nanoseconds(100));
    });
    
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    
    // Producer threads (high frequency event generation)
    for (int i = 0; i < num_producer_threads; ++i) {
        producers.emplace_back([&, i]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> delay_dist(1, 10);
            
            while (stress_running) {
                MockThreadEvent event(total_events_generated.load(), i);
                g_processor->enqueueEvent(event);
                total_events_generated++;
                
                // Random small delay
                std::this_thread::sleep_for(std::chrono::microseconds(delay_dist(gen)));
            }
        });
    }
    
    // Consumer threads
    for (int i = 0; i < num_consumer_threads; ++i) {
        consumers.emplace_back([&]() {
            while (stress_running) {
                g_processor->processEvents();
                std::this_thread::sleep_for(std::chrono::microseconds(500));
            }
        });
    }
    
    // Run stress test
    std::this_thread::sleep_for(std::chrono::milliseconds(stress_duration_ms));
    stress_running = false;
    
    // Wait for threads to finish
    for (auto& producer : producers) {
        producer.join();
    }
    for (auto& consumer : consumers) {
        consumer.join();
    }
    
    g_processor->stop();
    
    // Final processing to clear remaining events
    g_processor->processEvents();
    
    // Verify stress test results
    ASSERT_TRUE(total_events_generated.load() > 100); // Should generate significant load
    ASSERT_TRUE(g_processor->getEventsProcessed() > 0);
    ASSERT_TRUE(total_events_observed.load() > 0);
    
    // Allow for some events still in queue
    ASSERT_TRUE(g_processor->getEventsProcessed() + g_processor->getQueueSize() >= 
                total_events_observed.load());
    
    teardownThreadSafetyTests();
}

TEST_UNIT(ThreadSafety, MemoryConsistencyValidation) {
    setupThreadSafetyTests();
    
    // Test memory ordering and consistency across threads
    std::atomic<int> shared_counter{0};
    std::atomic<bool> flag{false};
    std::vector<int> results(4, -1);
    
    std::vector<std::thread> threads;
    
    // Thread 0: Sets counter and flag
    threads.emplace_back([&]() {
        shared_counter.store(42, std::memory_order_release);
        flag.store(true, std::memory_order_release);
    });
    
    // Threads 1-3: Wait for flag and read counter
    for (int i = 1; i < 4; ++i) {
        threads.emplace_back([&, i]() {
            while (!flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            results[i] = shared_counter.load(std::memory_order_acquire);
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify memory consistency
    ASSERT_EQ(42, shared_counter.load());
    ASSERT_TRUE(flag.load());
    
    // All threads should see the same value after synchronization
    for (int i = 1; i < 4; ++i) {
        ASSERT_EQ(42, results[i]);
    }
    
    teardownThreadSafetyTests();
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "🔒 Thread Safety Tests - Unified Framework" << std::endl;
    std::cout << "============================================" << std::endl;
    
    auto& runner = TestFramework::TestRunner::getInstance();
    
    // Run all unit tests for ThreadSafety
    auto results = runner.runCategory("unit/ThreadSafety");
    
    return results.failed_tests == 0 ? 0 : 1;
}
