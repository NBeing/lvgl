/**
 * @brief Simple Event Tracer Test (No LVGL Dependencies)
 * 
 * Tests just the RT-safe event tracing functionality without
 * the visual components that require LVGL integration.
 */

#include "TestFramework.h"

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)

#include "components/debug/RTEventTracer.h"
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
    
    void simulateBPMChange(float bpm) {
        if (bpm_changed_callback_) bpm_changed_callback_(bpm);
    }
};

/**
 * @brief Simple Event Tracer Test Suite
 */
class SimpleEventTracerTests {
private:
    RTEventTracer* tracer_;
    MockMidiClockManager mock_clock_manager_;
    
public:
    void setUp() {
        tracer_ = &RTEventTracer::getInstance();
        // Reset any existing state
    }
    
    void tearDown() {
        // Clean up
    }
    
    void runAllTests() {
        std::cout << "🎵 Simple Event Tracer Tests (No LVGL)" << std::endl;
        
        setUp();
        
        testBasicEventTracing();
        testHighFrequencyEvents();
        testThreadSafety();
        
        tearDown();
        
        TestFramework::getInstance().printSummary();
        std::cout << "✅ Simple Event Tracer Tests Completed" << std::endl;
    }
    
    void testBasicEventTracing() {
        TEST_SUITE("Basic Event Tracing");
        
        TEST("RTEventTracer captures events using macros") {
            // Test the basic macro functionality
            TRACE_CLOCK_EVENT("MidiClockManager", "ClockTab", "onClockTick", "144");
            TRACE_UI_EVENT("ClockTab", "TransportControl", "updateTransportState", "1");
            TRACE_MIDI_EVENT("MidiHandler", "ExternalSynth", "sendNoteOn", "60");
            
            // Verify events can be retrieved
            RTEventTracer::EventTrace trace;
            bool found_clock_event = false;
            bool found_ui_event = false;
            bool found_midi_event = false;
            
            // Process several events
            for (int i = 0; i < 10 && tracer_->popTrace(trace); ++i) {
                if (strcmp(trace.source_name, "MidiClockManager") == 0) {
                    found_clock_event = true;
                }
                if (strcmp(trace.source_name, "ClockTab") == 0) {
                    found_ui_event = true;
                }
                if (strcmp(trace.source_name, "MidiHandler") == 0) {
                    found_midi_event = true;
                }
            }
            
            ASSERT_TRUE(found_clock_event);
            ASSERT_TRUE(found_ui_event);
            ASSERT_TRUE(found_midi_event);
            
        } END_TEST();
        
        TEST("Event data integrity") {
            // Trace a specific event with known data
            TRACE_PARAMETER_EVENT("ParameterManager", "MidiBridge", "parameterChanged", "1001:0.75");
            
            // Read it back and verify
            RTEventTracer::EventTrace trace;
            ASSERT_TRUE(tracer_->popTrace(trace));
            
            ASSERT_TRUE(strcmp(trace.source_name, "ParameterManager") == 0);
            ASSERT_TRUE(strcmp(trace.target_name, "MidiBridge") == 0);
            ASSERT_TRUE(strcmp(trace.event_name, "parameterChanged") == 0);
            ASSERT_TRUE(strcmp(trace.event_data, "1001:0.75") == 0);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    void testHighFrequencyEvents() {
        TEST_SUITE("High Frequency Event Handling");
        
        TEST("System handles rapid event generation") {
            const int num_events = 100;
            
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // Generate events rapidly
            for (int i = 0; i < num_events; ++i) {
                std::string data = std::to_string(i);
                TRACE_CLOCK_EVENT("TestSource", "TestTarget", "rapidEvent", data.c_str());
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            
            // Should complete in reasonable time (under 10ms for 100 events)
            ASSERT_TRUE(duration.count() < 10000);
            
            // Verify we can read back some events
            RTEventTracer::EventTrace trace;
            int events_read = 0;
            while (tracer_->popTrace(trace) && events_read < 50) {
                events_read++;
            }
            
            ASSERT_TRUE(events_read > 0);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    void testThreadSafety() {
        TEST_SUITE("Thread Safety");
        
        TEST("Multiple threads can trace events safely") {
            const int num_threads = 2;
            const int events_per_thread = 50;
            std::atomic<int> completed_threads{0};
            
            std::vector<std::thread> threads;
            
            // Start threads that trace events
            for (int t = 0; t < num_threads; ++t) {
                threads.emplace_back([this, t, events_per_thread, &completed_threads]() {
                    for (int i = 0; i < events_per_thread; ++i) {
                        std::string source = "Thread" + std::to_string(t);
                        std::string data = std::to_string(i);
                        TRACE_UI_EVENT(source.c_str(), "TestTarget", "threadEvent", data.c_str());
                        
                        // Small delay
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                    completed_threads++;
                });
            }
            
            // Wait for completion
            for (auto& t : threads) {
                t.join();
            }
            
            // Verify all threads completed
            ASSERT_EQ(num_threads, completed_threads.load());
            
            // Verify we can read events
            RTEventTracer::EventTrace trace;
            int events_read = 0;
            while (tracer_->popTrace(trace) && events_read < 20) {
                events_read++;
            }
            
            ASSERT_TRUE(events_read > 0);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
};

// Test runner function
void runSimpleEventTracerTests() {
    SimpleEventTracerTests test_suite;
    test_suite.runAllTests();
}

#else

// Stub for non-desktop builds
void runSimpleEventTracerTests() {
    std::cout << "⚠️ Simple Event Tracer Tests skipped - requires DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER" << std::endl;
}

#endif // DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER
