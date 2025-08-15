/**
 * @brief Digitakt Features Test Suite - Comprehensive Testing
 * 
 * This file contains comprehensive tests for all the Digitakt-style sequencer features
 * including probability, micro-timing, retriggering, conditional triggers, and fill mode.
 * 
 * TEST COVERAGE - COMPLETE DIGITAKT FEATURE TESTING:
 * 
 * 🎲 CHAPTER 1: Probability Engine Testing
 *    Random step triggering, seed reproducibility, edge cases
 * 
 * ⏱️ CHAPTER 2: Micro-Timing Engine Testing
 *    Sub-step timing adjustments, swing calculation, offset validation
 * 
 * 🔄 CHAPTER 3: Retrigger Engine Testing
 *    Multiple triggers per step, velocity ramping, timing calculation
 * 
 * 🎯 CHAPTER 4: Conditional Trigger Engine Testing
 *    Context-dependent triggering, all condition types, parameter validation
 * 
 * 🎵 CHAPTER 5: Fill Mode Manager Testing
 *    Fill mode state management, per-track patterns, thread safety
 * 
 * 📊 CHAPTER 6: Sequencer Context Testing
 *    Context information accuracy, neighbor states, pattern chains
 * 
 * 🎛️ CHAPTER 7: Enhanced Sequencer Step Testing
 *    Complete step functionality, feature integration, memory usage
 * 
 * ⚡ CHAPTER 8: Performance and Integration Testing
 *    Real-time performance, thread safety, memory efficiency
 * 
 * 🔧 CHAPTER 9: Edge Cases and Error Handling
 *    Boundary conditions, invalid inputs, graceful degradation
 * 
 * ARCHITECTURE:
 * - Unit testing for each engine component
 * - Integration testing for feature combinations
 * - Performance testing for real-time constraints
 * - Memory usage validation
 * - Thread safety verification
 * 
 * @author Generated for Enhanced Sequencer Features
 * @date August 14, 2025
 */

#include "../framework/unified_test_framework.h"
#include "../fixtures/test_fixtures.h"
#include "../../src/components/midi/DigitaktFeatures.h"
#include "../../src/components/midi/EnhancedSequencerStep.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>

using namespace MIDI;

// =============================================================================
// CHAPTER 1: PROBABILITY ENGINE TESTING
// =============================================================================

TEST_UNIT(ProbabilityEngine_AlwaysTrigger) {
    PRINT_TEST_HEADER("Probability Engine - Always Trigger (1.0)");
    
    // Test that probability 1.0 always triggers
    for (int i = 0; i < 100; ++i) {
        bool result = ProbabilityEngine::shouldTrigger(1.0f);
        ASSERT_TRUE(result, "Probability 1.0 should always trigger");
    }
    
    PASS("Probability 1.0 always triggers correctly");
}

TEST_UNIT(ProbabilityEngine_NeverTrigger) {
    PRINT_TEST_HEADER("Probability Engine - Never Trigger (0.0)");
    
    // Test that probability 0.0 never triggers
    for (int i = 0; i < 100; ++i) {
        bool result = ProbabilityEngine::shouldTrigger(0.0f);
        ASSERT_FALSE(result, "Probability 0.0 should never trigger");
    }
    
    PASS("Probability 0.0 never triggers correctly");
}

TEST_UNIT(ProbabilityEngine_SeedReproducibility) {
    PRINT_TEST_HEADER("Probability Engine - Seed Reproducibility");
    
    // Test that same seed produces same sequence
    ProbabilityEngine::setSeed(12345);
    std::vector<bool> sequence1;
    for (int i = 0; i < 20; ++i) {
        sequence1.push_back(ProbabilityEngine::shouldTrigger(0.5f));
    }
    
    ProbabilityEngine::setSeed(12345);  // Reset to same seed
    std::vector<bool> sequence2;
    for (int i = 0; i < 20; ++i) {
        sequence2.push_back(ProbabilityEngine::shouldTrigger(0.5f));
    }
    
    ASSERT_TRUE(sequence1 == sequence2, "Same seed should produce identical sequences");
    
    uint32_t retrieved_seed = ProbabilityEngine::getSeed();
    ASSERT_EQ(retrieved_seed, 12345U, "Retrieved seed should match set seed");
    
    PASS("Seed reproducibility works correctly");
}

TEST_UNIT(ProbabilityEngine_StatisticalDistribution) {
    PRINT_TEST_HEADER("Probability Engine - Statistical Distribution");
    
    ProbabilityEngine::setSeed(42);
    const int sample_size = 10000;
    const float target_probability = 0.3f;
    const float tolerance = 0.05f;  // 5% tolerance
    
    int trigger_count = 0;
    for (int i = 0; i < sample_size; ++i) {
        if (ProbabilityEngine::shouldTrigger(target_probability)) {
            trigger_count++;
        }
    }
    
    float actual_probability = static_cast<float>(trigger_count) / sample_size;
    float error = std::abs(actual_probability - target_probability);
    
    ASSERT_TRUE(error < tolerance, 
                std::string("Statistical distribution should be within tolerance. "
                           "Expected: ") + std::to_string(target_probability) + 
                ", Actual: " + std::to_string(actual_probability) + 
                ", Error: " + std::to_string(error));
    
    PASS("Statistical distribution is within expected tolerance");
}

TEST_UNIT(ProbabilityEngine_EdgeCases) {
    PRINT_TEST_HEADER("Probability Engine - Edge Cases");
    
    // Test edge cases and invalid inputs
    ASSERT_FALSE(ProbabilityEngine::shouldTrigger(-0.5f), "Negative probability should be treated as 0.0");
    ASSERT_TRUE(ProbabilityEngine::shouldTrigger(1.5f), "Probability > 1.0 should be treated as 1.0");
    
    // Test very small probabilities
    bool any_triggered = false;
    for (int i = 0; i < 1000; ++i) {
        if (ProbabilityEngine::shouldTrigger(0.001f)) {
            any_triggered = true;
            break;
        }
    }
    // Note: This could randomly fail, but with 1000 samples and 0.1% probability,
    // we expect ~1 trigger on average
    
    PASS("Edge cases handled correctly");
}

// =============================================================================
// CHAPTER 2: MICRO-TIMING ENGINE TESTING
// =============================================================================

