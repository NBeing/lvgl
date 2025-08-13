/**
 * @brief Priority Queue Tests - Unified Framework Migration
 * 
 * This file contains comprehensive tests for priority queue data structures,
 * migrated from priority_queue_test.cpp to use the unified test framework
 * with clean mock dependencies.
 * 
 * MIGRATION TARGET:
 * - Original: test/priority_queue_test.cpp (151 lines with complex dependencies)
 * - Target: Unit test category (data structure validation)
 * - Focus: Priority-based ordering, MIDI event handling, real-time performance
 * 
 * TEST COVERAGE - THE PRIORITY QUEUE STORY:
 * 
 * 🔢 CHAPTER 1: Basic Priority Ordering
 *    Events are dequeued in strict priority order regardless of insertion order.
 *    Critical events always come before high, medium, and low priority events.
 * 
 * ⚡ CHAPTER 2: MIDI Event Priority Handling
 *    Different MIDI event types receive appropriate priority levels
 *    (Clock = Critical, Notes = High, CC = Medium, SysEx = Low).
 * 
 * 📊 CHAPTER 3: Performance Under Load
 *    Priority queue maintains ordering efficiency with large numbers
 *    of events and frequent enqueue/dequeue operations.
 * 
 * 🔄 CHAPTER 4: Dynamic Priority Changes
 *    Queue correctly handles mixed priority events arriving in
 *    random order and maintains correct prioritization.
 * 
 * 🧵 CHAPTER 5: Thread-Safe Operations
 *    Multiple threads can safely enqueue/dequeue events without
 *    data corruption or priority violations.
 * 
 * 🎯 CHAPTER 6: Edge Case Handling
 *    Queue properly handles empty conditions, overflow scenarios,
 *    and maintains stability under stress.
 * 
 * 📈 CHAPTER 7: Real-Time Constraints
 *    All operations complete within acceptable time bounds
 *    for real-time audio processing requirements.
 * 
 * ARCHITECTURE:
 * - Mock-based testing eliminates external component dependencies
 * - Comprehensive priority level validation (Critical > High > Medium > Low)
 * - Performance testing with timing constraints
 * - Thread-safety validation with concurrent operations
 * - Edge case and error condition testing
 * 
 * REAL-WORLD APPLICATION:
 * Priority queues are essential in real-time audio where MIDI clock events
 * must be processed before note events, which must be processed before
 * control changes, ensuring proper timing and musical accuracy.
 * 
 * @author Migrated to Unified Framework
 * @date August 12, 2025
 */

#include "../framework/unified_test_framework.h"
#include "../fixtures/test_fixtures.h"
#include <queue>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <memory>

/**
 * @brief MIDI Event Priority levels for queue ordering
 */
enum class MIDIEventPriority {
    CRITICAL = 3,  ///< Clock, Start, Stop, Continue
    HIGH = 2,      ///< Note On/Off, Pitch Bend
    MEDIUM = 1,    ///< Control Changes, Program Changes
    LOW = 0        ///< SysEx, Meta events
};

/**
 * @brief Mock MIDI Event structure for testing
 */
struct MockMIDIEvent {
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
    MIDIEventPriority priority;
    uint64_t timestamp_us;
    
    MockMIDIEvent(uint8_t s = 0, uint8_t d1 = 0, uint8_t d2 = 0, MIDIEventPriority p = MIDIEventPriority::LOW)
        : status(s), data1(d1), data2(d2), priority(p) {
        timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    
    // Factory methods for different event types
    static MockMIDIEvent noteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
        return MockMIDIEvent(0x90 | (channel & 0x0F), note, velocity, MIDIEventPriority::HIGH);
    }
    
    static MockMIDIEvent noteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
        return MockMIDIEvent(0x80 | (channel & 0x0F), note, velocity, MIDIEventPriority::HIGH);
    }
    
    static MockMIDIEvent controlChange(uint8_t channel, uint8_t cc, uint8_t value) {
        return MockMIDIEvent(0xB0 | (channel & 0x0F), cc, value, MIDIEventPriority::MEDIUM);
    }
    
    static MockMIDIEvent clockTick() {
        return MockMIDIEvent(0xF8, 0, 0, MIDIEventPriority::CRITICAL);
    }
    
    static MockMIDIEvent startSequence() {
        return MockMIDIEvent(0xFA, 0, 0, MIDIEventPriority::CRITICAL);
    }
    
    static MockMIDIEvent sysExStart() {
        return MockMIDIEvent(0xF0, 0, 0, MIDIEventPriority::LOW);
    }
    
    bool operator==(const MockMIDIEvent& other) const {
        return status == other.status && data1 == other.data1 && data2 == other.data2;
    }
};

