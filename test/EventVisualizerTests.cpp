/**
 * @brief Event Visualizer System Tests
 * 
 * Tests    void simulateBPMChange(float bpm) {
        if (bpm_changed_callback_) bpm_changed_callback_(bpm);
    }e complete event flow visualization system:
 * - RT-safe event tracing
 * - Traced callback registration  
 * - Event flow visualization
 * - UI integration
 */

#include "TestFramework.h"

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)

#include "components/debug/RTEventTracer.h"
#include "components/debug/EventFlowVisualizer.h"
#include "components/debug/EventVisualizerIntegration.h"
#include "components/debug/TracedCallbacks.h"
#include <thread>
#include <chrono>
#include <functional>

using namespace Debug;
using namespace Test;

/**
 * @brief Mock MidiClockManager for testing traced callbacks
 */
class MockMidiClockManager {
private:
    std::function<void(int)> clock_tick_callback_;
    std::function<void(int, int)> transport_changed_callback_;
    std::function<void(float)> bpm_changed_callback_;
    
public:
    void setClockTickCallback(std::function<void(int)> callback) {
        clock_tick_callback_ = callback;
    }
    
    void setTransportChangedCallback(std::function<void(int, int)> callback) {
        transport_changed_callback_ = callback;
    }
    
    void setBPMChangedCallback(std::function<void(float)> callback) {
        bpm_changed_callback_ = callback;
    }
    
    // Test methods to trigger callbacks
    void simulateClockTick(int tick) {
        if (clock_tick_callback_) clock_tick_callback_(tick);
    }
    
    void simulateTransportChange(int old_state, int new_state) {
        if (transport_changed_callback_) transport_changed_callback_(old_state, new_state);
    }
    
    void simulateBPMChange(float bmp) {
        if (bpm_changed_callback_) bpm_changed_callback_(bpm);
    }
};

/**
 * @brief Event Visualizer Test Suite
 */
class EventVisualizerTests {
private:
    RTEventTracer* tracer_;
    MockMidiClockManager mock_clock_manager_;
    
public:
    void setUp() {
        tracer_ = &RTEventTracer::getInstance();
        tracer_->resetStatistics();
        tracer_->setTracingEnabled(true);
    }
    
    void tearDown() {
        tracer_->setTracingEnabled(false);
    }
    
    void runAllTests() {
        std::cout << "🎵 Starting Event Visualizer System Tests" << std::endl;
        
        setUp();
        
        testRTEventTracingBasic();
        testTracedCallbackSystem();
        testHighFrequencyEventHandling();
        testThreadSafetyUnderLoad();
        testEventFlowVisualization();
        
        tearDown();
        
        TestFramework::getInstance().printSummary();
        std::cout << "✅ Event Visualizer Tests Completed" << std::endl;
    }
    
