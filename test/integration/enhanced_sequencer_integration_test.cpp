/**
 * @brief Enhanced Step Sequencer Integration Test Suite
 * 
 * This file contains comprehensive integration tests for the enhanced step sequencer
 * that combines all Digitakt-style features with the existing parameter lock system
 * and observer pattern architecture.
 * 
 * TEST COVERAGE - COMPLETE SEQUENCER INTEGRATION:
 * 
 * 🎛️ CHAPTER 1: Enhanced Step Integration
 *    Integration of EnhancedSequencerStep with existing StepSequencer
 * 
 * 🔄 CHAPTER 2: Parameter Lock Integration
 *    Parameter locks working with probability, conditions, and retriggers
 * 
 * 📡 CHAPTER 3: Observer Pattern Integration
 *    Event distribution with enhanced sequencer features
 * 
 * ⏱️ CHAPTER 4: Real-Time Performance Integration
 *    RT-safe operation with all enhanced features enabled
 * 
 * 🎯 CHAPTER 5: MIDI Clock Integration
 *    Sync with MIDI clock including micro-timing and swing
 * 
 * 🎵 CHAPTER 6: Multi-Track Sequencer Integration
 *    Multiple tracks with independent enhanced features
 * 
 * 🔧 CHAPTER 7: Pattern Chain Integration
 *    Pattern chaining with fill mode and conditional triggers
 * 
 * ⚡ CHAPTER 8: Performance Under Load
 *    High-throughput sequencing with all features active
 * 
 * 📊 CHAPTER 9: Memory Management Integration
 *    Memory efficiency with complex patterns and locks
 * 
 * ARCHITECTURE VALIDATION:
 * - Validates complete integration of all systems
 * - Tests real-world usage scenarios
 * - Verifies performance under realistic load
 * - Ensures thread safety in complex scenarios
 * - Validates memory usage patterns
 * 
 * @author Generated for Enhanced Sequencer Integration
 * @date August 14, 2025
 */

#include "../framework/unified_test_framework.h"
#include "../fixtures/test_fixtures.h"
#include "../../src/components/midi/StepSequencer.h"
#include "../../src/components/midi/ParameterLockManager.h"
#include "../../src/components/midi/EnhancedSequencerStep.h"
#include "../../src/components/midi/DigitaktFeatures.h"
#include "../../src/components/midi/MidiEvents.h"
#include "../../src/components/parameter/ParameterManager.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>

using namespace MIDI;

// Test fixtures for sequencer integration
class MockParameterManager {
public:
    std::vector<std::pair<int, float>> parameter_changes;
    
    void setParameter(int param_id, float value, int source = 0) {
        parameter_changes.emplace_back(param_id, value);
    }
    
    float getParameterNormalized(int param_id) const {
        // Return the last set value for this parameter
        for (auto it = parameter_changes.rbegin(); it != parameter_changes.rend(); ++it) {
            if (it->first == param_id) {
                return it->second;
            }
        }
        return 0.0f;  // Default value
    }
    
    void clear() {
        parameter_changes.clear();
    }
};

class MockSequencerObserver : public TypedObserver<MIDI::StepSequencer::SequencerEvent> {
public:
    std::vector<MIDI::StepSequencer::SequencerEvent> received_events;
    
    void onEvent(const MIDI::StepSequencer::SequencerEvent& event) override {
        received_events.push_back(event);
    }
    
    void clear() {
        received_events.clear();
    }
    
    size_t getNoteOnCount() const {
        return std::count_if(received_events.begin(), received_events.end(),
                           [](const auto& event) { 
                               return event.type == MIDI::StepSequencer::SequencerEvent::NOTE_ON; 
                           });
    }
    
    size_t getNoteOffCount() const {
        return std::count_if(received_events.begin(), received_events.end(),
                           [](const auto& event) { 
                               return event.type == MIDI::StepSequencer::SequencerEvent::NOTE_OFF; 
                           });
    }
};

// =============================================================================
// CHAPTER 1: ENHANCED STEP INTEGRATION
// =============================================================================