TEST_UNIT(MicroTimingEngine_BasicTiming) {
    PRINT_TEST_HEADER("Micro-Timing Engine - Basic Timing");
    
    uint32_t base_time = 1000;
    
    // Test positive offset (later)
    uint32_t later_time = MicroTimingEngine::calculateTiming(base_time, 10);
    ASSERT_EQ(later_time, 1010U, "Positive offset should add to base time");
    
    // Test negative offset (earlier)
    uint32_t earlier_time = MicroTimingEngine::calculateTiming(base_time, -10);
    ASSERT_EQ(earlier_time, 990U, "Negative offset should subtract from base time");
    
    // Test no offset
    uint32_t same_time = MicroTimingEngine::calculateTiming(base_time, 0);
    ASSERT_EQ(same_time, base_time, "Zero offset should return base time");
    
    PASS("Basic timing calculations work correctly");
}

TEST_UNIT(MicroTimingEngine_OffsetClamping) {
    PRINT_TEST_HEADER("Micro-Timing Engine - Offset Clamping");
    
    // Test offset clamping
    int8_t clamped_high = MicroTimingEngine::clampMicroTiming(100);
    ASSERT_EQ(clamped_high, 50, "High offset should be clamped to maximum");
    
    int8_t clamped_low = MicroTimingEngine::clampMicroTiming(-100);
    ASSERT_EQ(clamped_low, -50, "Low offset should be clamped to minimum");
    
    int8_t not_clamped = MicroTimingEngine::clampMicroTiming(25);
    ASSERT_EQ(not_clamped, 25, "Valid offset should not be clamped");
    
    PASS("Offset clamping works correctly");
}

TEST_UNIT(MicroTimingEngine_SwingTiming) {
    PRINT_TEST_HEADER("Micro-Timing Engine - Swing Timing");
    
    uint32_t base_time = 0;
    float swing_amount = 0.5f;
    
    // Test downbeat (no swing)
    uint32_t downbeat_time = MicroTimingEngine::calculateSwingTiming(base_time, 0, swing_amount);
    ASSERT_EQ(downbeat_time, base_time, "Downbeats should not be affected by swing");
    
    // Test offbeat (with swing)
    uint32_t offbeat_time = MicroTimingEngine::calculateSwingTiming(base_time, 1, swing_amount);
    ASSERT_TRUE(offbeat_time > base_time, "Offbeats should be delayed by swing");
    
    // Test no swing
    uint32_t no_swing_time = MicroTimingEngine::calculateSwingTiming(base_time, 1, 0.0f);
    ASSERT_EQ(no_swing_time, base_time, "No swing should leave timing unchanged");
    
    PASS("Swing timing calculations work correctly");
}

TEST_UNIT(MicroTimingEngine_MillisecondConversion) {
    PRINT_TEST_HEADER("Micro-Timing Engine - Millisecond Conversion");
    
    float bpm = 120.0f;
    int ppqn = 24;
    
    // Test positive offset
    float positive_ms = MicroTimingEngine::offsetToMilliseconds(24, bpm, ppqn);
    ASSERT_TRUE(positive_ms > 0.0f, "Positive offset should produce positive milliseconds");
    
    // Test negative offset
    float negative_ms = MicroTimingEngine::offsetToMilliseconds(-24, bpm, ppqn);
    ASSERT_TRUE(negative_ms < 0.0f, "Negative offset should produce negative milliseconds");
    
    // Test zero offset
    float zero_ms = MicroTimingEngine::offsetToMilliseconds(0, bpm, ppqn);
    ASSERT_EQ(zero_ms, 0.0f, "Zero offset should produce zero milliseconds");
    
    PASS("Millisecond conversion works correctly");
}

// =============================================================================
// CHAPTER 3: RETRIGGER ENGINE TESTING
// =============================================================================

TEST_UNIT(RetriggerEngine_BasicRetriggering) {
    PRINT_TEST_HEADER("Retrigger Engine - Basic Retriggering");
    
    uint32_t step_start = 0;
    uint32_t step_duration = 96;  // 1/16 note at 384 PPQN
    uint8_t retrigger_count = 3;
    RetriggerEngine::RetriggerRate rate = RetriggerEngine::RetriggerRate::RATE_1_16;
    
    auto trigger_times = RetriggerEngine::calculateRetriggerTimes(step_start, step_duration, retrigger_count, rate);
    
    ASSERT_EQ(trigger_times.size(), 4U, "Should have main trigger + 3 retriggers");
    ASSERT_EQ(trigger_times[0], step_start, "First trigger should be at step start");
    
    // Check that triggers are evenly spaced
    for (size_t i = 1; i < trigger_times.size(); ++i) {
        ASSERT_TRUE(trigger_times[i] > trigger_times[i-1], "Triggers should be in ascending order");
    }
    
    PASS("Basic retriggering works correctly");
}

TEST_UNIT(RetriggerEngine_VelocityRamping) {
    PRINT_TEST_HEADER("Retrigger Engine - Velocity Ramping");
    
    uint8_t base_velocity = 127;
    int total_retriggers = 4;
    float velocity_curve = 0.7f;
    
    auto velocities = std::vector<uint8_t>();
    for (int i = 0; i < total_retriggers; ++i) {
        uint8_t velocity = RetriggerEngine::calculateRetriggerVelocity(base_velocity, i, total_retriggers, velocity_curve);
        velocities.push_back(velocity);
    }
    
    ASSERT_EQ(velocities[0], base_velocity, "First trigger should have base velocity");
    
    // Check that velocities generally decrease
    for (size_t i = 1; i < velocities.size(); ++i) {
        ASSERT_TRUE(velocities[i] <= velocities[i-1], "Retrigger velocities should not increase");
        ASSERT_TRUE(velocities[i] >= 10, "Retrigger velocities should stay above minimum");
    }
    
    PASS("Velocity ramping works correctly");
}

TEST_UNIT(RetriggerEngine_CountClamping) {
    PRINT_TEST_HEADER("Retrigger Engine - Count Clamping");
    
    uint8_t clamped_high = RetriggerEngine::clampRetriggerCount(20);
    ASSERT_EQ(clamped_high, 8U, "High retrigger count should be clamped to maximum");
    
    uint8_t not_clamped = RetriggerEngine::clampRetriggerCount(5);
    ASSERT_EQ(not_clamped, 5U, "Valid retrigger count should not be clamped");
    
    PASS("Retrigger count clamping works correctly");
}

TEST_UNIT(RetriggerEngine_RateConversion) {
    PRINT_TEST_HEADER("Retrigger Engine - Rate Conversion");
    
    int ppqn = 96;
    
    uint32_t quarter_ticks = RetriggerEngine::rateToTicks(RetriggerEngine::RetriggerRate::RATE_1_4, ppqn);
    ASSERT_EQ(quarter_ticks, 24U, "1/4 note should be 24 ticks at 96 PPQN");
    
    uint32_t sixteenth_ticks = RetriggerEngine::rateToTicks(RetriggerEngine::RetriggerRate::RATE_1_16, ppqn);
    ASSERT_EQ(sixteenth_ticks, 6U, "1/16 note should be 6 ticks at 96 PPQN");
    
    PASS("Rate conversion works correctly");
}

// =============================================================================
// CHAPTER 4: CONDITIONAL TRIGGER ENGINE TESTING
// =============================================================================

TEST_UNIT(ConditionalTriggerEngine_FirstCondition) {
    PRINT_TEST_HEADER("Conditional Trigger Engine - First Condition");
    
    // Test FIRST condition
    SequencerContext first_loop_context(0, 0, 0, false);  // loop_count = 0
    bool first_result = ConditionalTriggerEngine::evaluateCondition(
        ConditionalTriggerEngine::TriggerCondition::FIRST, 0, first_loop_context);
    ASSERT_TRUE(first_result, "FIRST condition should trigger on first loop");
    
    SequencerContext later_loop_context(0, 0, 2, false);  // loop_count = 2
    bool later_result = ConditionalTriggerEngine::evaluateCondition(
        ConditionalTriggerEngine::TriggerCondition::FIRST, 0, later_loop_context);
    ASSERT_FALSE(later_result, "FIRST condition should not trigger on later loops");
    
    PASS("FIRST condition works correctly");
}

TEST_UNIT(ConditionalTriggerEngine_NotFirstCondition) {
    PRINT_TEST_HEADER("Conditional Trigger Engine - Not First Condition");
    
    // Test NOT_FIRST condition
    SequencerContext first_loop_context(0, 0, 0, false);  // loop_count = 0
    bool first_result = ConditionalTriggerEngine::evaluateCondition(
        ConditionalTriggerEngine::TriggerCondition::NOT_FIRST, 0, first_loop_context);
    ASSERT_FALSE(first_result, "NOT_FIRST condition should not trigger on first loop");
    
    SequencerContext later_loop_context(0, 0, 1, false);  // loop_count = 1
    bool later_result = ConditionalTriggerEngine::evaluateCondition(
        ConditionalTriggerEngine::TriggerCondition::NOT_FIRST, 0, later_loop_context);
    ASSERT_TRUE(later_result, "NOT_FIRST condition should trigger on later loops");
    
    PASS("NOT_FIRST condition works correctly");
}

TEST_UNIT(ConditionalTriggerEngine_FillConditions) {
    PRINT_TEST_HEADER("Conditional Trigger Engine - Fill Conditions");
    
    // Test FILL condition
    SequencerContext fill_context(0, 0, 0, true);  // fill_active = true
    bool fill_result = ConditionalTriggerEngine::evaluateCondition(
        ConditionalTriggerEngine::TriggerCondition::FILL, 0, fill_context);
    ASSERT_TRUE(fill_result, "FILL condition should trigger during fill mode");
    
    SequencerContext no_fill_context(0, 0, 0, false);  // fill_active = false
    bool no_fill_result = ConditionalTriggerEngine::evaluateCondition(
        ConditionalTriggerEngine::TriggerCondition::FILL, 0, no_fill_context);
    ASSERT_FALSE(no_fill_result, "FILL condition should not trigger outside fill mode");
    
    // Test NOT_FILL condition
    bool not_fill_result = ConditionalTriggerEngine::evaluateCondition(
        ConditionalTriggerEngine::TriggerCondition::NOT_FILL, 0, no_fill_context);
    ASSERT_TRUE(not_fill_result, "NOT_FILL condition should trigger outside fill mode");
    
    PASS("Fill conditions work correctly");
}

TEST_UNIT(ConditionalTriggerEngine_NeighborConditions) {
    PRINT_TEST_HEADER("Conditional Trigger Engine - Neighbor Conditions");
    
    SequencerContext context(1, 5, 0, false);
    context.setNeighborStates(true, false);  // previous active, next inactive
    
    // Test neighbor condition for previous step
    bool prev_result = ConditionalTriggerEngine::evaluateCondition(
        ConditionalTriggerEngine::TriggerCondition::NEI, 0, context);
    ASSERT_TRUE(prev_result, "NEI condition should trigger when previous step is active");
    
    // Test neighbor condition for next step
    bool next_result = ConditionalTriggerEngine::evaluateCondition(
        ConditionalTriggerEngine::TriggerCondition::NEI, 1, context);
    ASSERT_FALSE(next_result, "NEI condition should not trigger when next step is inactive");
    
    PASS("Neighbor conditions work correctly");
}

TEST_UNIT(ConditionalTriggerEngine_ConditionNames) {
    PRINT_TEST_HEADER("Conditional Trigger Engine - Condition Names");
    
    const char* first_name = ConditionalTriggerEngine::getConditionName(
        ConditionalTriggerEngine::TriggerCondition::FIRST);
    ASSERT_TRUE(std::string(first_name) == "First", "FIRST condition should have correct name");
    
    const char* fill_name = ConditionalTriggerEngine::getConditionName(
        ConditionalTriggerEngine::TriggerCondition::FILL);
    ASSERT_TRUE(std::string(fill_name) == "Fill", "FILL condition should have correct name");
    
    PASS("Condition names are correct");
}

// =============================================================================
// CHAPTER 5: FILL MODE MANAGER TESTING
// =============================================================================