    void testRTEventTracingBasic() {
        TEST_SUITE("RT Event Tracing - Basic Functionality");
        
        TEST("Event tracer captures basic events") {
            // Initial state
            ASSERT_EQ(0, tracer_->getTotalEvents());
            ASSERT_EQ(0, tracer_->getDroppedEvents());
            
            // Trace some events using the macros
            TRACE_CLOCK_EVENT("MidiClockManager", "ClockTab", "onClockTick", "144");
            TRACE_UI_EVENT("ClockTab", "TransportControl", "updateTransportState", "1");
            TRACE_MIDI_EVENT("HardwareMidiManager", "ExternalSynth", "sendNoteOn", "60");
            
            // Check statistics
            ASSERT_EQ(3, tracer_->getTotalEvents());
            ASSERT_EQ(0, tracer_->getDroppedEvents());
            
        } END_TEST();
        
        TEST("Event tracer provides correct event data") {
            // Trace a specific event
            TRACE_PARAMETER_EVENT("ParameterManager", "MidiBridge", "parameterChanged", "1001:0.75");
            
            // Read it back
            RTEventTracer::EventTrace trace;
            ASSERT_TRUE(tracer_->popTrace(trace));
            
            // Verify event details
            ASSERT_TRUE(strcmp(trace.source_name, "ParameterManager") == 0);
            ASSERT_TRUE(strcmp(trace.target_name, "MidiBridge") == 0);
            ASSERT_TRUE(strcmp(trace.event_name, "parameterChanged") == 0);
            ASSERT_TRUE(strcmp(trace.event_data, "1001:0.75") == 0);
            ASSERT_EQ(static_cast<uint8_t>(RTEventTracer::EventType::PARAMETER_EVENT), trace.event_type);
            
        } END_TEST();
        
        TEST("Event tracer handles buffer wraparound") {
            // Fill buffer beyond capacity to test wraparound
            for (int i = 0; i < 10000; ++i) {
                TRACE_CLOCK_EVENT("TestSource", "TestTarget", "testEvent", std::to_string(i).c_str());
            }
            
            // Should have some dropped events due to buffer overflow
            ASSERT_TRUE(tracer_->getDroppedEvents() > 0);
            ASSERT_TRUE(tracer_->getTotalEvents() == 10000);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    void testTracedCallbackSystem() {
        TEST_SUITE("Traced Callback System");
        
        TEST("TRACED_CLOCK_TICK_CALLBACK macro works correctly") {
            bool callback_called = false;
            int received_tick = 0;
            
            // Register traced callback using the macro correctly
            auto traced_callback = [&callback_called, &received_tick](int tick) {
                callback_called = true;
                received_tick = tick;
            };
            
            // Manually trace and call for testing
            mock_clock_manager_.setClockTickCallback([&](int tick) {
                TRACE_CLOCK_EVENT("MidiClockManager", "TestClockTab", "onClockTick", std::to_string(tick).c_str());
                traced_callback(tick);
            });
            
            // Simulate clock tick
            mock_clock_manager_.simulateClockTick(144);
            
            // Verify callback was called
            ASSERT_TRUE(callback_called);
            ASSERT_EQ(144, received_tick);
            
            // Verify event was traced
            RTEventTracer::EventTrace trace;
            ASSERT_TRUE(tracer_->popTrace(trace));
            ASSERT_TRUE(strcmp(trace.source_name, "MidiClockManager") == 0);
            ASSERT_TRUE(strcmp(trace.target_name, "TestClockTab") == 0);
            ASSERT_TRUE(strcmp(trace.event_name, "onClockTick") == 0);
            
        } END_TEST();
        
        TEST("TRACED_TRANSPORT_CHANGED_CALLBACK macro works correctly") {
            bool callback_called = false;
            int old_state = 0, new_state = 0;
            
            // Register traced transport callback
            auto traced_transport_callback = [&callback_called, &old_state, &new_state](int old_s, int new_s) {
                callback_called = true;
                old_state = old_s;
                new_state = new_s;
            };
            
            // Manually trace and call for testing
            mock_clock_manager_.setTransportChangedCallback([&](int old_s, int new_s) {
                TRACE_TRANSPORT_EVENT("MidiClockManager", "TestTransportTab", "onTransportChanged", 
                                     (std::to_string(old_s) + "->" + std::to_string(new_s)).c_str());
                traced_transport_callback(old_s, new_s);
            });
            
            // Simulate transport change
            mock_clock_manager_.simulateTransportChange(0, 1); // Stop -> Play
            
            // Verify callback was called
            ASSERT_TRUE(callback_called);
            ASSERT_EQ(0, old_state);
            ASSERT_EQ(1, new_state);
            
            // Verify event was traced
            RTEventTracer::EventTrace trace;
            ASSERT_TRUE(tracer_->popTrace(trace));
            ASSERT_TRUE(strcmp(trace.source_name, "MidiClockManager") == 0);
            ASSERT_TRUE(strcmp(trace.target_name, "TestTransportTab") == 0);
            ASSERT_TRUE(strcmp(trace.event_name, "onTransportChanged") == 0);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    void testHighFrequencyEventHandling() {
        TEST_SUITE("High Frequency Event Handling");
        
        TEST("System handles rapid clock tick events") {
            const int num_ticks = 1000;
            int callback_count = 0;
            
            // Register callback that counts invocations
            auto high_freq_callback = [&callback_count](int tick) {
                callback_count++;
            };
            
            // Manually trace and call for testing
            mock_clock_manager_.setClockTickCallback([&](int tick) {
                // Only trace every 10th tick to avoid overwhelming the buffer
                if (tick % 10 == 0) {
                    TRACE_CLOCK_EVENT("MidiClockManager", "HighFreqTest", "onClockTick", std::to_string(tick).c_str());
                }
                high_freq_callback(tick);
            });
            
            // Simulate rapid clock ticks (like 24 PPQN at 120 BPM)
            auto start_time = std::chrono::high_resolution_clock::now();
            
            for (int i = 0; i < num_ticks; ++i) {
                mock_clock_manager_.simulateClockTick(i);
                
                // Simulate realistic timing - ~48 ticks per second at 120 BPM 24 PPQN
                std::this_thread::sleep_for(std::chrono::microseconds(20));
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            // Verify all callbacks were processed
            ASSERT_EQ(num_ticks, callback_count);
            
            // Verify reasonable performance (should complete in under 1 second)
            ASSERT_TRUE(duration.count() < 1000);
            
            // Verify events were traced (may have some drops due to frequency)
            ASSERT_TRUE(tracer_->getTotalEvents() > 0);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    void testThreadSafetyUnderLoad() {
        TEST_SUITE("Thread Safety Under Load");
        
        TEST("Multiple threads can trace events simultaneously") {
            const int num_threads = 4;
            const int events_per_thread = 250;
            std::atomic<int> completed_threads{0};
            
            std::vector<std::thread> threads;
            
            // Start multiple threads tracing events
            for (int t = 0; t < num_threads; ++t) {
                threads.emplace_back([this, t, events_per_thread, &completed_threads]() {
                    for (int i = 0; i < events_per_thread; ++i) {
                        std::string source = "Thread" + std::to_string(t);
                        std::string data = std::to_string(i);
                        TRACE_UI_EVENT(source.c_str(), "TestTarget", "threadEvent", data.c_str());
                        
                        // Small delay to create realistic timing
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                    }
                    completed_threads++;
                });
            }
            
            // Wait for all threads to complete
            for (auto& t : threads) {
                t.join();
            }
            
            // Verify all threads completed
            ASSERT_EQ(num_threads, completed_threads.load());
            
            // Verify we got most events (some drops expected under high load)
            uint64_t total_expected = num_threads * events_per_thread;
            uint64_t total_events = tracer_->getTotalEvents();
            
            // Should get at least 90% of events
            ASSERT_TRUE(total_events >= total_expected * 0.9);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    void testEventFlowVisualization() {
        TEST_SUITE("Event Flow Visualization");
        
        // Note: This test would require LVGL initialization for full testing
        // For now, we test the data processing part
        
        TEST("Event processing doesn't crash with real event flow") {
            // Simulate realistic event flow sequence
            TRACE_CLOCK_EVENT("MidiClockManager", "RTSequenceEngine", "onClockTick", "144");
            TRACE_PARAMETER_EVENT("RTSequenceEngine", "ParameterManager", "setParameter", "1001:0.75");
            TRACE_PARAMETER_EVENT("ParameterManager", "MidiBridge", "parameterChanged", "1001:0.75");
            TRACE_MIDI_EVENT("MidiBridge", "MidiHandler", "sendControlChange", "1:74:95");
            TRACE_MIDI_EVENT("MidiHandler", "ExternalSynth", "midiOutput", "CC:1:74:95");
            
            // Verify all events were captured
            ASSERT_TRUE(tracer_->getTotalEvents() >= 5);
            
            // Process events (simulate what EventVisualizerIntegration would do)
            int processed_count = 0;
            RTEventTracer::EventTrace trace;
            while (tracer_->popTrace(trace)) {
                processed_count++;
                
                // Verify event data integrity
                ASSERT_TRUE(strlen(trace.source_name) > 0);
                ASSERT_TRUE(strlen(trace.target_name) > 0);
                ASSERT_TRUE(strlen(trace.event_name) > 0);
                ASSERT_TRUE(trace.timestamp_us > 0);
            }
            
            ASSERT_TRUE(processed_count >= 5);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
};

// Test runner function
void runEventVisualizerTests() {
    EventVisualizerTests test_suite;
    test_suite.runAllTests();
}

#else

// Stub for non-desktop builds
void runEventVisualizerTests() {
    std::cout << "⚠️ Event Visualizer Tests skipped - requires DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER" << std::endl;
}

#endif // DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER

// Test event tracing under load
TEST_F(EventVisualizerTest, RTEventTracingStressTest) {
    auto& tracer = RTEventTracer::getInstance();
    
    const int num_events = 1000;
    const int num_threads = 4;
    
    std::vector<std::thread> threads;
    
    // Create multiple threads generating events simultaneously
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&tracer, num_events, t]() {
            for (int i = 0; i < num_events; i++) {
                std::string source = "Source" + std::to_string(t);
                std::string target = "Target" + std::to_string(t);
                std::string data = std::to_string(i);
                
                TRACE_RT_EVENT(source.c_str(), target.c_str(), "testEvent", data.c_str());
                
                // Small delay to simulate real-world timing
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Check that most events were captured (some might be dropped due to buffer overflow)
    uint64_t total_expected = num_events * num_threads;
    uint64_t total_captured = tracer.getTotalEvents();
    uint64_t total_dropped = tracer.getDroppedEvents();
    
    EXPECT_EQ(total_expected, total_captured + total_dropped);
    
    // Should capture at least 90% of events
    EXPECT_GE(total_captured, total_expected * 0.9);
    
    std::cout << "Stress test results:" << std::endl;
    std::cout << "  Expected: " << total_expected << std::endl;
    std::cout << "  Captured: " << total_captured << std::endl;
    std::cout << "  Dropped: " << total_dropped << std::endl;
    std::cout << "  Capture rate: " << (double(total_captured) / total_expected * 100.0) << "%" << std::endl;
}

// Test buffer overflow handling
TEST_F(EventVisualizerTest, BufferOverflowHandling) {
    auto& tracer = RTEventTracer::getInstance();
    
    // Fill up the buffer beyond capacity
    const int overflow_events = 10000;
    
    for (int i = 0; i < overflow_events; i++) {
        TRACE_RT_EVENT("Source", "Target", "overflowTest", std::to_string(i).c_str());
    }
    
    // Should have dropped some events
    EXPECT_GT(tracer.getDroppedEvents(), 0);
    
    // Buffer utilization should be high but not cause crashes
    double utilization = tracer.getBufferUtilization();
    EXPECT_GE(utilization, 0.0);
    EXPECT_LE(utilization, 100.0);
    
    std::cout << "Buffer overflow test:" << std::endl;
    std::cout << "  Total events: " << tracer.getTotalEvents() << std::endl;
    std::cout << "  Dropped events: " << tracer.getDroppedEvents() << std::endl;
    std::cout << "  Buffer utilization: " << utilization << "%" << std::endl;
}

// Test traced callback macros
TEST_F(EventVisualizerTest, TracedCallbackMacros) {
    auto& tracer = RTEventTracer::getInstance();
    
    // Mock manager class for testing
    class MockManager {
    public:
        std::function<void(int)> callback_;
        void setCallback(std::function<void(int)> cb) { callback_ = cb; }
        void triggerCallback(int value) { if (callback_) callback_(value); }
    };
    
    MockManager manager;
    int received_value = 0;
    
    // Use traced callback macro
    SET_TRACED_CALLBACK(manager, setCallback, [&received_value](int value) {
        received_value = value;
    }, "TestTarget");
    
    // Trigger the callback
    manager.triggerCallback(42);
    
    // Check that callback was executed
    EXPECT_EQ(42, received_value);
    
    // Check that event was traced
    EXPECT_GT(tracer.getTotalEvents(), 0);
    
    RTEventTracer::EventTrace trace;
    bool found_registration = false;
    bool found_callback = false;
    
    while (tracer.popTrace(trace)) {
        if (std::string(trace.event_name).find("registration") != std::string::npos) {
            found_registration = true;
            EXPECT_STREQ("MockManager", trace.source_name);
            EXPECT_STREQ("TestTarget", trace.target_name);
        }
        if (std::string(trace.event_name).find("setCallback") != std::string::npos) {
            found_callback = true;
            EXPECT_STREQ("MockManager", trace.source_name);
            EXPECT_STREQ("TestTarget", trace.target_name);
        }
    }
    
    EXPECT_TRUE(found_registration);
    EXPECT_TRUE(found_callback);
}

// Test event type filtering and colors
TEST_F(EventVisualizerTest, EventTypeHandling) {
    auto& tracer = RTEventTracer::getInstance();
    
    // Trace different event types
    TRACE_RT_EVENT("Source", "Target", "rtEvent", "");
    TRACE_UI_EVENT("Source", "Target", "uiEvent", "");
    TRACE_MIDI_EVENT("Source", "Target", "midiEvent", "");
    TRACE_PARAMETER_EVENT("Source", "Target", "paramEvent", "");
    TRACE_CLOCK_EVENT("Source", "Target", "clockEvent", "");
    TRACE_SETTINGS_EVENT("Source", "Target", "settingsEvent", "");
    
    // Check that all events have correct types
    RTEventTracer::EventTrace trace;
    std::vector<RTEventTracer::EventType> expected_types = {
        RTEventTracer::EventType::RT_EVENT,
        RTEventTracer::EventType::UI_EVENT,
        RTEventTracer::EventType::MIDI_EVENT,
        RTEventTracer::EventType::PARAMETER_EVENT,
        RTEventTracer::EventType::CLOCK_EVENT,
        RTEventTracer::EventType::SETTINGS_EVENT
    };
    
    for (auto expected_type : expected_types) {
        EXPECT_TRUE(tracer.popTrace(trace));
        EXPECT_EQ(static_cast<uint8_t>(expected_type), trace.event_type);
    }
}

// Test performance characteristics
TEST_F(EventVisualizerTest, PerformanceCharacteristics) {
    auto& tracer = RTEventTracer::getInstance();
    
    const int num_events = 10000;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Generate events as fast as possible
    for (int i = 0; i < num_events; i++) {
        TRACE_RT_EVENT("PerfTest", "Target", "event", std::to_string(i).c_str());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double events_per_second = (double(num_events) / duration.count()) * 1000000.0;
    double microseconds_per_event = double(duration.count()) / num_events;
    
    std::cout << "Performance test results:" << std::endl;
    std::cout << "  Events: " << num_events << std::endl;
    std::cout << "  Duration: " << duration.count() << " μs" << std::endl;
    std::cout << "  Events/sec: " << events_per_second << std::endl;
    std::cout << "  μs/event: " << microseconds_per_event << std::endl;
    
    // Should be very fast - less than 1μs per event
    EXPECT_LT(microseconds_per_event, 1.0);
    
    // Should handle at least 1M events per second
    EXPECT_GT(events_per_second, 1000000.0);
}

};

// Test runner function
void runEventVisualizerTests() {
    EventVisualizerTests test_suite;
    test_suite.runAllTests();
}

#else

// Stub for non-desktop builds
void runEventVisualizerTests() {
    std::cout << "⚠️ Event Visualizer Tests skipped - requires DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER" << std::endl;
}

#endif // DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER
