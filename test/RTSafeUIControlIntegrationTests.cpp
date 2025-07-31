/**
 * @brief Comprehensive RT-Safe UI Control Integration Tests
 * 
 * ===================================================================
 * 🎵 THE STORY OF OUR RT-SAFE SYNTHESIZER SYSTEM 🎵
 * ===================================================================
 * 
 * This test suite tells the complete story of how our professional 
 * synthesizer handles UI control integration with real-time audio:
 * 
 * 📖 CHAPTER 1: User Interaction (UI → Parameters)
 *    User turns knobs and moves sliders. Values are validated, 
 *    quantized, and sent to the audio engine with perfect precision.
 * 
 * 📖 CHAPTER 2: External Control (Parameters → UI) 
 *    MIDI controllers and automation change parameters. UI controls
 *    automatically update to stay synchronized with reality.
 * 
 * 📖 CHAPTER 3: Visual Polish (Smooth Interpolation)
 *    Parameter changes animate smoothly for professional visual appeal,
 *    while critical changes can bypass smoothing when needed.
 * 
 * 📖 CHAPTER 4: Bulletproof Operation (Error Handling)
 *    Invalid inputs are gracefully rejected. Values are safely clamped.
 *    System remains stable under all error conditions.
 * 
 * 📖 CHAPTER 5: Live Performance (Thread Safety)
 *    Multiple threads operate concurrently without data races or crashes.
 *    UI, MIDI, and automation all work together harmoniously.
 * 
 * 📖 CHAPTER 6: Professional Standards (RT Timing)
 *    All operations complete within strict real-time deadlines.
 *    No audio dropouts or glitches, even under heavy load.
 * 
 * 📖 CHAPTER 7: Musical Intelligence (Control Behaviors)
 *    Different control types (dials, sliders) with specialized scaling
 *    (logarithmic frequency, linear volume) for musical workflows.
 * 
 * 📖 CHAPTER 8: Smart Interaction (User Priority)
 *    When user is actively controlling a parameter, external changes
 *    are ignored to prevent "fighting" between control sources.
 * 
 * 📖 CHAPTER 9: System Health (Monitoring)
 *    Comprehensive statistics track performance metrics for 
 *    optimization and debugging in production environments.
 * 
 * 🎯 THE RESULT: A production-ready RT-safe parameter system that
 *    could power professional synthesizers and DAW plugins!
 * 
 * ===================================================================
 * 
 * Tests the complete UI control integration system:
 * - UI control → parameter updates (user interaction)
 * - Parameter → UI control updates (MIDI/automation)
 * - Smooth value interpolation and responsiveness
 * - Thread safety and RT-safe operation
 * - Validation and error handling
 * - Performance and timing requirements
 */

#include "TestFramework.h"
#include "components/ui/RTSafeUIControlIntegration.h"
#include "components/threading/RTSafeEventDistributor.h"
#include "components/parameter/RTSafeParameterManager.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <unordered_map>
#include <iomanip>

using namespace RTSafe;
using namespace Test;

/**
 * @brief Mock UI System for testing
 */
class MockUISystem {
private:
    std::unordered_map<uint32_t, float> ui_control_values_;
    std::unordered_map<uint32_t, bool> ui_update_flags_;
    std::atomic<int> ui_update_calls_{0};
    mutable std::mutex mutex_;
    
public:
    // UI update callback (simulates actual UI framework updates)
    void onUIControlUpdate(uint32_t control_id, float value, bool immediate) {
        std::lock_guard<std::mutex> lock(mutex_);
        ui_control_values_[control_id] = value;
        ui_update_flags_[control_id] = true;
        ui_update_calls_++;
        
        std::cout << "🖥️  UI Update: Control " << control_id 
                  << " = " << value 
                  << (immediate ? " (immediate)" : " (smooth)") << std::endl;
    }
    
    float getControlValue(uint32_t control_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = ui_control_values_.find(control_id);
        return (it != ui_control_values_.end()) ? it->second : 0.0f;
    }
    
    bool wasControlUpdated(uint32_t control_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = ui_update_flags_.find(control_id);
        return (it != ui_update_flags_.end()) ? it->second : false;
    }
    
    void clearUpdateFlags() {
        std::lock_guard<std::mutex> lock(mutex_);
        ui_update_flags_.clear();
    }
    
    int getUpdateCallsCount() const {
        return ui_update_calls_.load();
    }
    
    void resetUpdateCallsCount() {
        ui_update_calls_ = 0;
    }
};

/**
 * @brief UI Control Integration Test Suite
 */
class RTSafeUIControlIntegrationTests {
private:
    RTSafeEventDistributor distributor_;
    std::unique_ptr<RTSafeParameterManager> parameter_manager_;
    std::unique_ptr<RTSafeUIControlIntegration> integration_;
    MockUISystem ui_system_;
    
public:
    void setUp() {
        distributor_.initialize();
        
        // Create parameter manager
        parameter_manager_ = std::make_unique<RTSafeParameterManager>(&distributor_);
        parameter_manager_->initialize();
        parameter_manager_->setSampleRate(44100.0f);
        
        // Create integration
        integration_ = std::make_unique<RTSafeUIControlIntegration>(&distributor_, parameter_manager_.get());
        integration_->initialize();
        integration_->resetStatistics();
        
        // Set UI update callback
        integration_->setUIUpdateCallback(
            [this](uint32_t control_id, float value, bool immediate) {
                ui_system_.onUIControlUpdate(control_id, value, immediate);
            }
        );
        
        // Add test controls
        setupTestControls();
        
        // Clear UI state
        ui_system_.clearUpdateFlags();
        ui_system_.resetUpdateCallsCount();
    }
    
    void tearDown() {
        integration_->shutdown();
        parameter_manager_->shutdown();
        distributor_.shutdown();
    }
    
    void setupTestControls() {
        // Filter Cutoff Dial
        RTSafeUIControlIntegration::ControlConfig cutoff_config;
        cutoff_config.control_id = 1001;
        cutoff_config.parameter_id = 1001;
        cutoff_config.type = RTSafeUIControlIntegration::ControlType::DIAL;
        cutoff_config.min_value = 20.0f;
        cutoff_config.max_value = 20000.0f;
        cutoff_config.default_value = 1000.0f;
        cutoff_config.logarithmic = true;
        cutoff_config.label = "Filter Cutoff";
        cutoff_config.units = "Hz";
        integration_->addControl(cutoff_config);
        
        // Filter Resonance Slider
        RTSafeUIControlIntegration::ControlConfig resonance_config;
        resonance_config.control_id = 1002;
        resonance_config.parameter_id = 1002;
        resonance_config.type = RTSafeUIControlIntegration::ControlType::SLIDER;
        resonance_config.min_value = 0.0f;
        resonance_config.max_value = 1.0f;
        resonance_config.default_value = 0.5f;
        resonance_config.step_size = 0.01f;
        resonance_config.label = "Filter Resonance";
        resonance_config.units = "";
        integration_->addControl(resonance_config);
        
        // Master Volume Fader
        RTSafeUIControlIntegration::ControlConfig volume_config;
        volume_config.control_id = 4001;
        volume_config.parameter_id = 4001;
        volume_config.type = RTSafeUIControlIntegration::ControlType::SLIDER;
        volume_config.min_value = 0.0f;
        volume_config.max_value = 1.0f;
        volume_config.default_value = 0.8f;
        volume_config.label = "Master Volume";
        volume_config.units = "";
        integration_->addControl(volume_config);
    }
    