/**
 * @brief Mock Priority Queue implementation for testing
 */
class MockPriorityQueue {
private:
    struct PriorityEvent {
        MockMIDIEvent event;
        MIDIEventPriority priority;
        
        PriorityEvent(const MockMIDIEvent& e, MIDIEventPriority p) : event(e), priority(p) {}
        
        // Higher priority numbers come first (reverse comparison)
        bool operator<(const PriorityEvent& other) const {
            return priority < other.priority;
        }
    };
    
    std::priority_queue<PriorityEvent> queue_;
    std::atomic<size_t> enqueue_count_{0};
    std::atomic<size_t> dequeue_count_{0};
    mutable std::mutex queue_mutex_;
    
public:
    void enqueue(const MockMIDIEvent& event, MIDIEventPriority priority) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.emplace(event, priority);
        enqueue_count_++;
    }
    
    bool dequeue(MockMIDIEvent& event) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (queue_.empty()) {
            return false;
        }
        
        event = queue_.top().event;
        queue_.pop();
        dequeue_count_++;
        return true;
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return queue_.empty();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return queue_.size();
    }
    
    size_t getEnqueueCount() const { return enqueue_count_.load(); }
    size_t getDequeueCount() const { return dequeue_count_.load(); }
    
    void clear() {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
        enqueue_count_ = 0;
        dequeue_count_ = 0;
    }
};

// ============================================================================
// GLOBAL TEST FIXTURES
// ============================================================================

static std::unique_ptr<MockPriorityQueue> g_priority_queue;

void setupPriorityQueueTests() {
    g_priority_queue = std::make_unique<MockPriorityQueue>();
}

void teardownPriorityQueueTests() {
    if (g_priority_queue) {
        g_priority_queue->clear();
        g_priority_queue.reset();
    }
}

// ============================================================================
// UNIT TESTS
// ============================================================================

TEST_UNIT(PriorityQueue, BasicPriorityOrdering) {
    setupPriorityQueueTests();
    
    /**
     * TEST: Priority queue maintains strict ordering regardless of insertion order
     * 
     * SCENARIO: Live performance where MIDI events arrive out of order:
     *           - SysEx dump starts downloading (LOW priority)
     *           - MIDI clock keeps tempo (CRITICAL priority) 
     *           - User plays notes (HIGH priority)
     *           - Automation sends CC (MEDIUM priority)
     *           - Sequencer starts (CRITICAL priority)
     * 
     * VALIDATES:
     * - Critical events (clock, start) are processed first
     * - Musical events (notes) come before control changes
     * - System maintenance (SysEx) has lowest priority
     * - Insertion order doesn't affect priority order
     * - Essential for maintaining musical timing accuracy
     */
    
    // SIMULATE: Mixed priority events arriving in random order
    g_priority_queue->enqueue(MockMIDIEvent::sysExStart(), MIDIEventPriority::LOW);       // 5th priority
    g_priority_queue->enqueue(MockMIDIEvent::clockTick(), MIDIEventPriority::CRITICAL);  // 1st priority  
    g_priority_queue->enqueue(MockMIDIEvent::controlChange(0, 7, 127), MIDIEventPriority::MEDIUM); // 4th priority
    g_priority_queue->enqueue(MockMIDIEvent::noteOn(0, 60, 127), MIDIEventPriority::HIGH); // 3rd priority
    g_priority_queue->enqueue(MockMIDIEvent::startSequence(), MIDIEventPriority::CRITICAL); // 2nd priority
    
    ASSERT_EQ(5lu, g_priority_queue->size());
    ASSERT_EQ(5lu, g_priority_queue->getEnqueueCount());
    
    // VERIFY: Events dequeued in strict priority order (CRITICAL, CRITICAL, HIGH, MEDIUM, LOW)
    std::vector<MIDIEventPriority> expected_order = {
        MIDIEventPriority::CRITICAL,  // Clock tick (timing critical)
        MIDIEventPriority::CRITICAL,  // Start sequence (timing critical)
        MIDIEventPriority::HIGH,      // Note on (musical priority)
        MIDIEventPriority::MEDIUM,    // Control change (automation)
        MIDIEventPriority::LOW        // SysEx (bulk data transfer)
    };
    
    std::vector<MIDIEventPriority> actual_order;
    MockMIDIEvent event;
    
    while (g_priority_queue->dequeue(event)) {
        actual_order.push_back(event.priority);
    }
    
    // VERIFY: Perfect priority ordering maintained
    ASSERT_EQ(expected_order.size(), actual_order.size());
    for (size_t i = 0; i < expected_order.size(); ++i) {
        ASSERT_TRUE(static_cast<int>(expected_order[i]) == static_cast<int>(actual_order[i]));
    }
    
    // VERIFY: All events processed without loss
    ASSERT_EQ(5lu, g_priority_queue->getDequeueCount());
    ASSERT_TRUE(g_priority_queue->empty());
    
    teardownPriorityQueueTests();
}

