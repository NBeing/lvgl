/**
 * @brief Simplified RT-Safe Event Distributor Tests
 * 
 * Focused tests with proper isolation to validate the RT-safe system
 */

#include "TestFramework.h"
#include "components/threading/RTSafeEventDistributor.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <mutex>

using namespace RTSafe;
using namespace Test;

// Helper function to convert EventType to string for testing
std::string eventTypeToString(EventType type) {
    switch (type) {
        case EventType::MIDI_INPUT: return "MIDI_INPUT";
        case EventType::MIDI_OUTPUT: return "MIDI_OUTPUT";
        case EventType::PARAMETER_CHANGE: return "PARAMETER_CHANGE";
        case EventType::CLOCK_TICK: return "CLOCK_TICK";
        case EventType::CONTROL_CHANGE: return "CONTROL_CHANGE";
        default: return "UNKNOWN";
    }
}

// Custom ASSERT_EQ for EventType
#define ASSERT_EQ_EVENT_TYPE(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            throw std::runtime_error("ASSERT_EQ failed: expected " + eventTypeToString(expected) + \
                                   ", got " + eventTypeToString(actual)); \
        } \
    } while(0)

/**
 * @brief Test RT Observer that tracks calls and timing
 */
class TestRTObserver : public RTObserver {
private:
    std::atomic<int> call_count_{0};
    std::atomic<uint32_t> last_timestamp_{0};
    int priority_;
    
public:
    TestRTObserver(int priority = 0) : priority_(priority) {}
    
    void handleRTEvent(const RTEvent& event) override {
        call_count_++;
        last_timestamp_ = event.timestamp_us;
        
        // Simulate minimal RT processing
        volatile int dummy = 0;
        for (int i = 0; i < 10; ++i) {
            dummy += i;
        }
    }
    
    int getPriority() const override { return priority_; }
    
    int getCallCount() const { return call_count_.load(); }
    uint32_t getLastTimestamp() const { return last_timestamp_.load(); }
    
    void reset() {
        call_count_ = 0;
        last_timestamp_ = 0;
    }
};

/**
 * @brief Test UI Observer that tracks calls
 */
class TestUIObserver : public UIObserver {
private:
    std::atomic<int> call_count_{0};
    std::vector<RTEvent> received_events_;
    mutable std::mutex events_mutex_;
    
public:
    void handleUIEvent(const RTEvent& event) override {
        call_count_++;
        
        std::lock_guard<std::mutex> lock(events_mutex_);
        received_events_.push_back(event);
    }
    
    int getCallCount() const { return call_count_.load(); }
    
    std::vector<RTEvent> getReceivedEvents() const {
        std::lock_guard<std::mutex> lock(events_mutex_);
        return received_events_;
    }
    
    void reset() {
        call_count_ = 0;
        std::lock_guard<std::mutex> lock(events_mutex_);
        received_events_.clear();
    }
};