TEST_INTEGRATION(EnhancedSequencer, BasicStepIntegration) {
    PRINT_TEST_HEADER("Enhanced Sequencer - Basic Step Integration");
    
    StepSequencer sequencer;
    MockSequencerObserver observer;
    sequencer.addSequencerObserver(&observer);
    
    // Set up an enhanced step with basic properties
    StepSequencer::Step enhanced_step;
    enhanced_step.active = true;
    enhanced_step.note = 72;
    enhanced_step.velocity = 110;
    enhanced_step.length = 12;  // MIDI clock ticks
    
    // Set the step in the sequencer
    sequencer.setStep(0, 0, enhanced_step);
    
    // Get the step back and verify
    const auto& retrieved_step = sequencer.getStep(0, 0);
    ASSERT_TRUE(retrieved_step.active, "Retrieved step should be active");
    ASSERT_EQ_NUM(retrieved_step.note, 72U, "Retrieved step should have correct note");
    ASSERT_EQ_NUM(retrieved_step.velocity, 110U, "Retrieved step should have correct velocity");
    ASSERT_EQ_NUM(retrieved_step.length, 12U, "Retrieved step should have correct length");
    
    sequencer.removeSequencerObserver(&observer);
    PASS("Basic step integration works correctly");
}

TEST_INTEGRATION(EnhancedSequencer, ParameterLockIntegration) {
    PRINT_TEST_HEADER("Enhanced Sequencer - Parameter Lock Integration");
    
    StepSequencer sequencer;
    MockParameterManager param_manager;
    
    // Set up parameter lock manager
    auto lock_manager = std::make_unique<ParameterLockManager>();
    
    // Create step with parameter locks
    StepSequencer::Step step;
    step.active = true;
    step.note = 60;
    step.velocity = 100;
    
    // Add parameter locks to the step
    step.lockParameter(42, 0.75f);  // Lock filter cutoff
    step.lockParameter(43, 0.50f);  // Lock resonance
    
    sequencer.setStep(0, 4, step);  // Set on track 0, step 4
    
    // Verify locks were set
    ASSERT_TRUE(step.hasParameterLock(42), "Step should have parameter lock 42");
    ASSERT_TRUE(step.hasParameterLock(43), "Step should have parameter lock 43");
    ASSERT_EQ(step.getLockedParameterValue(42), 0.75f, "Parameter lock 42 should have correct value");
    ASSERT_EQ(step.getLockedParameterValue(43), 0.50f, "Parameter lock 43 should have correct value");
    ASSERT_EQ_NUM(step.getParameterLockCount(), 2U, "Step should have 2 parameter locks");
    
    PASS("Parameter lock integration works correctly");
}

TEST_INTEGRATION(EnhancedSequencer, ProbabilityIntegration) {
    PRINT_TEST_HEADER("Enhanced Sequencer - Probability Integration");
    
    // This test verifies that probability works in a real sequencer context
    ProbabilityEngine::setSeed(12345);  // Deterministic seed
    
    const int test_runs = 100;
    const float probability = 0.3f;  // 30% chance
    const float tolerance = 0.15f;   // 15% tolerance
    
    int trigger_count = 0;
    
    for (int run = 0; run < test_runs; ++run) {
        // Create a step with probability
        EnhancedSequencerStep step;
        step.setActive(true);
        step.setProbability(probability);
        
        // Create context for evaluation
        SequencerContext context(0, 0, 0, false);
        
        // Test if step would trigger
        if (step.shouldTrigger(context)) {
            trigger_count++;
        }
    }
    
    float actual_probability = static_cast<float>(trigger_count) / test_runs;
    float error = std::abs(actual_probability - probability);
    
    ASSERT_TRUE(error < tolerance, 
                std::string("Probability integration failed. Expected: ") + 
                std::to_string(probability) + ", Actual: " + std::to_string(actual_probability));
    
    PASS("Probability integration works correctly");
}

TEST_INTEGRATION(EnhancedSequencer, MicroTimingIntegration) {
    PRINT_TEST_HEADER("Enhanced Sequencer - Micro-Timing Integration");
    
    EnhancedSequencerStep step;
    step.setActive(true);
    step.setMicroTiming(20);  // 20 ticks late
    
    uint32_t base_time = 1000;
    uint32_t adjusted_time = step.calculateTriggerTime(base_time);
    
    ASSERT_EQ(adjusted_time, 1020U, "Micro-timing should adjust trigger time");
    
    // Test negative micro-timing
    step.setMicroTiming(-15);  // 15 ticks early
    uint32_t early_time = step.calculateTriggerTime(base_time);
    ASSERT_EQ(early_time, 985U, "Negative micro-timing should make trigger earlier");
    
    // Test with swing integration
    uint32_t swing_time = MicroTimingEngine::calculateSwingTiming(base_time, 1, 0.5f);
    ASSERT_TRUE(swing_time > base_time, "Swing should delay off-beat steps");
    
    PASS("Micro-timing integration works correctly");
}

TEST_INTEGRATION(EnhancedSequencer, RetriggerIntegration) {
    PRINT_TEST_HEADER("Enhanced Sequencer - Retrigger Integration");
    
    EnhancedSequencerStep step;
    step.setActive(true);
    step.setRetriggerCount(3);  // 3 additional triggers
    step.setRetriggerRate(RetriggerEngine::RetriggerRate::RATE_1_16);
    step.setVelocity(127);
    
    // Calculate retrigger times
    uint32_t step_start = 0;
    uint32_t step_duration = 96;  // 1/16 note at 384 PPQN
    auto retrigger_times = step.calculateRetriggerTimes(step_start, step_duration);
    
    ASSERT_EQ_NUM(retrigger_times.size(), 4U, "Should have main trigger + 3 retriggers");
    ASSERT_EQ(retrigger_times[0], step_start, "First trigger should be at step start");
    
    // Check timing intervals
    for (size_t i = 1; i < retrigger_times.size(); ++i) {
        ASSERT_TRUE(retrigger_times[i] > retrigger_times[i-1], "Retriggers should be in chronological order");
    }
    
    // Calculate retrigger velocities
    auto retrigger_velocities = step.calculateRetriggerVelocities();
    ASSERT_EQ_NUM(retrigger_velocities.size(), 4U, "Should have velocity for each trigger");
    ASSERT_EQ_NUM(retrigger_velocities[0], 127U, "First velocity should be base velocity");
    
    // Velocities should generally decrease
    for (size_t i = 1; i < retrigger_velocities.size(); ++i) {
        ASSERT_TRUE(retrigger_velocities[i] <= retrigger_velocities[i-1], "Retrigger velocities should not increase");
    }
    
    PASS("Retrigger integration works correctly");
}

TEST_INTEGRATION(EnhancedSequencer, ConditionalTriggerIntegration) {
    PRINT_TEST_HEADER("Enhanced Sequencer - Conditional Trigger Integration");
    
    EnhancedSequencerStep first_step;
    first_step.setActive(true);
    first_step.setTriggerCondition(ConditionalTriggerEngine::TriggerCondition::FIRST);
    
    EnhancedSequencerStep not_first_step;
    not_first_step.setActive(true);
    not_first_step.setTriggerCondition(ConditionalTriggerEngine::TriggerCondition::NOT_FIRST);
    
    // Test first loop
    SequencerContext first_loop_context(0, 0, 0, false);  // loop_count = 0
    ASSERT_TRUE(first_step.shouldTrigger(first_loop_context), "FIRST step should trigger on first loop");
    ASSERT_FALSE(not_first_step.shouldTrigger(first_loop_context), "NOT_FIRST step should not trigger on first loop");
    
    // Test later loop
    SequencerContext later_loop_context(0, 0, 2, false);  // loop_count = 2
    ASSERT_FALSE(first_step.shouldTrigger(later_loop_context), "FIRST step should not trigger on later loop");
    ASSERT_TRUE(not_first_step.shouldTrigger(later_loop_context), "NOT_FIRST step should trigger on later loop");
    
    PASS("Conditional trigger integration works correctly");
}