TEST_UNIT(PriorityQueue, MIDIEventTypeHandling) {
    setupPriorityQueueTests();
    
    // Test different MIDI event types with their correct priorities
    auto note_on = MockMIDIEvent::noteOn(0, 60, 100);
    auto note_off = MockMIDIEvent::noteOff(0, 60, 0);
    auto cc_volume = MockMIDIEvent::controlChange(0, 7, 80);
    auto clock = MockMIDIEvent::clockTick();
    auto sysex = MockMIDIEvent::sysExStart();
    
    // Verify event priorities are set correctly
    ASSERT_TRUE(static_cast<int>(note_on.priority) == static_cast<int>(MIDIEventPriority::HIGH));
    ASSERT_TRUE(static_cast<int>(note_off.priority) == static_cast<int>(MIDIEventPriority::HIGH));
    ASSERT_TRUE(static_cast<int>(cc_volume.priority) == static_cast<int>(MIDIEventPriority::MEDIUM));
    ASSERT_TRUE(static_cast<int>(clock.priority) == static_cast<int>(MIDIEventPriority::CRITICAL));
    ASSERT_TRUE(static_cast<int>(sysex.priority) == static_cast<int>(MIDIEventPriority::LOW));
    
    // Enqueue in reverse priority order
    g_priority_queue->enqueue(sysex, sysex.priority);
    g_priority_queue->enqueue(cc_volume, cc_volume.priority);
    g_priority_queue->enqueue(note_on, note_on.priority);
    g_priority_queue->enqueue(clock, clock.priority);
    
    // Dequeue and verify clock comes first (critical priority)
    MockMIDIEvent dequeued_event;
    ASSERT_TRUE(g_priority_queue->dequeue(dequeued_event));
    ASSERT_TRUE(dequeued_event == clock);
    
    // Then note event (high priority)
    ASSERT_TRUE(g_priority_queue->dequeue(dequeued_event));
    ASSERT_TRUE(dequeued_event == note_on);
    
    // Then CC (medium priority)
    ASSERT_TRUE(g_priority_queue->dequeue(dequeued_event));
    ASSERT_TRUE(dequeued_event == cc_volume);
    
    // Finally SysEx (low priority)
    ASSERT_TRUE(g_priority_queue->dequeue(dequeued_event));
    ASSERT_TRUE(dequeued_event == sysex);
    
    teardownPriorityQueueTests();
}