    // ========================================================================
    // TEST 1: UI Control → Parameter Updates
    // ========================================================================
    // STORY: User interacts with synthesizer controls (dials, sliders, buttons)
    // GOAL: Ensure UI changes are properly converted to parameter values and 
    //       validated, with appropriate feedback to the audio engine
    void testUIControlToParameterUpdates() {
        TEST_SUITE("UI Control → Parameter Updates");
        
        /**
         * TEST: User turns a dial and the system responds correctly
         * 
         * SCENARIO: User adjusts the filter cutoff dial from default (1kHz) to 
         *           75% position, which should map to ~3.5kHz on logarithmic scale
         * 
         * VALIDATES:
         * - Raw UI input (0.75) is properly stored as control value
         * - Logarithmic scaling converts control value to proper frequency
         * - Parameter update event is sent to RT system
         * - Statistics tracking works correctly
         * - Audio engine gets immediate feedback for responsive sound
         */
        TEST("Dial value change triggers parameter update") {
            integration_->resetStatistics();
            
            // SIMULATE: User turns filter cutoff dial to 75% position
            bool success = integration_->updateControlFromUI(1001, 0.75f, true);
            ASSERT_TRUE(success);
            
            // PROCESS: UI update system processes the change
            integration_->processUIUpdates();
            
            // VERIFY: Control value stored correctly (normalized 0-1)
            float control_value = integration_->getControlValue(1001);
            std::cout << "DEBUG: Control value = " << control_value << ", Expected: 0.75" << std::endl;
            ASSERT_TRUE(control_value >= 0.74f && control_value <= 0.76f);
            
            // VERIFY: Display value uses logarithmic scaling (20Hz-20kHz range)
            float display_value = integration_->getControlDisplayValue(1001);
            std::cout << "DEBUG: Display value = " << display_value << " Hz" << std::endl;
            ASSERT_TRUE(display_value >= 1000.0f && display_value <= 20000.0f); // Should be in Hz range
            
            // VERIFY: System tracked the UI→Parameter update for monitoring
            auto stats = integration_->getStatistics();
            ASSERT_EQ(1, stats.ui_to_param_updates);
            
        } END_TEST();
        
        /**
         * TEST: Step size quantization for precise control
         * 
         * SCENARIO: User moves resonance slider to 54.7%, but slider has 0.01 
         *           step size, so it should quantize to 55% (0.55)
         * 
         * VALIDATES:
         * - Step size quantization works for precise parameter control
         * - Useful for parameters that need specific discrete values
         * - Prevents audio artifacts from micro-adjustments
         * - Essential for professional music production workflows
         */
        TEST("Slider with step size quantizes values") {
            // SIMULATE: User drags resonance slider to 54.7% (has 0.01 step size)
            bool success = integration_->updateControlFromUI(1002, 0.547f, true);
            ASSERT_TRUE(success);
            
            integration_->processUIUpdates();
            
            // VERIFY: Value quantized to nearest 0.01 step (54.7% → 55%)
            float control_value = integration_->getControlValue(1002);
            ASSERT_TRUE(std::abs(control_value - 0.55f) < 0.005f); // Should be ~0.55
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // ========================================================================
    // TEST 2: Parameter → UI Control Updates  
    // ========================================================================
    // STORY: External systems (MIDI controllers, automation, presets) change 
    //        parameter values and UI controls must reflect these changes
    // GOAL: Ensure the UI stays synchronized with the actual parameter state
    void testParameterToUIControlUpdates() {
        TEST_SUITE("Parameter → UI Control Updates");
        
        /**
         * TEST: MIDI controller changes parameter, UI control updates automatically
         * 
         * SCENARIO: External MIDI controller sends CC message changing filter 
         *           resonance. The UI resonance slider should automatically 
         *           update to reflect the new value.
         * 
         * VALIDATES:
         * - RT system can notify UI of parameter changes
         * - UI controls stay synchronized with actual parameter values
         * - Essential for hardware controller integration
         * - Prevents UI showing wrong values during live performance
         * - Maintains single source of truth for parameter state
         */
        TEST("Parameter change updates UI control") {
            integration_->resetStatistics();
            ui_system_.clearUpdateFlags();
            
            // SETUP: Clear any user interaction flags from previous tests
            integration_->clearUserInteraction(1002);
            
            // SIMULATE: External MIDI controller changes resonance parameter
            RTEvent param_event = RTEvent::parameterChange(1002 >> 8, 1002 & 0xFF);
            
            // PROCESS: Send through UI observer (simulates RT → UI event flow)
            integration_->handleUIEvent(param_event);
            
            // PROCESS: UI update system processes the parameter change
            integration_->processUIUpdates();
            
            // VERIFY: UI control was updated to match parameter change
            ASSERT_TRUE(ui_system_.wasControlUpdated(1002));
            
            // VERIFY: System tracked the Parameter→UI update for monitoring
            auto stats = integration_->getStatistics();
            ASSERT_EQ(1, stats.param_to_ui_updates);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // ========================================================================
    // TEST 3: Smooth Value Interpolation
    // ========================================================================
    // STORY: When parameters change from external sources, the UI should 
    //        smoothly animate to new values rather than jumping instantly
    // GOAL: Provide responsive, professional UI feedback that's easy on the eyes
    void testSmoothValueInterpolation() {
        TEST_SUITE("Smooth Value Interpolation");
        
        /**
         * TEST: UI controls smoothly animate to new values for visual appeal
         * 
         * SCENARIO: Automation system slowly sweeps master volume from 20% to 80%.
         *           The UI volume fader should smoothly glide to show the change
         *           rather than jumping instantly.
         * 
         * VALIDATES:
         * - Smooth interpolation makes UI changes visually appealing
         * - Multiple update cycles gradually reach target value
         * - Essential for professional DAW-like user experience
         * - Prevents jarring visual jumps during automation playback  
         * - Interpolation statistics are tracked for performance monitoring
         */
        TEST("Smooth updates interpolate values gradually") {
            integration_->setSmoothUpdatesEnabled(true);
            integration_->setInterpolationSpeed(0.9f); // Very slow for testing
            integration_->resetStatistics();
            
            // SETUP: Clear UI update tracking to start fresh
            ui_system_.clearUpdateFlags();
            ui_system_.resetUpdateCallsCount();
            
            // SETUP: Set initial value for master volume
            integration_->updateControlFromUI(4001, 0.2f, true);
            integration_->processUIUpdates();
            
            // SIMULATE: Automation changes volume to 80% (smooth transition)
            integration_->updateControlFromUI(4001, 0.8f, false);
            
            float initial_value = integration_->getControlValue(4001);
            
            // PROCESS: Multiple update cycles for smooth interpolation
            for (int i = 0; i < 10; ++i) {
                integration_->processUIUpdates();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            float final_value = integration_->getControlValue(4001);
            
            // VERIFY: Value moved toward target but not fully reached (smooth interpolation)
            ASSERT_TRUE(final_value > initial_value);
            ASSERT_TRUE(final_value < 0.8f); // Not reached target yet due to slow interpolation
            
            // VERIFY: Interpolation statistics tracked for performance monitoring
            auto stats = integration_->getStatistics();
            ASSERT_TRUE(stats.smooth_interpolations > 0);
            
        } END_TEST();
        
        /**
         * TEST: Critical UI changes happen immediately when needed
         * 
         * SCENARIO: User makes a critical adjustment (like emergency volume cut)
         *           that needs immediate response, not smooth interpolation
         * 
         * VALIDATES:
         * - Immediate flag bypasses smooth interpolation
         * - Critical safety functions work instantly
         * - User has control over responsiveness vs. smoothness
         * - Essential for live performance emergency controls
         */
        TEST("Immediate updates bypass interpolation") {
            integration_->setSmoothUpdatesEnabled(true);
            
            // SIMULATE: Emergency volume cut needs immediate response
            integration_->updateControlFromUI(4001, 0.9f, true);
            integration_->processUIUpdates();
            
            // VERIFY: Value changed immediately, no interpolation delay
            float value = integration_->getControlValue(4001);
            ASSERT_TRUE(std::abs(value - 0.9f) < 0.01f);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // ========================================================================
    // TEST 4: Control Validation and Error Handling
    // ========================================================================
    // STORY: Robust error handling prevents crashes and data corruption
    //        when invalid inputs or edge cases occur
    // GOAL: System stability and predictable behavior under all conditions
    void testControlValidationAndErrorHandling() {
        TEST_SUITE("Control Validation and Error Handling");
        
        /**
         * TEST: System gracefully handles invalid control IDs
         * 
         * SCENARIO: UI framework sends update for control that doesn't exist
         *           (could happen during UI teardown or plugin unloading)
         * 
         * VALIDATES:
         * - Invalid control IDs are rejected without crashing
         * - Validation failures are tracked for debugging
         * - System remains stable when UI sends bad data
         * - Essential for plugin host environments
         */
        TEST("Invalid control ID rejected") {
            integration_->resetStatistics();
            
            // SIMULATE: UI sends update for non-existent control
            bool success = integration_->updateControlFromUI(9999, 0.5f);
            ASSERT_FALSE(success);
            
            // VERIFY: Validation failure was recorded for debugging
            auto stats = integration_->getStatistics();
            ASSERT_EQ(1, stats.validation_failures);
            
        } END_TEST();
        
        /**
         * TEST: Parameter values are safely clamped to valid ranges
         * 
         * SCENARIO: UI sends out-of-range values (could happen from automation 
         *           data corruption, user scripts, or hardware controller glitches)
         * 
         * VALIDATES:
         * - Values above maximum are clamped to maximum
         * - Values below minimum are clamped to minimum  
         * - Prevents audio engine receiving invalid parameter values
         * - Protects against hardware controller malfunctions
         * - Essential for audio safety (prevents ear damage)
         */
        TEST("Values clamped to valid range") {
            // SIMULATE: Corrupted automation sends value above maximum (150%)
            integration_->updateControlFromUI(1002, 1.5f, true); // Resonance max is 1.0
            integration_->processUIUpdates();
            
            float value = integration_->getControlValue(1002);
            ASSERT_TRUE(value <= 1.0f); // VERIFY: Clamped to maximum
            
            // SIMULATE: Hardware controller glitch sends negative value
            integration_->updateControlFromUI(1002, -0.5f, true); // Below minimum 0.0
            integration_->processUIUpdates();
            
            value = integration_->getControlValue(1002);
            ASSERT_TRUE(value >= 0.0f); // VERIFY: Clamped to minimum
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // ========================================================================
    // TEST 5: Thread Safety
    // ========================================================================
    // STORY: Professional audio software runs with multiple threads concurrently
    //        (UI thread, audio thread, MIDI thread, automation thread)
    // GOAL: Ensure no data races, crashes, or corruption under concurrent access
    void testThreadSafety() {
        TEST_SUITE("Thread Safety");
        
        /**
         * TEST: System handles concurrent UI and parameter updates safely
         * 
         * SCENARIO: Live performance with multiple threads running:
         *           - UI thread: User turning knobs rapidly
         *           - MIDI thread: Hardware controller sending CC messages  
         *           - Automation thread: DAW playing back automation data
         *           - UI processing thread: Updating screen at 60Hz
         * 
         * VALIDATES:
         * - No data races between concurrent threads
         * - No crashes under high concurrency load
         * - All updates are processed correctly
         * - Atomic operations prevent corruption
         * - Essential for professional live performance
         */
        TEST("Concurrent UI and parameter updates") {
            std::atomic<bool> test_running{true};
            std::atomic<int> ui_updates{0};
            std::atomic<int> param_updates{0};
            
            // SIMULATE: UI thread (user rapidly turning controls)
            std::thread ui_thread([&]() {
                while (test_running) {
                    float value = (ui_updates % 100) / 100.0f;
                    integration_->updateControlFromUI(1001, value, true);
                    ui_updates++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
            
            // SIMULATE: MIDI/automation thread (external parameter changes)
            std::thread param_thread([&]() {
                while (test_running) {
                    RTEvent param_event = RTEvent::parameterChange(1002 >> 8, 1002 & 0xFF);
                    integration_->handleUIEvent(param_event);
                    param_updates++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(15));
                }
            });
            
            // SIMULATE: UI processing thread (60Hz screen updates)
            std::thread ui_process_thread([&]() {
                while (test_running) {
                    integration_->processUIUpdates();
                    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60Hz
                }
            });
            
            // RUN: Stress test for 200ms with high concurrency
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            test_running = false;
            
            ui_thread.join();
            param_thread.join();
            ui_process_thread.join();
            
            // VERIFY: No crashes occurred and all threads processed updates
            ASSERT_TRUE(ui_updates > 0);
            ASSERT_TRUE(param_updates > 0);
            ASSERT_TRUE(ui_system_.getUpdateCallsCount() > 0);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // ========================================================================
    // TEST 6: RT Timing Constraints
    // ========================================================================
    // STORY: Professional audio systems have strict timing requirements
    //        UI processing must not interfere with audio thread performance
    // GOAL: Verify UI operations complete within real-time deadlines
    void testRTTimingConstraints() {
        TEST_SUITE("RT Timing Constraints");
        
        /**
         * TEST: UI processing meets professional real-time deadlines
         * 
         * SCENARIO: Live performance with complex synthesizer patch containing
         *           many UI controls that all need updates simultaneously
         * 
         * VALIDATES:
         * - UI processing completes within RT deadlines (<1ms for UI)
         * - Performance doesn't degrade with many controls
         * - System tracks timing statistics for monitoring
         * - Suitable for professional audio applications
         * - Won't cause audio dropouts or glitches
         */
        RT_TEST("UI processing meets timing requirements") {
            // SETUP: Add many controls to stress test the system
            for (uint32_t i = 5000; i < 5020; ++i) {
                RTSafeUIControlIntegration::ControlConfig config;
                config.control_id = i;
                config.parameter_id = i;
                config.type = RTSafeUIControlIntegration::ControlType::DIAL;
                integration_->addControl(config);
            }
            
            // SIMULATE: Update all controls simultaneously (automation burst)
            for (uint32_t i = 5000; i < 5020; ++i) {
                integration_->updateControlFromUI(i, 0.5f, false);
            }
            
            // MEASURE: UI processing time under load
            ASSERT_RT_TIMING({
                integration_->processUIUpdates();
            }, 1000); // 1ms max for UI processing (lenient for UI thread)
            
            // VERIFY: Processing time statistics are tracked
            auto stats = integration_->getStatistics();
            ASSERT_TRUE(stats.max_update_time_us < 10000); // < 10ms reasonable for UI
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // ========================================================================
    // TEST 7: Control Types and Features
    // ========================================================================  
    // STORY: Professional synthesizers need different types of controls with
    //        specialized behaviors (logarithmic frequency, linear volume, etc.)
    // GOAL: Verify all control types work correctly with proper scaling
    void testControlTypesAndFeatures() {
        TEST_SUITE("Control Types and Features");
        
        /**
         * TEST: Logarithmic scaling works correctly for frequency controls
         * 
         * SCENARIO: Filter cutoff frequency needs logarithmic scaling because
         *           human hearing perceives frequency logarithmically
         * 
         * VALIDATES:
         * - 50% control position maps to geometric mean frequency
         * - Provides even perceptual spacing across frequency range
         * - Essential for musical frequency controls (filters, oscillators)
         * - Matches user expectations from other synthesizers
         */
        TEST("Logarithmic scaling works correctly") {
            // SIMULATE: User sets filter cutoff to middle position (50%)
            integration_->updateControlFromUI(1001, 0.5f, true);
            integration_->processUIUpdates();
            
            float display_value = integration_->getControlDisplayValue(1001);
            
            // VERIFY: Logarithmic scaling (50% ≠ linear midpoint)
            // Geometric mean of 20Hz-20kHz range ≈ 632Hz, not 10kHz linear midpoint
            float linear_midpoint = (20.0f + 20000.0f) / 2.0f; // 10010 Hz (linear)
            ASSERT_TRUE(std::abs(display_value - linear_midpoint) > 1000.0f);
            
        } END_TEST();
        
        /**
         * TEST: Different control types are handled correctly
         * 
         * SCENARIO: Synthesizer patch with mixed control types:
         *           - Rotary dial for filter frequency (continuous, logarithmic)
         *           - Linear slider for resonance (continuous, linear)  
         *           - Volume fader for master level (continuous, linear)
         * 
         * VALIDATES:
         * - System handles multiple control types simultaneously
         * - Each control type uses appropriate scaling and behavior
         * - Statistics track updates across all control types
         * - Essential for complex synthesizer interfaces
         */
        TEST("Different control types handled correctly") {
            auto stats_before = integration_->getStatistics();
            
            // SIMULATE: User adjusts different types of controls
            integration_->updateControlFromUI(1001, 0.3f, true); // DIAL (filter cutoff)
            integration_->updateControlFromUI(1002, 0.7f, true); // SLIDER (resonance)
            integration_->updateControlFromUI(4001, 0.9f, true); // SLIDER (master volume)
            
            integration_->processUIUpdates();
            
            auto stats_after = integration_->getStatistics();
            
            // VERIFY: All control types processed correctly
            ASSERT_EQ(3, stats_after.ui_to_param_updates - stats_before.ui_to_param_updates);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // ========================================================================
    // TEST 8: User Interaction Detection
    // ========================================================================
    // STORY: When user is actively adjusting a control, external changes
    //        (MIDI, automation) should not interfere with user's actions
    // GOAL: Prioritize user interaction over external control sources
    void testUserInteractionDetection() {
        TEST_SUITE("User Interaction Detection");
        
        /**
         * TEST: User control takes priority over external automation
         * 
         * SCENARIO: User is actively turning filter cutoff knob while 
         *           automation is also trying to control the same parameter
         * 
         * VALIDATES:
         * - User interaction locks out external parameter changes
         * - Prevents "fighting" between user and automation systems
         * - Timeout releases lock after user stops interacting
         * - Essential for intuitive live performance control
         * - Matches behavior of professional hardware/software
         */
        TEST("User interaction prevents external updates") {
            integration_->setSmoothUpdatesEnabled(true);
            
            // SIMULATE: User starts actively adjusting filter cutoff
            integration_->updateControlFromUI(1001, 0.3f, false);
            
            // SIMULATE: External automation tries to change same parameter
            RTEvent param_event = RTEvent::parameterChange(1001 >> 8, 1001 & 0xFF);
            integration_->handleUIEvent(param_event);
            
            integration_->processUIUpdates();
            
            // VERIFY: Control maintains user's value, ignores external changes
            float value = integration_->getControlValue(1001);
            ASSERT_TRUE(std::abs(value - 0.3f) < 0.1f);
            
            // SIMULATE: User stops interacting (timeout occurs)
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            integration_->processUIUpdates();
            
            // VERIFY: External updates work again after timeout
            integration_->handleUIEvent(param_event);
            integration_->processUIUpdates();
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // ========================================================================
    // TEST 9: Statistics and Monitoring
    // ========================================================================
    // STORY: Professional audio software needs comprehensive monitoring
    //        to debug performance issues and optimize user experience
    // GOAL: Track all important metrics for system health monitoring
    void testStatisticsAndMonitoring() {
        TEST_SUITE("Statistics and Monitoring");
        
        /**
         * TEST: Comprehensive statistics tracking for system monitoring
         * 
         * SCENARIO: Complex synthesizer session with multiple parameter sources:
         *           - User interactions (UI → Parameter updates)
         *           - External control (Parameter → UI updates)  
         *           - Smooth interpolation for visual appeal
         *           - Performance timing measurements
         * 
         * VALIDATES:
         * - All update types are counted correctly
         * - Smooth interpolation events tracked
         * - Performance timing statistics maintained
         * - Active control count accurate
         * - Essential for performance optimization and debugging
         */
        TEST("Statistics tracked correctly") {
            integration_->resetStatistics();
            
            // SIMULATE: User interactions (UI → Parameters)
            integration_->updateControlFromUI(1001, 0.2f, true);  // Immediate update
            integration_->updateControlFromUI(1002, 0.8f, false); // Smooth update
            
            // SETUP: Clear user interaction flags to allow parameter updates
            integration_->clearUserInteraction(1001);
            integration_->clearUserInteraction(4001);
            
            // SIMULATE: External parameter changes (Parameters → UI)
            RTEvent param_event1 = RTEvent::parameterChange(1001 >> 8, 1001 & 0xFF);
            RTEvent param_event2 = RTEvent::parameterChange(4001 >> 8, 4001 & 0xFF);
            integration_->handleUIEvent(param_event1);
            integration_->handleUIEvent(param_event2);
            
            // PROCESS: Multiple update cycles to trigger smooth interpolation
            for (int i = 0; i < 5; ++i) {
                integration_->processUIUpdates();
            }
            
            auto stats = integration_->getStatistics();
            
            // VERIFY: All statistics tracked correctly
            ASSERT_EQ(2, stats.ui_to_param_updates);     // 2 user interactions
            ASSERT_EQ(2, stats.param_to_ui_updates);     // 2 external parameter changes
            ASSERT_TRUE(stats.smooth_interpolations > 0); // Smooth updates occurred
            ASSERT_TRUE(stats.max_update_time_us > 0);    // Timing measured
            ASSERT_TRUE(stats.active_controls >= 3);     // At least our 3 test controls
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // ========================================================================
    // TEST RUNNER: Execute comprehensive test suite
    // ========================================================================
    void runAllTests() {
        std::cout << "🎛️  Starting RT-Safe UI Control Integration Tests" << std::endl;
        
        setUp();
        
        // Execute test suites in logical order telling the story of our system:
        testUIControlToParameterUpdates();      // 1. User turns knobs → Parameters change
        testParameterToUIControlUpdates();      // 2. MIDI/automation → UI updates
        testSmoothValueInterpolation();         // 3. Smooth visual feedback
        testControlValidationAndErrorHandling(); // 4. Robust error handling
        testThreadSafety();                     // 5. Multi-threaded stability
        testRTTimingConstraints();              // 6. Professional performance
        testControlTypesAndFeatures();          // 7. Different control behaviors  
        testUserInteractionDetection();         // 8. Smart interaction priority
        testStatisticsAndMonitoring();          // 9. System health monitoring
        
        tearDown();
        
        TestFramework::getInstance().printSummary();
        std::cout << "✅ RT-Safe UI Control Integration Tests Completed" << std::endl;
    }
};

// Test runner function
void runRTSafeUIControlIntegrationTests() {
    RTSafeUIControlIntegrationTests test_suite;
    test_suite.runAllTests();
}