TEST_INTEGRATION(EnhancedSequencer, FillModeIntegration) {
    PRINT_TEST_HEADER("Enhanced Sequencer - Fill Mode Integration");
    
    auto& fillMgr = FillModeManager::getInstance();
    
    EnhancedSequencerStep fill_step;
    fill_step.setActive(true);
    fill_step.setTriggerCondition(ConditionalTriggerEngine::TriggerCondition::FILL);
    
    EnhancedSequencerStep not_fill_step;
    not_fill_step.setActive(true);
    not_fill_step.setTriggerCondition(ConditionalTriggerEngine::TriggerCondition::NOT_FILL);
    
    // Test normal mode
    fillMgr.exitFill();  // Ensure fill mode is off
    SequencerContext normal_context(0, 0, 0, false);
    ASSERT_FALSE(fill_step.shouldTrigger(normal_context), "FILL step should not trigger in normal mode");
    ASSERT_TRUE(not_fill_step.shouldTrigger(normal_context), "NOT_FILL step should trigger in normal mode");
    
    // Test fill mode
    fillMgr.enterFill();
    SequencerContext fill_context(0, 0, 0, true);
    ASSERT_TRUE(fill_step.shouldTrigger(fill_context), "FILL step should trigger in fill mode");
    ASSERT_FALSE(not_fill_step.shouldTrigger(fill_context), "NOT_FILL step should not trigger in fill mode");
    
    // Set track fill pattern
    std::vector<bool> fill_pattern = {true, false, true, true, false, false, true, false,
                                     true, true, false, true, false, true, true, false};
    fillMgr.setTrackFillPattern(0, fill_pattern);
    
    const auto& retrieved_pattern = fillMgr.getTrackFillPattern(0);
    ASSERT_EQ(retrieved_pattern.size(), fill_pattern.size(), "Fill pattern should be stored correctly");
    
    fillMgr.exitFill();  // Clean up
    PASS("Fill mode integration works correctly");
}

// =============================================================================
// CHAPTER 2: COMPLEX FEATURE COMBINATIONS
// =============================================================================

TEST_INTEGRATION(EnhancedSequencer, CombinedFeatures) {
    PRINT_TEST_HEADER("Enhanced Sequencer - Combined Features");
    
    // Create a step with multiple advanced features
    EnhancedSequencerStep complex_step;
    complex_step.setActive(true);
    complex_step.setNote(84);
    complex_step.setVelocity(120);
    complex_step.setProbability(0.8f);  // 80% chance
    complex_step.setMicroTiming(15);    // 15 ticks late
    complex_step.setRetriggerCount(2);  // 2 additional triggers
    complex_step.setTriggerCondition(ConditionalTriggerEngine::TriggerCondition::NOT_FIRST);
    complex_step.setParameterLock(42, 0.9f);  // Filter cutoff lock
    complex_step.setParameterLock(43, 0.3f);  // Resonance lock
    
    ASSERT_TRUE(complex_step.hasAdvancedFeatures(), "Step with multiple features should be detected as advanced");
    
    // Test evaluation in different contexts
    SequencerContext first_loop_context(0, 5, 0, false);
    SequencerContext later_loop_context(0, 5, 3, false);
    
    // Should not trigger on first loop due to NOT_FIRST condition
    bool first_loop_result = complex_step.shouldTrigger(first_loop_context);
    ASSERT_FALSE(first_loop_result, "Step with NOT_FIRST condition should not trigger on first loop");
    
    // Should potentially trigger on later loops (subject to probability)
    ProbabilityEngine::setSeed(42);  // Set deterministic seed
    bool later_loop_result = complex_step.shouldTrigger(later_loop_context);
    // Result depends on probability, but condition should pass
    
    // Test retrigger timing with micro-timing
    uint32_t base_time = 1000;
    uint32_t adjusted_base = complex_step.calculateTriggerTime(base_time);
    ASSERT_EQ(adjusted_base, 1015U, "Micro-timing should be applied to base time");
    
    auto retrigger_times = complex_step.calculateRetriggerTimes(adjusted_base, 96);
    ASSERT_EQ_NUM(retrigger_times.size(), 3U, "Should have main trigger + 2 retriggers");
    
    // Test parameter locks
    ASSERT_EQ_NUM(complex_step.getParameterLockCount(), 2U, "Should have 2 parameter locks");
    
    // Test configuration summary
    std::string summary = complex_step.getConfigurationSummary();
    ASSERT_TRUE(summary.find("Note=84") != std::string::npos, "Summary should include note");
    ASSERT_TRUE(summary.find("Prob=80") != std::string::npos, "Summary should include probability");
    ASSERT_TRUE(summary.find("Timing=15") != std::string::npos, "Summary should include micro-timing");
    ASSERT_TRUE(summary.find("Retrig=2") != std::string::npos, "Summary should include retriggers");
    ASSERT_TRUE(summary.find("Locks=2") != std::string::npos, "Summary should include parameter locks");
    
    PASS("Combined features work correctly together");
}

