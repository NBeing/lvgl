/**
 * @brief Comprehensive RT-Safe MIDI Tests
 * 
 * Tests the bidirectional MIDI-Parameter synchronization system
 * Works on both ESP32 and Linux desktop builds
 */

#include "LVGLTestFramework.h"
#include "components/midi/RTSafeEventDistributor.h"
#include "components/midi/RTSafeMidiEventSystem.h"
#include "components/controls/BidirectionalParameterMidiBridge.h"
#include "components/parameter/Parameter.h"
#include "components/parameter/ParameterManager.h"

using namespace LVGLTest;

class RTSafeMidiTestSuite {
private:
    RTSafeEventDistributor distributor_;
    RTSafeMidiEventSystem midi_system_;
    BidirectionalParameterMidiBridge bridge_;
    MockMidiInterface mock_midi_;
    
    // Test parameters
    std::unique_ptr<Parameter> filter_cutoff_;
    std::unique_ptr<Parameter> filter_resonance_;
    
public:
    void setUp() {
        // Create test parameters
        filter_cutoff_ = std::make_unique<Parameter>(
            "Filter Cutoff", "Cutoff", 74, ParameterCategory::FILTER, 0, 127, 64
        );
        filter_resonance_ = std::make_unique<Parameter>(
            "Filter Resonance", "Res", 71, ParameterCategory::FILTER, 0, 127, 32
        );
        
        // Initialize systems
        distributor_.initialize();
        midi_system_.initialize(&distributor_);
        bridge_.initialize(&midi_system_, &mock_midi_);
        
        mock_midi_.clear();
    }
    
    void tearDown() {
        bridge_.shutdown();
        midi_system_.shutdown();
        distributor_.shutdown();
    }
    
    // Test 1: Basic RT-Safe Event Distribution
    LVGL_RT_TEST(test_rt_safe_event_distribution) {
        bool rt_observer_called = false;
        bool ui_observer_called = false;
        
        // Register RT observer
        auto rt_observer = [&](const MidiEvent& event) {
            rt_observer_called = true;
            // This must be RT-safe!
            ASSERT_RT_SAFE({
                // Simulate RT processing
                volatile int dummy = 0;
                for (int i = 0; i < 100; ++i) dummy += i;
            });
        };
        
        // Register UI observer  
        auto ui_observer = [&](const MidiEvent& event) {
            ui_observer_called = true;
            // UI operations can take longer
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        };
        
        distributor_.addRTObserver(rt_observer);
        distributor_.addUIObserver(ui_observer);
        
        // Send event
        MidiEvent test_event{MidiEventType::CONTROL_CHANGE, 1, 74, 100};
        distributor_.notifyRTObservers(test_event);
        
        // RT observer should be called immediately
        LVGL_ASSERT_TRUE(rt_observer_called);
        
        // Process UI events
        distributor_.processUIEvents();
        LVGL_ASSERT_TRUE(ui_observer_called);
    } END_LVGL_RT_TEST()
    
    // Test 2: Parameter Change → MIDI CC
    LVGL_RT_TEST(test_parameter_to_midi) {
        // Bind parameter to bridge
        bridge_.bindParameter(filter_cutoff_.get());
        
        // Change parameter value
        filter_cutoff_->setValue(100);
        
        // Process RT events
        distributor_.processRTEvents();
        
        // Verify MIDI CC was sent
        ASSERT_MIDI_CC_SENT(mock_midi_, 74, 100);
    } END_LVGL_RT_TEST()
    
    // Test 3: MIDI CC → Parameter Change
    LVGL_RT_TEST(test_midi_to_parameter) {
        bridge_.bindParameter(filter_cutoff_.get());
        
        // Simulate incoming MIDI CC
        mock_midi_.simulateReceiveCC(1, 74, 85);
        bridge_.processMidiInput();
        
        // Verify parameter was updated
        LVGL_ASSERT_EQ(85, filter_cutoff_->getCurrentValue());
    } END_LVGL_RT_TEST()
    
    // Test 4: Bidirectional Sync (No Feedback Loop)
    LVGL_RT_TEST(test_bidirectional_no_feedback) {
        bridge_.bindParameter(filter_cutoff_.get());
        
        size_t initial_midi_count = mock_midi_.getSentMessageCount();
        
        // External MIDI changes parameter
        mock_midi_.simulateReceiveCC(1, 74, 90);
        bridge_.processMidiInput();
        
        // Should not cause feedback MIDI message
        LVGL_ASSERT_EQ(initial_midi_count, mock_midi_.getSentMessageCount());
        LVGL_ASSERT_EQ(90, filter_cutoff_->getCurrentValue());
    } END_LVGL_RT_TEST()
    
    // Test 5: RT Timing Constraints
    LVGL_RT_TEST(test_rt_timing_constraints) {
        bridge_.bindParameter(filter_cutoff_.get());
        
        // Test RT event processing timing
        ASSERT_RT_SAFE({
            for (int i = 0; i < 10; ++i) {
                MidiEvent event{MidiEventType::CONTROL_CHANGE, 1, 74, static_cast<uint8_t>(i * 10)};
                distributor_.notifyRTObservers(event);
            }
        });
        
        // Test parameter update timing
        ASSERT_RT_SAFE({
            for (uint8_t value = 0; value < 127; value += 10) {
                filter_cutoff_->setValue(value);
            }
        });
    } END_LVGL_RT_TEST()
    