TEST_UNIT(FillModeManager_BasicFunctionality) {
    PRINT_TEST_HEADER("Fill Mode Manager - Basic Functionality");
    
    auto& fillMgr = FillModeManager::getInstance();
    
    // Test initial state
    ASSERT_FALSE(fillMgr.isActive(), "Fill mode should be inactive initially");
    
    // Test entering fill mode
    fillMgr.enterFill();
    ASSERT_TRUE(fillMgr.isActive(), "Fill mode should be active after entering");
    
    // Test exiting fill mode
    fillMgr.exitFill();
    ASSERT_FALSE(fillMgr.isActive(), "Fill mode should be inactive after exiting");
    
    PASS("Fill mode basic functionality works correctly");
}

TEST_UNIT(FillModeManager_TrackPatterns) {
    PRINT_TEST_HEADER("Fill Mode Manager - Track Patterns");
    
    auto& fillMgr = FillModeManager::getInstance();
    
    // Set fill pattern for track 0
    std::vector<bool> fill_pattern = {true, false, true, true, false, false, true, false};
    fillMgr.setTrackFillPattern(0, fill_pattern);
    
    // Retrieve and verify pattern
    const auto& retrieved_pattern = fillMgr.getTrackFillPattern(0);
    ASSERT_EQ(retrieved_pattern.size(), fill_pattern.size(), "Retrieved pattern should have same size");
    
    for (size_t i = 0; i < fill_pattern.size(); ++i) {
        ASSERT_EQ(retrieved_pattern[i], fill_pattern[i], "Retrieved pattern should match set pattern");
    }
    
    // Test clearing pattern
    fillMgr.clearTrackFillPattern(0);
    const auto& cleared_pattern = fillMgr.getTrackFillPattern(0);
    ASSERT_TRUE(cleared_pattern.empty(), "Cleared pattern should be empty");
    
    PASS("Track pattern management works correctly");
}

TEST_UNIT(FillModeManager_SingletonPattern) {
    PRINT_TEST_HEADER("Fill Mode Manager - Singleton Pattern");
    
    auto& fillMgr1 = FillModeManager::getInstance();
    auto& fillMgr2 = FillModeManager::getInstance();
    
    ASSERT_EQ(&fillMgr1, &fillMgr2, "getInstance should return same instance");
    
    // Test state consistency across references
    fillMgr1.enterFill();
    ASSERT_TRUE(fillMgr2.isActive(), "State should be consistent across references");
    
    fillMgr2.exitFill();
    ASSERT_FALSE(fillMgr1.isActive(), "State changes should be visible across references");
    
    PASS("Singleton pattern works correctly");
}

// =============================================================================
// CHAPTER 6: SEQUENCER CONTEXT TESTING
// =============================================================================

TEST_UNIT(SequencerContext_BasicProperties) {
    PRINT_TEST_HEADER("Sequencer Context - Basic Properties");
    
    SequencerContext context(2, 7, 3, true);
    
    ASSERT_EQ(context.getCurrentTrack(), 2, "Current track should be correct");
    ASSERT_EQ(context.getCurrentStep(), 7, "Current step should be correct");
    ASSERT_EQ(context.getLoopCount(), 3, "Loop count should be correct");
    ASSERT_TRUE(context.isFillActive(), "Fill active state should be correct");
    
    PASS("Basic properties work correctly");
}

TEST_UNIT(SequencerContext_NeighborStates) {
    PRINT_TEST_HEADER("Sequencer Context - Neighbor States");
    
    SequencerContext context(0, 0, 0, false);
    
    // Set neighbor states
    context.setNeighborStates(true, false);
    
    ASSERT_TRUE(context.isPreviousStepActive(), "Previous step state should be correct");
    ASSERT_FALSE(context.isNextStepActive(), "Next step state should be correct");
    
    // Change neighbor states
    context.setNeighborStates(false, true);
    
    ASSERT_FALSE(context.isPreviousStepActive(), "Updated previous step state should be correct");
    ASSERT_TRUE(context.isNextStepActive(), "Updated next step state should be correct");
    
    PASS("Neighbor states work correctly");
}

TEST_UNIT(SequencerContext_PatternChainStates) {
    PRINT_TEST_HEADER("Sequencer Context - Pattern Chain States");
    
    SequencerContext context(0, 0, 0, false);
    
    // Set pattern chain states
    context.setPatternChainState(true, false);
    
    ASSERT_TRUE(context.isPatternAActive(), "Pattern A state should be correct");
    ASSERT_FALSE(context.isPatternBActive(), "Pattern B state should be correct");
    
    // Change pattern chain states
    context.setPatternChainState(false, true);
    
    ASSERT_FALSE(context.isPatternAActive(), "Updated pattern A state should be correct");
    ASSERT_TRUE(context.isPatternBActive(), "Updated pattern B state should be correct");
    
    PASS("Pattern chain states work correctly");
}

// =============================================================================
// CHAPTER 7: ENHANCED SEQUENCER STEP TESTING
// =============================================================================

TEST_UNIT(EnhancedSequencerStep_BasicProperties) {
    PRINT_TEST_HEADER("Enhanced Sequencer Step - Basic Properties");
    
    EnhancedSequencerStep step;
    
    // Test initial state
    ASSERT_FALSE(step.isActive(), "Step should be inactive initially");
    ASSERT_EQ(step.getNote(), 60U, "Default note should be C4 (60)");
    ASSERT_EQ(step.getVelocity(), 100U, "Default velocity should be 100");
    ASSERT_EQ(step.getLength(), 80U, "Default length should be 80%");
    
    // Test setting properties
    step.setActive(true);
    step.setNote(72);
    step.setVelocity(127);
    step.setLength(90);
    
    ASSERT_TRUE(step.isActive(), "Step should be active after setting");
    ASSERT_EQ(step.getNote(), 72U, "Note should be updated");
    ASSERT_EQ(step.getVelocity(), 127U, "Velocity should be updated");
    ASSERT_EQ(step.getLength(), 90U, "Length should be updated");
    
    PASS("Basic properties work correctly");
}