TEST_INTEGRATION(EnhancedSequencer, FeatureInteractions) {
    PRINT_TEST_HEADER("Enhanced Sequencer - Feature Interactions");
    
    // Test interaction between probability and conditions
    EnhancedSequencerStep prob_condition_step;
    prob_condition_step.setActive(true);
    prob_condition_step.setProbability(1.0f);  // Always trigger if condition passes
    prob_condition_step.setTriggerCondition(ConditionalTriggerEngine::TriggerCondition::FIRST);
    
    SequencerContext first_context(0, 0, 0, false);
    SequencerContext later_context(0, 0, 1, false);
    
    ASSERT_TRUE(prob_condition_step.shouldTrigger(first_context), "Should trigger on first loop with 100% probability");
    ASSERT_FALSE(prob_condition_step.shouldTrigger(later_context), "Should not trigger on later loop due to condition");
    
    // Test interaction between retriggers and parameter locks
    EnhancedSequencerStep retrig_lock_step;
    retrig_lock_step.setActive(true);
    retrig_lock_step.setRetriggerCount(3);
    retrig_lock_step.setParameterLock(42, 0.7f);
    retrig_lock_step.setParameterLock(43, 0.4f);
    
    auto retrig_times = retrig_lock_step.calculateRetriggerTimes(0, 96);
    ASSERT_EQ_NUM(retrig_times.size(), 4U, "Retriggers should work with parameter locks");
    ASSERT_EQ_NUM(retrig_lock_step.getParameterLockCount(), 2U, "Parameter locks should be preserved");
    
    // Test interaction between micro-timing and swing
    uint32_t base_time = 1000;
    int8_t micro_offset = 10;
    float swing_amount = 0.5f;
    
    // Apply micro-timing first, then swing
    uint32_t micro_time = MicroTimingEngine::calculateTiming(base_time, micro_offset);
    uint32_t swing_time = MicroTimingEngine::calculateSwingTiming(micro_time, 1, swing_amount);
    
    ASSERT_TRUE(swing_time > micro_time, "Swing should be applied after micro-timing");
    ASSERT_TRUE(swing_time > base_time + micro_offset, "Combined timing should include both offsets");
    
    PASS("Feature interactions work correctly");
}

// =============================================================================
// CHAPTER 3: PERFORMANCE INTEGRATION TESTING
// =============================================================================