    // Test 6: Multiple Parameter Sync
    LVGL_RT_TEST(test_multiple_parameters) {
        bridge_.bindParameter(filter_cutoff_.get());
        bridge_.bindParameter(filter_resonance_.get());
        
        // Change both parameters
        filter_cutoff_->setValue(80);
        filter_resonance_->setValue(60);
        
        distributor_.processRTEvents();
        
        // Verify both MIDI CCs were sent
        ASSERT_MIDI_CC_SENT(mock_midi_, 74, 80);  // Cutoff
        ASSERT_MIDI_CC_SENT(mock_midi_, 71, 60);  // Resonance
    } END_LVGL_RT_TEST()
    
    // Test 7: LVGL Dial Integration
    LVGL_RT_TEST(test_lvgl_dial_integration) {
        auto dial = LVGLTestHelper::createTestDial(74);
        bridge_.bindParameter(filter_cutoff_.get());
        
        // Simulate user moving dial
        LVGLTestHelper::simulateDialChange(dial, 95);
        
        // Process events (in real app this would be automatic)
        distributor_.processRTEvents();
        
        // Verify MIDI was sent and parameter updated
        ASSERT_MIDI_CC_SENT(mock_midi_, 74, 95);
        LVGL_ASSERT_EQ(95, filter_cutoff_->getCurrentValue());
        
        lv_obj_del(dial);
    } END_LVGL_RT_TEST()
    
    // Test 8: Memory Safety (No Allocations in RT Path)
    LVGL_RT_TEST(test_rt_memory_safety) {
        bridge_.bindParameter(filter_cutoff_.get());
        
        size_t objects_before = LVGLTestHelper::getObjectCount();
        
        // RT operations should not allocate
        ASSERT_RT_SAFE({
            for (int i = 0; i < 100; ++i) {
                MidiEvent event{MidiEventType::CONTROL_CHANGE, 1, 74, static_cast<uint8_t>(i % 128)};
                distributor_.notifyRTObservers(event);
            }
        });
        
        // No objects should be created/destroyed
        size_t objects_after = LVGLTestHelper::getObjectCount();
        LVGL_ASSERT_EQ(objects_before, objects_after);
    } END_LVGL_RT_TEST()
    
    // Test 9: Thread Safety
    LVGL_RT_TEST(test_thread_safety) {
        bridge_.bindParameter(filter_cutoff_.get());
        
        std::atomic<bool> rt_thread_running{true};
        std::atomic<int> rt_events_processed{0};
        std::atomic<int> ui_events_processed{0};
        
        // Simulate RT thread
        std::thread rt_thread([&]() {
            while (rt_thread_running) {
                MidiEvent event{MidiEventType::CONTROL_CHANGE, 1, 74, 
                               static_cast<uint8_t>(rt_events_processed % 128)};
                distributor_.notifyRTObservers(event);
                rt_events_processed++;
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        // Simulate UI thread
        std::thread ui_thread([&]() {
            while (rt_thread_running) {
                distributor_.processUIEvents();
                ui_events_processed++;
                std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
            }
        });
        
        // Run for a short time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        rt_thread_running = false;
        
        rt_thread.join();
        ui_thread.join();
        
        // Verify both threads processed events
        LVGL_ASSERT_TRUE(rt_events_processed > 0);
        LVGL_ASSERT_TRUE(ui_events_processed > 0);
    } END_LVGL_RT_TEST()
    
    // Test 10: Error Handling
    LVGL_RT_TEST(test_error_handling) {
        // Test invalid CC numbers
        auto invalid_param = std::make_unique<Parameter>(
            "Invalid", "Inv", 255, ParameterCategory::FILTER  // Invalid CC
        );
        
        // Should not crash
        LVGL_ASSERT_TRUE(bridge_.bindParameter(invalid_param.get()));
        
        // Test null parameter
        LVGL_ASSERT_FALSE(bridge_.bindParameter(nullptr));
        
        // Test out-of-range values
        filter_cutoff_->setValue(200);  // > 127
        LVGL_ASSERT_TRUE(filter_cutoff_->getCurrentValue() <= 127);
    } END_LVGL_RT_TEST()
};

// Test runner function
void runRTSafeMidiTests() {
    #ifdef ESP32_BUILD
    UNITY_BEGIN();
    #else
    TEST_SUITE("RT-Safe MIDI System");
    #endif
    
    RTSafeMidiTestSuite test_suite;
    
    // Run all tests
    test_suite.setUp();
    
    test_suite.test_rt_safe_event_distribution();
    test_suite.test_parameter_to_midi();
    test_suite.test_midi_to_parameter();
    test_suite.test_bidirectional_no_feedback();
    test_suite.test_rt_timing_constraints();
    test_suite.test_multiple_parameters();
    test_suite.test_lvgl_dial_integration();
    test_suite.test_rt_memory_safety();
    test_suite.test_thread_safety();
    test_suite.test_error_handling();
    
    test_suite.tearDown();
    
    #ifdef ESP32_BUILD
    UNITY_END();
    #else
    END_TEST_SUITE();
    Test::TestFramework::getInstance().printSummary();
    #endif
}