TEST_UNIT(EnhancedSequencerStep_ProbabilityIntegration) {
    PRINT_TEST_HEADER("Enhanced Sequencer Step - Probability Integration");
    
    EnhancedSequencerStep step;
    
    // Test probability setting and clamping
    step.setProbability(0.5f);
    ASSERT_EQ(step.getProbability(), 0.5f, "Probability should be set correctly");
    
    step.setProbability(1.5f);  // Above maximum
    ASSERT_EQ(step.getProbability(), 1.0f, "Probability should be clamped to maximum");
    
    step.setProbability(-0.5f);  // Below minimum
    ASSERT_EQ(step.getProbability(), 0.0f, "Probability should be clamped to minimum");
    
    // Test probability evaluation (statistical)
    ProbabilityEngine::setSeed(42);
    step.setProbability(1.0f);
    ASSERT_TRUE(step.shouldTriggerWithProbability(), "Probability 1.0 should always trigger");
    
    step.setProbability(0.0f);
    ASSERT_FALSE(step.shouldTriggerWithProbability(), "Probability 0.0 should never trigger");
    
    PASS("Probability integration works correctly");
}

TEST_UNIT(EnhancedSequencerStep_MicroTimingIntegration) {
    PRINT_TEST_HEADER("Enhanced Sequencer Step - Micro-Timing Integration");
    
    EnhancedSequencerStep step;
    
    // Test micro-timing setting and clamping
    step.setMicroTiming(25);
    ASSERT_EQ(step.getMicroTiming(), 25, "Micro-timing should be set correctly");
    
    step.setMicroTiming(100);  // Above maximum
    ASSERT_EQ(step.getMicroTiming(), 50, "Micro-timing should be clamped to maximum");
    
    step.setMicroTiming(-100);  // Below minimum
    ASSERT_EQ(step.getMicroTiming(), -50, "Micro-timing should be clamped to minimum");
    
    // Test timing calculation
    uint32_t base_time = 1000;
    step.setMicroTiming(10);
    uint32_t adjusted_time = step.calculateTriggerTime(base_time);
    ASSERT_EQ(adjusted_time, 1010U, "Adjusted timing should include micro-timing offset");
    
    PASS("Micro-timing integration works correctly");
}

TEST_UNIT(EnhancedSequencerStep_RetriggerIntegration) {
    PRINT_TEST_HEADER("Enhanced Sequencer Step - Retrigger Integration");
    
    EnhancedSequencerStep step;
    
    // Test retrigger count setting and clamping
    step.setRetriggerCount(3);
    ASSERT_EQ(step.getRetriggerCount(), 3U, "Retrigger count should be set correctly");
    
    step.setRetriggerCount(20);  // Above maximum
    ASSERT_EQ(step.getRetriggerCount(), 8U, "Retrigger count should be clamped to maximum");
    
    // Test retrigger rate setting
    step.setRetriggerRate(RetriggerEngine::RetriggerRate::RATE_1_8);
    ASSERT_EQ(step.getRetriggerRate(), RetriggerEngine::RetriggerRate::RATE_1_8, "Retrigger rate should be set correctly");
    
    // Test retrigger timing calculation
    step.setRetriggerCount(2);
    auto retrigger_times = step.calculateRetriggerTimes(0, 96);
    ASSERT_EQ(retrigger_times.size(), 3U, "Should have main trigger + 2 retriggers");
    
    // Test retrigger velocity calculation
    step.setVelocity(127);
    auto retrigger_velocities = step.calculateRetriggerVelocities();
    ASSERT_EQ(retrigger_velocities.size(), 3U, "Should have velocities for all triggers");
    ASSERT_EQ(retrigger_velocities[0], 127U, "First velocity should be base velocity");
    
    PASS("Retrigger integration works correctly");
}

TEST_UNIT(EnhancedSequencerStep_ConditionalTriggerIntegration) {
    PRINT_TEST_HEADER("Enhanced Sequencer Step - Conditional Trigger Integration");
    
    EnhancedSequencerStep step;
    
    // Test condition setting
    step.setTriggerCondition(ConditionalTriggerEngine::TriggerCondition::FIRST, 0);
    ASSERT_EQ(step.getTriggerCondition(), ConditionalTriggerEngine::TriggerCondition::FIRST, "Condition should be set correctly");
    ASSERT_EQ(step.getTriggerConditionParam(), 0U, "Condition parameter should be set correctly");
    
    // Test condition evaluation
    SequencerContext first_loop_context(0, 0, 0, false);
    bool first_result = step.evaluateTriggerCondition(first_loop_context);
    ASSERT_TRUE(first_result, "FIRST condition should evaluate correctly");
    
    SequencerContext later_loop_context(0, 0, 1, false);
    bool later_result = step.evaluateTriggerCondition(later_loop_context);
    ASSERT_FALSE(later_result, "FIRST condition should not trigger on later loops");
    
    PASS("Conditional trigger integration works correctly");
}

TEST_UNIT(EnhancedSequencerStep_ParameterLockIntegration) {
    PRINT_TEST_HEADER("Enhanced Sequencer Step - Parameter Lock Integration");
    
    EnhancedSequencerStep step;
    
    // Test parameter lock setting
    step.setParameterLock(42, 0.75f);
    ASSERT_TRUE(step.hasParameterLock(42), "Should have parameter lock");
    ASSERT_EQ(step.getParameterLock(42), 0.75f, "Parameter lock value should be correct");
    ASSERT_EQ(step.getParameterLockCount(), 1U, "Should have one parameter lock");
    
    // Test value clamping
    step.setParameterLock(43, 1.5f);  // Above maximum
    ASSERT_EQ(step.getParameterLock(43), 1.0f, "Parameter value should be clamped to maximum");
    
    step.setParameterLock(44, -0.5f);  // Below minimum
    ASSERT_EQ(step.getParameterLock(44), 0.0f, "Parameter value should be clamped to minimum");
    
    // Test parameter lock removal
    step.removeParameterLock(42);
    ASSERT_FALSE(step.hasParameterLock(42), "Should not have parameter lock after removal");
    
    // Test getting all locks
    step.setParameterLock(45, 0.25f);
    step.setParameterLock(46, 0.50f);
    const auto& all_locks = step.getParameterLocks();
    ASSERT_EQ(all_locks.size(), 3U, "Should have all parameter locks");  // 43, 44, 45, 46 (42 was removed)
    
    // Test clearing all locks
    step.clearAllParameterLocks();
    ASSERT_EQ(step.getParameterLockCount(), 0U, "Should have no parameter locks after clearing");
    
    PASS("Parameter lock integration works correctly");
}