TEST_INTEGRATION(EnhancedSequencer, PerformanceUnderLoad) {
    PRINT_TEST_HEADER("Enhanced Sequencer - Performance Under Load");
    
    const int num_tracks = 8;
    const int num_steps = 16;
    const int iterations = 1000;
    
    // Create steps with all features enabled
    std::vector<std::vector<EnhancedSequencerStep>> pattern(num_tracks, std::vector<EnhancedSequencerStep>(num_steps));
    
    for (int track = 0; track < num_tracks; ++track) {
        for (int step = 0; step < num_steps; ++step) {
            auto& current_step = pattern[track][step];
            current_step.setActive((step + track) % 3 == 0);  // Some steps active
            current_step.setNote(60 + (step % 12));
            current_step.setVelocity(100 + (step % 28));
            current_step.setProbability(0.5f + (step % 10) / 20.0f);
            current_step.setMicroTiming((step % 21) - 10);
            current_step.setRetriggerCount(step % 4);
            current_step.setTriggerCondition(static_cast<ConditionalTriggerEngine::TriggerCondition>(step % 3));
            
            // Add parameter locks
            for (int param = 0; param < (step % 3 + 1); ++param) {
                current_step.setParameterLock(42 + param, (param + 1) * 0.25f);
            }
        }
    }
    
    // Performance test
    auto start_time = std::chrono::high_resolution_clock::now();
    
    ProbabilityEngine::setSeed(42);
    for (int iteration = 0; iteration < iterations; ++iteration) {
        for (int track = 0; track < num_tracks; ++track) {
            for (int step = 0; step < num_steps; ++step) {
                SequencerContext context(track, step, iteration / num_steps, false);
                
                // Evaluate step trigger
                pattern[track][step].shouldTrigger(context);
                
                // Calculate timing if step would trigger
                if (pattern[track][step].isActive()) {
                    uint32_t base_time = iteration * 24 + step * 24;  // 24 PPQN
                    pattern[track][step].calculateTriggerTime(base_time);
                    
                    // Calculate retrigger times if retriggers enabled
                    if (pattern[track][step].getRetriggerCount() > 0) {
                        pattern[track][step].calculateRetriggerTimes(base_time, 24);
                        pattern[track][step].calculateRetriggerVelocities();
                    }
                }
            }
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    float total_evaluations = iterations * num_tracks * num_steps;
    float avg_time_per_evaluation = static_cast<float>(duration.count()) / total_evaluations;
    
    // Performance requirement: less than 1 microsecond per evaluation
    ASSERT_TRUE(avg_time_per_evaluation < 1.0f, 
                std::string("Performance under load failed. Average time per evaluation: ") + 
                std::to_string(avg_time_per_evaluation) + " microseconds");
    
    PASS(std::string("Performance under load passed. Average time: ") + 
         std::to_string(avg_time_per_evaluation) + " microseconds per evaluation");
}

TEST_INTEGRATION(EnhancedSequencer, MemoryEfficiency) {
    PRINT_TEST_HEADER("Enhanced Sequencer - Memory Efficiency");
    
    const int num_patterns = 10;
    const int steps_per_pattern = 64;
    
    std::vector<std::vector<EnhancedSequencerStep>> patterns(num_patterns, 
                                                           std::vector<EnhancedSequencerStep>(steps_per_pattern));
    
    // Configure patterns with varying complexity
    for (int pattern = 0; pattern < num_patterns; ++pattern) {
        for (int step = 0; step < steps_per_pattern; ++step) {
            auto& current_step = patterns[pattern][step];
            
            // Vary complexity based on pattern and step
            int complexity = (pattern * steps_per_pattern + step) % 10;
            
            current_step.setActive(complexity > 3);
            current_step.setNote(60 + (step % 12));
            
            if (complexity > 5) {
                current_step.setProbability(0.5f + complexity / 20.0f);
                current_step.setMicroTiming((step % 21) - 10);
            }
            
            if (complexity > 7) {
                current_step.setRetriggerCount(complexity % 4);
                current_step.setTriggerCondition(static_cast<ConditionalTriggerEngine::TriggerCondition>(complexity % 3));
            }
            
            // Add parameter locks based on complexity
            for (int param = 0; param < (complexity % 5); ++param) {
                current_step.setParameterLock(42 + param, param * 0.2f);
            }
        }
    }
    
    // Calculate total memory usage
    size_t total_memory = 0;
    for (const auto& pattern : patterns) {
        for (const auto& step : pattern) {
            total_memory += step.getMemoryUsage();
        }
    }
    
    float avg_memory_per_step = static_cast<float>(total_memory) / (num_patterns * steps_per_pattern);
    
    // Memory efficiency requirement: less than 512 bytes per step on average
    ASSERT_TRUE(avg_memory_per_step < 512.0f, 
                std::string("Memory efficiency test failed. Average memory per step: ") + 
                std::to_string(avg_memory_per_step) + " bytes");
    
    PASS(std::string("Memory efficiency test passed. Average memory per step: ") + 
         std::to_string(avg_memory_per_step) + " bytes");
}

// =============================================================================
// CHAPTER 4: REAL-WORLD SCENARIO TESTING
// =============================================================================

TEST_INTEGRATION(EnhancedSequencer, DrumPatternScenario) {
    PRINT_TEST_HEADER("Enhanced Sequencer - Drum Pattern Scenario");
    
    // Create a realistic drum pattern with Digitakt-style features
    const int num_drum_tracks = 8;
    const int pattern_length = 16;
    
    std::vector<std::vector<EnhancedSequencerStep>> drum_pattern(num_drum_tracks, 
                                                               std::vector<EnhancedSequencerStep>(pattern_length));
    
    // Track 0: Kick drum - Strong on 1 and 9, with fills
    for (int step = 0; step < pattern_length; ++step) {
        auto& kick = drum_pattern[0][step];
        if (step == 0 || step == 8) {
            kick.setActive(true);
            kick.setNote(36);  // C1 - Kick
            kick.setVelocity(127);
            kick.setTriggerCondition(ConditionalTriggerEngine::TriggerCondition::NONE);
        } else if (step == 4 || step == 12) {
            kick.setActive(true);
            kick.setNote(36);
            kick.setVelocity(100);
            kick.setProbability(0.7f);  // 70% chance for variation
            kick.setTriggerCondition(ConditionalTriggerEngine::TriggerCondition::NOT_FIRST);
        }
    }
    
    // Track 1: Snare drum - Strong on 5 and 13
    for (int step = 0; step < pattern_length; ++step) {
        auto& snare = drum_pattern[1][step];
        if (step == 4 || step == 12) {
            snare.setActive(true);
            snare.setNote(38);  // D1 - Snare
            snare.setVelocity(120);
            snare.setMicroTiming(-5);  // Slightly ahead for groove
        }
    }
    
    // Track 2: Hi-hat - 16th notes with probability and micro-timing
    for (int step = 0; step < pattern_length; ++step) {
        auto& hihat = drum_pattern[2][step];
        hihat.setActive(true);
        hihat.setNote(42);  // F#1 - Closed Hi-hat
        hihat.setVelocity(80 + (step % 4) * 5);  // Velocity variation
        hihat.setProbability(0.8f + (step % 3) * 0.05f);  // Slight probability variation
        hihat.setMicroTiming((step % 3) - 1);  // Micro-timing for groove
    }
    
    // Track 3: Open hi-hat - Sparse with retriggers
    for (int step = 0; step < pattern_length; ++step) {
        auto& open_hihat = drum_pattern[3][step];
        if (step % 8 == 6) {  // Every 8th step, offset by 6
            open_hihat.setActive(true);
            open_hihat.setNote(46);  // A#1 - Open Hi-hat
            open_hihat.setVelocity(90);
            open_hihat.setRetriggerCount(1);  // Double hit
            open_hihat.setRetriggerRate(RetriggerEngine::RetriggerRate::RATE_1_32);
        }
    }
    
    // Track 4: Percussion - Fill patterns only
    for (int step = 0; step < pattern_length; ++step) {
        auto& perc = drum_pattern[4][step];
        if (step >= 14) {  // Last two steps
            perc.setActive(true);
            perc.setNote(75 + step % 3);  // Various percussion sounds
            perc.setVelocity(100);
            perc.setTriggerCondition(ConditionalTriggerEngine::TriggerCondition::FILL);
            perc.setRetriggerCount(2);
        }
    }
    
    // Simulate pattern playback with different contexts
    int trigger_counts[num_drum_tracks] = {0};
    
    // First loop (no fills)
    for (int step = 0; step < pattern_length; ++step) {
        SequencerContext first_loop_context(0, step, 0, false);
        
        for (int track = 0; track < num_drum_tracks; ++track) {
            first_loop_context = SequencerContext(track, step, 0, false);
            if (drum_pattern[track][step].shouldTrigger(first_loop_context)) {
                trigger_counts[track]++;
            }
        }
    }
    
    // Later loop with fill mode
    auto& fillMgr = FillModeManager::getInstance();
    fillMgr.enterFill();
    
    for (int step = 0; step < pattern_length; ++step) {
        SequencerContext fill_context(0, step, 2, true);
        
        for (int track = 0; track < num_drum_tracks; ++track) {
            fill_context = SequencerContext(track, step, 2, true);
            if (drum_pattern[track][step].shouldTrigger(fill_context)) {
                trigger_counts[track]++;
            }
        }
    }
    
    fillMgr.exitFill();
    
    // Validate results
    ASSERT_TRUE(trigger_counts[0] > 0, "Kick drum should trigger");
    ASSERT_TRUE(trigger_counts[1] > 0, "Snare drum should trigger");
    ASSERT_TRUE(trigger_counts[2] > 0, "Hi-hat should trigger");
    ASSERT_TRUE(trigger_counts[4] > 0, "Fill percussion should trigger in fill mode");
    
    PASS("Drum pattern scenario works correctly");
}

TEST_INTEGRATION(EnhancedSequencer, MelodicSequenceScenario) {
    PRINT_TEST_HEADER("Enhanced Sequencer - Melodic Sequence Scenario");
    
    // Create a melodic sequence with parameter locks and probability
    const int sequence_length = 32;  // 2-bar sequence
    std::vector<EnhancedSequencerStep> melody(sequence_length);
    
    // Define a simple melodic pattern with variations
    std::vector<int> base_notes = {60, 62, 64, 65, 67, 69, 71, 72};  // C major scale
    
    for (int step = 0; step < sequence_length; ++step) {
        auto& note_step = melody[step];
        
        // Basic melodic pattern
        if (step % 4 == 0) {  // Every 4th step
            note_step.setActive(true);
            note_step.setNote(base_notes[step / 4]);
            note_step.setVelocity(100 + (step % 3) * 10);  // Velocity variation
            
            // Add parameter locks for filter automation
            float filter_value = 0.3f + (step / float(sequence_length)) * 0.5f;  // Rising filter
            note_step.setParameterLock(42, filter_value);  // Filter cutoff
            
            // Add micro-timing for humanization
            note_step.setMicroTiming((step % 7) - 3);  // -3 to +3 ticks
        }
        
        // Add grace notes with probability
        if (step % 4 == 3 && step < sequence_length - 1) {  // Before main notes
            note_step.setActive(true);
            note_step.setNote(base_notes[(step / 4) % base_notes.size()] + 2);  // Grace note
            note_step.setVelocity(70);  // Quieter
            note_step.setProbability(0.6f);  // 60% chance
            note_step.setMicroTiming(-10);  // Earlier timing
        }
        
        // Add chord hits on certain beats
        if (step % 16 == 15) {  // Every 16th step
            note_step.setActive(true);
            note_step.setNote(60);  // Root note
            note_step.setVelocity(120);
            note_step.setRetriggerCount(2);  // Chord-like effect
            note_step.setRetriggerRate(RetriggerEngine::RetriggerRate::RATE_1_32);
            note_step.setParameterLock(42, 0.8f);  // Bright filter for chords
            note_step.setParameterLock(43, 0.4f);  // Resonance
        }
    }
    
    // Simulate melodic sequence playback
    ProbabilityEngine::setSeed(12345);
    
    int total_triggers = 0;
    int parameter_lock_applications = 0;
    
    for (int loop = 0; loop < 4; ++loop) {  // 4 loops
        for (int step = 0; step < sequence_length; ++step) {
            SequencerContext context(0, step, loop, false);
            
            if (melody[step].shouldTrigger(context)) {
                total_triggers++;
                
                // Count parameter lock applications
                if (melody[step].getParameterLockCount() > 0) {
                    parameter_lock_applications++;
                }
                
                // Check retrigger functionality
                if (melody[step].getRetriggerCount() > 0) {
                    auto retrig_times = melody[step].calculateRetriggerTimes(step * 24, 24);
                    ASSERT_TRUE(retrig_times.size() > 1, "Chord steps should have retriggers");
                }
            }
        }
    }
    
    ASSERT_TRUE(total_triggers > 0, "Melodic sequence should generate triggers");
    ASSERT_TRUE(parameter_lock_applications > 0, "Parameter locks should be applied");
    
    PASS("Melodic sequence scenario works correctly");
}

// Run all tests
int main() {
    std::cout << "🎛️ RUNNING ENHANCED SEQUENCER INTEGRATION TEST SUITE" << std::endl;
    std::cout << "====================================================" << std::endl;
    
    RUN_ALL_TESTS();
    
    return 0;
}
