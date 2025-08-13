/**
 * @brief Event Visualizer Test Run    void setBPMChangedCallback(std::function<void(float)> callback) {
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
    } the RT-safe event tracing and traced callback system
 * for the event flow visualizer.
 */

#include <iostream>
#include "TestFramework.h"

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
#include "components/debug/RTEventTracer.h"
#include "components/debug/TracedCallbacks.h"
#endif

#include <thread>
#include <chrono>
#include <functional>

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
 * @brief Event Visualizer Test Suite
 */
class EventVisualizerTests {
private:
    MockMidiClockManager mock_clock_manager_;
    
public:
    void runAllTests() {
        std::cout << "🎵 Starting Event Visualizer System Tests" << std::endl;
        
        #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
        
        testRTEventTracingBasic();
        testTracedCallbackSystem();
        testHighFrequencyEventHandling();
        
        #else
        
        std::cout << "⚠️ Event Visualizer Tests skipped - requires DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER" << std::endl;
        
        #endif
        
        TestFramework::getInstance().printSummary();
        std::cout << "✅ Event Visualizer Tests Completed" << std::endl;
    }
    
    #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
    
    void testRTEventTracingBasic() {
        TEST_SUITE("RT Event Tracing - Basic Functionality");
        
        TEST("Event tracer singleton access") {
            auto& tracer = Debug::RTEventTracer::getInstance();
            
            // Should be able to access singleton
            ASSERT_TRUE(&tracer != nullptr);
            
        } END_TEST();
        
        TEST("Event tracing macros work") {
            auto& tracer = Debug::RTEventTracer::getInstance();
            
            // Reset statistics
            tracer.resetStatistics();
            
            // Trace some events using the macros
            TRACE_CLOCK_EVENT("MidiClockManager", "ClockTab", "onClockTick", "144");
            TRACE_UI_EVENT("ClockTab", "TransportControl", "updateTransportState", "1");
            TRACE_MIDI_EVENT("HardwareMidiManager", "ExternalSynth", "sendNoteOn", "60");
            
            // Check statistics show events were recorded
            ASSERT_TRUE(tracer.getTotalEvents() >= 3);
            
        } END_TEST();
        
        TEST("Event data integrity") {
            auto& tracer = Debug::RTEventTracer::getInstance();
            tracer.resetStatistics();
            
            // Trace a specific event
            TRACE_PARAMETER_EVENT("ParameterManager", "MidiBridge", "parameterChanged", "1001:0.75");
            
            // Read it back
            Debug::RTEventTracer::EventTrace trace;
            ASSERT_TRUE(tracer.popTrace(trace));
            
            // Verify event details
            ASSERT_TRUE(strcmp(trace.source_name, "ParameterManager") == 0);
            ASSERT_TRUE(strcmp(trace.target_name, "MidiBridge") == 0);
            ASSERT_TRUE(strcmp(trace.event_name, "parameterChanged") == 0);
            ASSERT_TRUE(strcmp(trace.event_data, "1001:0.75") == 0);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    void testTracedCallbackSystem() {
        TEST_SUITE("Traced Callback System");
        
        TEST("Basic callback tracing") {
            auto& tracer = Debug::RTEventTracer::getInstance();
            tracer.resetStatistics();
            
            bool callback_called = false;
            int received_tick = 0;
            
            // Register a simple traced callback manually
            auto traced_callback = [&callback_called, &received_tick](int tick) {
                TRACE_CLOCK_EVENT("MidiClockManager", "TestClockTab", "onClockTick", std::to_string(tick).c_str());
                callback_called = true;
                received_tick = tick;
            };
            
            mock_clock_manager_.setClockTickCallback(traced_callback);
            
            // Simulate clock tick
            mock_clock_manager_.simulateClockTick(144);
            
            // Verify callback was called
            ASSERT_TRUE(callback_called);
            ASSERT_EQ(144, received_tick);
            
            // Verify event was traced
            ASSERT_TRUE(tracer.getTotalEvents() >= 1);
            
        } END_TEST();
        
        TEST("Transport callback tracing") {
            auto& tracer = Debug::RTEventTracer::getInstance();
            tracer.resetStatistics();
            
            bool callback_called = false;
            int old_state = 0, new_state = 0;
            
            // Register traced transport callback manually
            auto traced_transport_callback = [&](int old_s, int new_s) {
                TRACE_UI_EVENT("MidiClockManager", "TestTransportTab", "onTransportChanged", 
                                     (std::to_string(old_s) + "->" + std::to_string(new_s)).c_str());
                callback_called = true;
                old_state = old_s;
                new_state = new_s;
            };
            
            mock_clock_manager_.setTransportChangedCallback(traced_transport_callback);
            
            // Simulate transport change
            mock_clock_manager_.simulateTransportChange(0, 1); // Stop -> Play
            
            // Verify callback was called
            ASSERT_TRUE(callback_called);
            ASSERT_EQ(0, old_state);
            ASSERT_EQ(1, new_state);
            
            // Verify event was traced
            ASSERT_TRUE(tracer.getTotalEvents() >= 1);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    void testHighFrequencyEventHandling() {
        TEST_SUITE("High Frequency Event Handling");
        
        TEST("Rapid clock tick handling") {
            auto& tracer = Debug::RTEventTracer::getInstance();
            tracer.resetStatistics();
            
            const int num_ticks = 100; // Reduced for simple test
            int callback_count = 0;
            
            // Register callback that counts invocations
            auto high_freq_callback = [&callback_count, &tracer](int tick) {
                // Only trace every 10th tick to avoid overwhelming the buffer
                if (tick % 10 == 0) {
                    TRACE_CLOCK_EVENT("MidiClockManager", "HighFreqTest", "onClockTick", std::to_string(tick).c_str());
                }
                callback_count++;
            };
            
            mock_clock_manager_.setClockTickCallback(high_freq_callback);
            
            // Simulate rapid clock ticks
            auto start_time = std::chrono::high_resolution_clock::now();
            
            for (int i = 0; i < num_ticks; ++i) {
                mock_clock_manager_.simulateClockTick(i);
                
                // Small delay to simulate realistic timing
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            // Verify all callbacks were processed
            ASSERT_EQ(num_ticks, callback_count);
            
            // Verify reasonable performance
            ASSERT_TRUE(duration.count() < 1000);
            
            // Verify events were traced
            ASSERT_TRUE(tracer.getTotalEvents() > 0);
            
            std::cout << "    Processed " << num_ticks << " ticks in " << duration.count() << "ms" << std::endl;
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    #endif // DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER
};

int main() {
    std::cout << "🎛️ Event Visualizer Test Runner" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "Testing RT-safe event tracing and callback system" << std::endl;
    std::cout << "For the event flow visualizer\n" << std::endl;
    
    // Run the event visualizer test suite
    EventVisualizerTests test_suite;
    test_suite.runAllTests();
    
    return 0;
}