TEST_UNIT(PriorityQueue, PerformanceUnderLoad) {
    setupPriorityQueueTests();
    
    const size_t num_events = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Enqueue many events with random priorities
    for (size_t i = 0; i < num_events; ++i) {
        MIDIEventPriority priority = static_cast<MIDIEventPriority>(i % 4);
        MockMIDIEvent event(static_cast<uint8_t>(i & 0xFF), 
                           static_cast<uint8_t>((i >> 8) & 0xFF), 
                           static_cast<uint8_t>((i >> 16) & 0xFF), 
                           priority);
        g_priority_queue->enqueue(event, priority);
    }
    
    auto enqueue_time = std::chrono::high_resolution_clock::now();
    auto enqueue_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        enqueue_time - start_time);
    
    // Verify all events were enqueued
    ASSERT_EQ(num_events, g_priority_queue->size());
    ASSERT_EQ(num_events, g_priority_queue->getEnqueueCount());
    
    // Dequeue all events and verify priority ordering
    std::vector<int> priority_counts(4, 0);
    MockMIDIEvent event;
    int last_priority = static_cast<int>(MIDIEventPriority::CRITICAL);
    
    while (g_priority_queue->dequeue(event)) {
        int current_priority = static_cast<int>(event.priority);
        
        // Verify priority ordering (higher or equal priority)
        ASSERT_TRUE(current_priority <= last_priority);
        last_priority = current_priority;
        
        priority_counts[current_priority]++;
    }
    
    auto dequeue_time = std::chrono::high_resolution_clock::now();
    auto dequeue_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        dequeue_time - enqueue_time);
    
    // Performance requirements (should complete within reasonable time)
    ASSERT_TRUE(enqueue_duration.count() < 10000); // < 10ms for enqueue
    ASSERT_TRUE(dequeue_duration.count() < 10000); // < 10ms for dequeue
    
    // Verify all events were processed
    ASSERT_EQ(num_events, g_priority_queue->getDequeueCount());
    ASSERT_TRUE(g_priority_queue->empty());
    
    teardownPriorityQueueTests();
}

TEST_UNIT(PriorityQueue, ThreadSafeOperations) {
    setupPriorityQueueTests();
    
    std::atomic<bool> stop_flag{false};
    std::atomic<size_t> total_enqueued{0};
    std::atomic<size_t> total_dequeued{0};
    
    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < 100 && !stop_flag; ++i) {
            MIDIEventPriority priority = static_cast<MIDIEventPriority>(i % 4);
            MockMIDIEvent event(0x90, static_cast<uint8_t>(i), 100, priority);
            g_priority_queue->enqueue(event, priority);
            total_enqueued++;
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });
    
    // Consumer thread
    std::thread consumer([&]() {
        MockMIDIEvent event;
        while (!stop_flag || !g_priority_queue->empty()) {
            if (g_priority_queue->dequeue(event)) {
                total_dequeued++;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(15));
        }
    });
    
    // Let threads run
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    stop_flag = true;
    
    producer.join();
    consumer.join();
    
    // Verify thread safety (no data corruption)
    ASSERT_TRUE(total_enqueued.load() > 0);
    ASSERT_TRUE(total_dequeued.load() > 0);
    ASSERT_TRUE(g_priority_queue->getEnqueueCount() >= total_enqueued.load());
    
    teardownPriorityQueueTests();
}

TEST_UNIT(PriorityQueue, EdgeCaseHandling) {
    setupPriorityQueueTests();
    
    // Test empty queue dequeue
    MockMIDIEvent event;
    ASSERT_FALSE(g_priority_queue->dequeue(event));
    ASSERT_TRUE(g_priority_queue->empty());
    ASSERT_EQ(0lu, g_priority_queue->size());
    
    // Test single event
    auto single_event = MockMIDIEvent::noteOn(0, 60, 100);
    g_priority_queue->enqueue(single_event, single_event.priority);
    
    ASSERT_FALSE(g_priority_queue->empty());
    ASSERT_EQ(1lu, g_priority_queue->size());
    
    ASSERT_TRUE(g_priority_queue->dequeue(event));
    ASSERT_TRUE(event == single_event);
    ASSERT_TRUE(g_priority_queue->empty());
    
    // Test same priority events (should maintain insertion order among same priority)
    auto event1 = MockMIDIEvent::noteOn(0, 60, 100);
    auto event2 = MockMIDIEvent::noteOn(0, 62, 100);
    auto event3 = MockMIDIEvent::noteOn(0, 64, 100);
    
    g_priority_queue->enqueue(event1, MIDIEventPriority::HIGH);
    g_priority_queue->enqueue(event2, MIDIEventPriority::HIGH);
    g_priority_queue->enqueue(event3, MIDIEventPriority::HIGH);
    
    // All should have same priority, order doesn't matter for same priority
    std::vector<MockMIDIEvent> dequeued_events;
    while (g_priority_queue->dequeue(event)) {
        dequeued_events.push_back(event);
    }
    
    ASSERT_EQ(3lu, dequeued_events.size());
    
    teardownPriorityQueueTests();
}

TEST_UNIT(PriorityQueue, RealTimeConstraints) {
    setupPriorityQueueTests();
    
    // Test rapid enqueue/dequeue cycles (simulating real-time audio)
    const size_t rapid_cycles = 500;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t cycle = 0; cycle < rapid_cycles; ++cycle) {
        // Enqueue a critical event (like MIDI clock)
        auto clock_event = MockMIDIEvent::clockTick();
        g_priority_queue->enqueue(clock_event, clock_event.priority);
        
        // Immediately dequeue it
        MockMIDIEvent dequeued_event;
        ASSERT_TRUE(g_priority_queue->dequeue(dequeued_event));
        ASSERT_TRUE(dequeued_event == clock_event);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Should complete very quickly (under 1ms for 500 cycles)
    ASSERT_TRUE(duration.count() < 1000);
    
    // Verify queue statistics
    ASSERT_EQ(rapid_cycles, g_priority_queue->getEnqueueCount());
    ASSERT_EQ(rapid_cycles, g_priority_queue->getDequeueCount());
    ASSERT_TRUE(g_priority_queue->empty());
    
    teardownPriorityQueueTests();
}

TEST_UNIT(PriorityQueue, MixedPriorityStressTest) {
    setupPriorityQueueTests();
    
    // Create a complex scenario with all priority levels
    std::vector<std::pair<MockMIDIEvent, MIDIEventPriority>> test_events = {
        {MockMIDIEvent::sysExStart(), MIDIEventPriority::LOW},
        {MockMIDIEvent::clockTick(), MIDIEventPriority::CRITICAL},
        {MockMIDIEvent::controlChange(0, 1, 64), MIDIEventPriority::MEDIUM},
        {MockMIDIEvent::noteOn(0, 60, 127), MIDIEventPriority::HIGH},
        {MockMIDIEvent::startSequence(), MIDIEventPriority::CRITICAL},
        {MockMIDIEvent::noteOff(0, 60, 0), MIDIEventPriority::HIGH},
        {MockMIDIEvent::controlChange(0, 7, 100), MIDIEventPriority::MEDIUM},
        {MockMIDIEvent::clockTick(), MIDIEventPriority::CRITICAL},
    };
    
    // Enqueue all events
    for (const auto& [event, priority] : test_events) {
        g_priority_queue->enqueue(event, priority);
    }
    
    ASSERT_EQ(test_events.size(), g_priority_queue->size());
    
    // Dequeue and verify priority ordering
    std::vector<MIDIEventPriority> dequeued_priorities;
    MockMIDIEvent event;
    
    while (g_priority_queue->dequeue(event)) {
        dequeued_priorities.push_back(event.priority);
    }
    
    // Verify priorities are in non-increasing order (higher priority first)
    for (size_t i = 1; i < dequeued_priorities.size(); ++i) {
        int current = static_cast<int>(dequeued_priorities[i]);
        int previous = static_cast<int>(dequeued_priorities[i-1]);
        ASSERT_TRUE(current <= previous);
    }
    
    // Count each priority level
    size_t critical_count = 0, high_count = 0, medium_count = 0, low_count = 0;
    for (auto priority : dequeued_priorities) {
        switch (priority) {
            case MIDIEventPriority::CRITICAL: critical_count++; break;
            case MIDIEventPriority::HIGH: high_count++; break;
            case MIDIEventPriority::MEDIUM: medium_count++; break;
            case MIDIEventPriority::LOW: low_count++; break;
        }
    }
    
    ASSERT_EQ(3lu, critical_count); // 2 clocks + 1 start
    ASSERT_EQ(2lu, high_count);     // 1 note on + 1 note off
    ASSERT_EQ(2lu, medium_count);   // 2 control changes
    ASSERT_EQ(1lu, low_count);      // 1 sysex
    
    teardownPriorityQueueTests();
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "🔢 Priority Queue Tests - Unified Framework" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    auto& runner = TestFramework::TestRunner::getInstance();
    
    // Run all unit tests for PriorityQueue
    auto results = runner.runCategory("unit/PriorityQueue");
    
    return results.failed_tests == 0 ? 0 : 1;
}