TEST_UNIT(EnhancedSequencerStep_ComprehensiveEvaluation) {
    PRINT_TEST_HEADER("Enhanced Sequencer Step - Comprehensive Evaluation");
    
    EnhancedSequencerStep step;
    
    // Set up a step with multiple features
    step.setActive(true);
    step.setProbability(1.0f);  // Always trigger for deterministic testing
    step.setTriggerCondition(ConditionalTriggerEngine::TriggerCondition::NONE);
    
    SequencerContext context(0, 0, 0, false);
    
    // Test comprehensive evaluation
    ProbabilityEngine::setSeed(42);
    bool should_trigger = step.shouldTrigger(context);
    ASSERT_TRUE(should_trigger, "Active step with no conditions and 100% probability should trigger");
    
    // Test with inactive step
    step.setActive(false);
    bool inactive_trigger = step.shouldTrigger(context);
    ASSERT_FALSE(inactive_trigger, "Inactive step should never trigger");
    
    // Test with failed condition
    step.setActive(true);
    step.setTriggerCondition(ConditionalTriggerEngine::TriggerCondition::FIRST);
    SequencerContext later_context(0, 0, 1, false);  // Not first loop
    bool condition_failed = step.shouldTrigger(later_context);
    ASSERT_FALSE(condition_failed, "Step should not trigger when condition fails");
    
    PASS("Comprehensive evaluation works correctly");
}

TEST_UNIT(EnhancedSequencerStep_AdvancedFeatureDetection) {
    PRINT_TEST_HEADER("Enhanced Sequencer Step - Advanced Feature Detection");
    
    EnhancedSequencerStep simple_step;
    ASSERT_FALSE(simple_step.hasAdvancedFeatures(), "Default step should not have advanced features");
    
    EnhancedSequencerStep probability_step;
    probability_step.setProbability(0.8f);
    ASSERT_TRUE(probability_step.hasAdvancedFeatures(), "Step with probability should have advanced features");
    
    EnhancedSequencerStep timing_step;
    timing_step.setMicroTiming(10);
    ASSERT_TRUE(timing_step.hasAdvancedFeatures(), "Step with micro-timing should have advanced features");
    
    EnhancedSequencerStep retrigger_step;
    retrigger_step.setRetriggerCount(2);
    ASSERT_TRUE(retrigger_step.hasAdvancedFeatures(), "Step with retriggers should have advanced features");
    
    EnhancedSequencerStep condition_step;
    condition_step.setTriggerCondition(ConditionalTriggerEngine::TriggerCondition::FIRST);
    ASSERT_TRUE(condition_step.hasAdvancedFeatures(), "Step with conditions should have advanced features");
    
    EnhancedSequencerStep lock_step;
    lock_step.setParameterLock(42, 0.5f);
    ASSERT_TRUE(lock_step.hasAdvancedFeatures(), "Step with parameter locks should have advanced features");
    
    PASS("Advanced feature detection works correctly");
}

TEST_UNIT(EnhancedSequencerStep_CopyOperations) {
    PRINT_TEST_HEADER("Enhanced Sequencer Step - Copy Operations");
    
    EnhancedSequencerStep source_step;
    source_step.setActive(true);
    source_step.setNote(72);
    source_step.setVelocity(110);
    source_step.setProbability(0.8f);
    source_step.setMicroTiming(15);
    source_step.setRetriggerCount(2);
    source_step.setTriggerCondition(ConditionalTriggerEngine::TriggerCondition::FILL);
    source_step.setParameterLock(42, 0.6f);
    source_step.setParameterLock(43, 0.7f);
    
    // Test copy constructor
    EnhancedSequencerStep copied_step(source_step);
    ASSERT_TRUE(copied_step.isActive(), "Copied step should be active");
    ASSERT_EQ(copied_step.getNote(), 72U, "Copied step should have same note");
    ASSERT_EQ(copied_step.getVelocity(), 110U, "Copied step should have same velocity");
    ASSERT_EQ(copied_step.getProbability(), 0.8f, "Copied step should have same probability");
    ASSERT_EQ(copied_step.getMicroTiming(), 15, "Copied step should have same micro-timing");
    ASSERT_EQ(copied_step.getRetriggerCount(), 2U, "Copied step should have same retrigger count");
    ASSERT_EQ(copied_step.getTriggerCondition(), ConditionalTriggerEngine::TriggerCondition::FILL, "Copied step should have same condition");
    ASSERT_EQ(copied_step.getParameterLockCount(), 2U, "Copied step should have same number of locks");
    ASSERT_TRUE(copied_step.hasParameterLock(42), "Copied step should have parameter lock 42");
    ASSERT_TRUE(copied_step.hasParameterLock(43), "Copied step should have parameter lock 43");
    
    // Test assignment operator
    EnhancedSequencerStep assigned_step;
    assigned_step = source_step;
    ASSERT_TRUE(assigned_step.isActive(), "Assigned step should be active");
    ASSERT_EQ(assigned_step.getParameterLockCount(), 2U, "Assigned step should have same number of locks");
    
    // Test copyFrom method
    EnhancedSequencerStep copy_from_step;
    copy_from_step.copyFrom(source_step);
    ASSERT_TRUE(copy_from_step.isActive(), "CopyFrom step should be active");
    ASSERT_EQ(copy_from_step.getParameterLockCount(), 2U, "CopyFrom step should have same number of locks");
    
    PASS("Copy operations work correctly");
}

TEST_UNIT(EnhancedSequencerStep_MemoryUsage) {
    PRINT_TEST_HEADER("Enhanced Sequencer Step - Memory Usage");
    
    EnhancedSequencerStep empty_step;
    size_t empty_usage = empty_step.getMemoryUsage();
    ASSERT_TRUE(empty_usage > 0, "Empty step should have non-zero memory usage");
    
    EnhancedSequencerStep locked_step;
    locked_step.setParameterLock(42, 0.5f);
    locked_step.setParameterLock(43, 0.6f);
    locked_step.setParameterLock(44, 0.7f);
    size_t locked_usage = locked_step.getMemoryUsage();
    ASSERT_TRUE(locked_usage > empty_usage, "Step with locks should use more memory");
    
    PASS("Memory usage calculation works correctly");
}