void runRTSafeEventDistributorTests() {
    std::cout << "🧵 Starting RT-Safe Event Distributor Tests" << std::endl;
    
    // Test 1: Basic RT Event Distribution
    {
        TEST_SUITE("Basic RT Event Distribution");
        
        RTSafeEventDistributor distributor;
        TestRTObserver rt_observer_high(10);
        TestRTObserver rt_observer_low(1);
        TestUIObserver ui_observer;
        
        distributor.initialize();
        distributor.addRTObserver(&rt_observer_high);
        distributor.addRTObserver(&rt_observer_low);
        distributor.addUIObserver(&ui_observer);
        
        TEST("RT observers called immediately") {
            RTEvent test_event = RTEvent::midiCC(1, 74, 100);
            
            distributor.notifyRTObservers(test_event);
            
            // RT observers should be called immediately
            ASSERT_EQ(1, rt_observer_high.getCallCount());
            ASSERT_EQ(1, rt_observer_low.getCallCount());
            
            // UI observer not called yet (needs processing)
            ASSERT_EQ(0, ui_observer.getCallCount());
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 2: UI Event Processing
    {
        TEST_SUITE("UI Event Processing");
        
        RTSafeEventDistributor distributor;
        TestRTObserver rt_observer;
        TestUIObserver ui_observer;
        
        distributor.initialize();
        distributor.addRTObserver(&rt_observer);
        distributor.addUIObserver(&ui_observer);
        
        TEST("UI events processed correctly") {
            RTEvent test_event = RTEvent::parameterChange(42, 85);
            
            // Send RT event
            distributor.notifyRTObservers(test_event);
            
            // Process UI events
            distributor.processUIEvents();
            
            // Now UI observer should be called
            ASSERT_EQ(1, ui_observer.getCallCount());
            
            auto received_events = ui_observer.getReceivedEvents();
            ASSERT_EQ(1, received_events.size());
            ASSERT_EQ_EVENT_TYPE(EventType::PARAMETER_CHANGE, received_events[0].type);
            ASSERT_EQ(42, received_events[0].data1);
            ASSERT_EQ(85, received_events[0].data2);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 3: RT Timing Constraints
    {
        TEST_SUITE("RT Timing Constraints");
        
        RTSafeEventDistributor distributor;
        TestRTObserver rt_observer;
        
        distributor.initialize();
        distributor.addRTObserver(&rt_observer);
        
        RT_TEST("RT timing validation") {
            // Test multiple events in sequence
            for (int i = 0; i < 10; ++i) {
                RTEvent event = RTEvent::midiCC(1, 74, i);
                
                ASSERT_RT_TIMING({
                    distributor.notifyRTObservers(event);
                }, 200); // 200μs max (more lenient for test environment)
            }
            
            // Verify all events were processed
            ASSERT_EQ(10, rt_observer.getCallCount());
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 4: Event Types and Factory Methods
    {
        TEST_SUITE("Event Types and Factory Methods");
        
        TEST("Event factory methods") {
            auto midi_cc = RTEvent::midiCC(5, 74, 100);
            ASSERT_EQ_EVENT_TYPE(EventType::CONTROL_CHANGE, midi_cc.type);
            ASSERT_EQ(5, midi_cc.channel);
            ASSERT_EQ(74, midi_cc.data1);
            ASSERT_EQ(100, midi_cc.data2);
            
            auto param_change = RTEvent::parameterChange(42, 85);
            ASSERT_EQ_EVENT_TYPE(EventType::PARAMETER_CHANGE, param_change.type);
            ASSERT_EQ(42, param_change.data1);
            ASSERT_EQ(85, param_change.data2);
            
            auto clock_tick = RTEvent::clockTick();
            ASSERT_EQ_EVENT_TYPE(EventType::CLOCK_TICK, clock_tick.type);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 5: Statistics Tracking
    {
        TEST_SUITE("Statistics Tracking");
        
        RTSafeEventDistributor distributor;
        TestRTObserver rt_observer;
        TestUIObserver ui_observer;
        
        distributor.initialize();
        distributor.addRTObserver(&rt_observer);
        distributor.addUIObserver(&ui_observer);
        
        TEST("Statistics tracking") {
            // Reset statistics first
            distributor.resetStatistics();
            
            // Send some events
            for (int i = 0; i < 20; ++i) {
                RTEvent event = RTEvent::midiCC(1, 74, i);
                distributor.notifyRTObservers(event);
            }
            
            // Process UI events
            distributor.processUIEvents();
            
            auto stats = distributor.getStatistics();
            
            // Verify statistics
            ASSERT_EQ(20, stats.rt_events_processed);
            ASSERT_EQ(20, stats.ui_events_processed);
            ASSERT_TRUE(stats.max_rt_processing_time_us < 10000); // < 10ms
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 6: Bidirectional Communication
    {
        TEST_SUITE("Bidirectional Communication");
        
        RTSafeEventDistributor distributor;
        TestRTObserver rt_observer;
        
        distributor.initialize();
        distributor.addRTObserver(&rt_observer);
        
        TEST("UI to RT thread communication") {
            // Send event from UI to RT thread
            RTEvent ui_event = RTEvent::parameterChange(123, 64);
            bool sent = distributor.sendToRTThread(ui_event);
            ASSERT_TRUE(sent);
            
            // Process on RT side
            distributor.processRTEvents();
            
            // RT observer should be notified
            ASSERT_EQ(1, rt_observer.getCallCount());
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    TestFramework::getInstance().printSummary();
    std::cout << "✅ RT-Safe Event Distributor Tests Completed" << std::endl;
}