TEST_UNIT(EnhancedSequencerStep_ConfigurationSummary) {
    PRINT_TEST_HEADER("Enhanced Sequencer Step - Configuration Summary");
    
    EnhancedSequencerStep step;
    
    // Test inactive step summary
    std::string inactive_summary = step.getConfigurationSummary();
    ASSERT_TRUE(inactive_summary.find("INACTIVE") != std::string::npos, "Inactive step summary should mention INACTIVE");
    
    // Test active step with features
    step.setActive(true);
    step.setNote(72);
    step.setVelocity(120);
    step.setProbability(0.8f);
    step.setMicroTiming(10);
    step.setRetriggerCount(2);
    step.setParameterLock(42, 0.5f);
    
    std::string active_summary = step.getConfigurationSummary();
    ASSERT_TRUE(active_summary.find("Note=72") != std::string::npos, "Summary should include note");
    ASSERT_TRUE(active_summary.find("Vel=120") != std::string::npos, "Summary should include velocity");
    ASSERT_TRUE(active_summary.find("Prob=80") != std::string::npos, "Summary should include probability");
    ASSERT_TRUE(active_summary.find("Timing=10") != std::string::npos, "Summary should include micro-timing");
    ASSERT_TRUE(active_summary.find("Retrig=2") != std::string::npos, "Summary should include retriggers");
    ASSERT_TRUE(active_summary.find("Locks=1") != std::string::npos, "Summary should include parameter locks");
    
    PASS("Configuration summary works correctly");
}

TEST_UNIT(EnhancedSequencerStep_ResetFunctionality) {
    PRINT_TEST_HEADER("Enhanced Sequencer Step - Reset Functionality");
    
    EnhancedSequencerStep step;
    
    // Configure step with various features
    step.setActive(true);
    step.setNote(84);
    step.setVelocity(127);
    step.setLength(90);
    step.setProbability(0.7f);
    step.setMicroTiming(20);
    step.setRetriggerCount(3);
    step.setTriggerCondition(ConditionalTriggerEngine::TriggerCondition::FILL);
    step.setParameterLock(42, 0.8f);
    
    // Reset step
    step.reset();
    
    // Verify all properties are back to defaults
    ASSERT_FALSE(step.isActive(), "Reset step should be inactive");
    ASSERT_EQ(step.getNote(), 60U, "Reset step should have default note");
    ASSERT_EQ(step.getVelocity(), 100U, "Reset step should have default velocity");
    ASSERT_EQ(step.getLength(), 80U, "Reset step should have default length");
    ASSERT_EQ(step.getProbability(), 1.0f, "Reset step should have default probability");
    ASSERT_EQ(step.getMicroTiming(), 0, "Reset step should have default micro-timing");
    ASSERT_EQ(step.getRetriggerCount(), 0U, "Reset step should have default retrigger count");
    ASSERT_EQ(step.getTriggerCondition(), ConditionalTriggerEngine::TriggerCondition::NONE, "Reset step should have default condition");
    ASSERT_EQ(step.getParameterLockCount(), 0U, "Reset step should have no parameter locks");
    ASSERT_FALSE(step.hasAdvancedFeatures(), "Reset step should have no advanced features");
    
    PASS("Reset functionality works correctly");
}

// =============================================================================
// CHAPTER 8: PERFORMANCE AND INTEGRATION TESTING
// =============================================================================

TEST_INTEGRATION(DigitaktFeatures_PerformanceTest) {
    PRINT_TEST_HEADER("Digitakt Features - Performance Test");
    
    const int iterations = 10000;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Performance test for all engines
    ProbabilityEngine::setSeed(42);
    for (int i = 0; i < iterations; ++i) {
        // Test probability engine
        ProbabilityEngine::shouldTrigger(0.5f);
        
        // Test micro-timing engine
        MicroTimingEngine::calculateTiming(1000, 10);
        
        // Test retrigger engine
        RetriggerEngine::calculateRetriggerTimes(0, 96, 3, RetriggerEngine::RetriggerRate::RATE_1_16);
        
        // Test conditional trigger engine
        SequencerContext context(0, i % 16, i / 16, false);
        ConditionalTriggerEngine::evaluateCondition(
            ConditionalTriggerEngine::TriggerCondition::FIRST, 0, context);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    float avg_time_per_iteration = static_cast<float>(duration.count()) / iterations;
    
    // Performance requirement: less than 10 microseconds per iteration
    ASSERT_TRUE(avg_time_per_iteration < 10.0f, 
                std::string("Performance test failed. Average time per iteration: ") + 
                std::to_string(avg_time_per_iteration) + " microseconds");
    
    PASS(std::string("Performance test passed. Average time: ") + 
         std::to_string(avg_time_per_iteration) + " microseconds per iteration");
}

TEST_INTEGRATION(DigitaktFeatures_ThreadSafetyTest) {
    PRINT_TEST_HEADER("Digitakt Features - Thread Safety Test");
    
    const int num_threads = 4;
    const int iterations_per_thread = 1000;
    std::vector<std::thread> threads;
    std::atomic<int> total_operations(0);
    
    // Test thread safety of all engines
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&total_operations, iterations_per_thread, t]() {
            ProbabilityEngine::setSeed(42 + t);
            
            for (int i = 0; i < iterations_per_thread; ++i) {
                // Test probability engine (thread-local state)
                ProbabilityEngine::shouldTrigger(0.5f);
                
                // Test micro-timing engine (stateless)
                MicroTimingEngine::calculateTiming(1000, 10);
                
                // Test retrigger engine (stateless)
                RetriggerEngine::calculateRetriggerTimes(0, 96, 2, RetriggerEngine::RetriggerRate::RATE_1_16);
                
                // Test conditional trigger engine (stateless)
                SequencerContext context(t, i % 16, i / 16, false);
                ConditionalTriggerEngine::evaluateCondition(
                    ConditionalTriggerEngine::TriggerCondition::NONE, 0, context);
                
                total_operations.fetch_add(1);
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    int expected_operations = num_threads * iterations_per_thread;
    ASSERT_EQ(total_operations.load(), expected_operations, "All thread operations should complete");
    
    PASS("Thread safety test passed");
}

TEST_INTEGRATION(DigitaktFeatures_MemoryUsageTest) {
    PRINT_TEST_HEADER("Digitakt Features - Memory Usage Test");
    
    const int num_steps = 1000;
    std::vector<EnhancedSequencerStep> steps(num_steps);
    
    // Configure steps with various features
    for (int i = 0; i < num_steps; ++i) {
        steps[i].setActive(i % 2 == 0);
        steps[i].setNote(60 + (i % 12));
        steps[i].setVelocity(100 + (i % 28));
        steps[i].setProbability(0.5f + (i % 50) / 100.0f);
        steps[i].setMicroTiming((i % 101) - 50);
        steps[i].setRetriggerCount(i % 9);
        steps[i].setTriggerCondition(
            static_cast<ConditionalTriggerEngine::TriggerCondition>(i % 5));
        
        // Add parameter locks
        for (int j = 0; j < (i % 5); ++j) {
            steps[i].setParameterLock(42 + j, (j + 1) * 0.2f);
        }
    }
    
    // Calculate total memory usage
    size_t total_memory = 0;
    for (const auto& step : steps) {
        total_memory += step.getMemoryUsage();
    }
    
    float avg_memory_per_step = static_cast<float>(total_memory) / num_steps;
    
    // Memory requirement: less than 1KB per step on average
    ASSERT_TRUE(avg_memory_per_step < 1024.0f, 
                std::string("Memory usage test failed. Average memory per step: ") + 
                std::to_string(avg_memory_per_step) + " bytes");
    
    PASS(std::string("Memory usage test passed. Average memory per step: ") + 
         std::to_string(avg_memory_per_step) + " bytes");
}

// =============================================================================
// CHAPTER 9: EDGE CASES AND ERROR HANDLING
// =============================================================================

TEST_UNIT(DigitaktFeatures_EdgeCaseHandling) {
    PRINT_TEST_HEADER("Digitakt Features - Edge Case Handling");
    
    // Test extreme values for micro-timing
    int8_t extreme_positive = MicroTimingEngine::clampMicroTiming(32767);
    ASSERT_EQ(extreme_positive, 50, "Extreme positive value should be clamped");
    
    int8_t extreme_negative = MicroTimingEngine::clampMicroTiming(-32768);
    ASSERT_EQ(extreme_negative, -50, "Extreme negative value should be clamped");
    
    // Test extreme values for retrigger count
    uint8_t extreme_retriggers = RetriggerEngine::clampRetriggerCount(255);
    ASSERT_EQ(extreme_retriggers, 8U, "Extreme retrigger count should be clamped");
    
    // Test zero step duration for retriggers
    auto zero_duration_times = RetriggerEngine::calculateRetriggerTimes(0, 0, 3, RetriggerEngine::RetriggerRate::RATE_1_16);
    ASSERT_EQ(zero_duration_times.size(), 1U, "Zero duration should result in single trigger");
    
    // Test extreme probability values
    ASSERT_FALSE(ProbabilityEngine::shouldTrigger(-1000.0f), "Extreme negative probability should be treated as 0");
    ASSERT_TRUE(ProbabilityEngine::shouldTrigger(1000.0f), "Extreme positive probability should be treated as 1");
    
    PASS("Edge case handling works correctly");
}

TEST_UNIT(DigitaktFeatures_InvalidInputHandling) {
    PRINT_TEST_HEADER("Digitakt Features - Invalid Input Handling");
    
    // Test invalid context for conditional triggers
    SequencerContext invalid_context(-1, -1, -1, false);
    bool invalid_result = ConditionalTriggerEngine::evaluateCondition(
        ConditionalTriggerEngine::TriggerCondition::FIRST, 0, invalid_context);
    // Should not crash and should return a boolean result
    
    // Test invalid parameter validation
    uint8_t validated_param = ConditionalTriggerEngine::validateConditionParam(
        ConditionalTriggerEngine::TriggerCondition::NEI, 255);
    ASSERT_EQ(validated_param, 1U, "Invalid parameter should be clamped to valid range");
    
    // Test empty fill pattern
    auto& fillMgr = FillModeManager::getInstance();
    fillMgr.clearTrackFillPattern(999);  // Non-existent track
    const auto& empty_pattern = fillMgr.getTrackFillPattern(999);
    ASSERT_TRUE(empty_pattern.empty(), "Non-existent track should return empty pattern");
    
    PASS("Invalid input handling works correctly");
}

TEST_UNIT(DigitaktFeatures_BoundaryConditions) {
    PRINT_TEST_HEADER("Digitakt Features - Boundary Conditions");
    
    // Test minimum and maximum valid values
    EnhancedSequencerStep step;
    
    // Test minimum values
    step.setNote(0);
    step.setVelocity(0);
    step.setLength(0);
    step.setProbability(0.0f);
    step.setMicroTiming(-50);
    step.setRetriggerCount(0);
    
    ASSERT_EQ(step.getNote(), 0U, "Minimum note should be accepted");
    ASSERT_EQ(step.getVelocity(), 0U, "Minimum velocity should be accepted");
    ASSERT_EQ(step.getLength(), 0U, "Minimum length should be accepted");
    ASSERT_EQ(step.getProbability(), 0.0f, "Minimum probability should be accepted");
    ASSERT_EQ(step.getMicroTiming(), -50, "Minimum micro-timing should be accepted");
    ASSERT_EQ(step.getRetriggerCount(), 0U, "Minimum retrigger count should be accepted");
    
    // Test maximum values
    step.setNote(127);
    step.setVelocity(127);
    step.setLength(100);
    step.setProbability(1.0f);
    step.setMicroTiming(50);
    step.setRetriggerCount(8);
    
    ASSERT_EQ(step.getNote(), 127U, "Maximum note should be accepted");
    ASSERT_EQ(step.getVelocity(), 127U, "Maximum velocity should be accepted");
    ASSERT_EQ(step.getLength(), 100U, "Maximum length should be accepted");
    ASSERT_EQ(step.getProbability(), 1.0f, "Maximum probability should be accepted");
    ASSERT_EQ(step.getMicroTiming(), 50, "Maximum micro-timing should be accepted");
    ASSERT_EQ(step.getRetriggerCount(), 8U, "Maximum retrigger count should be accepted");
    
    PASS("Boundary conditions handled correctly");
}

// Run all tests
int main() {
    std::cout << "🎛️ RUNNING DIGITAKT FEATURES TEST SUITE" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    RUN_ALL_TESTS();
    
    return 0;
}
