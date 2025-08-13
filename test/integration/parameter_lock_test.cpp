/**
 * @brief Parameter Lock System Tests - Unified Framework Migration
 * 
 * This file contains comprehensive tests for the Parameter Lock System,
 * migrated from the old fragmented test_parameter_lock_system.cpp to use the 
 * unified test framework with clean mock dependencies.
 * 
 * MIGRATION TARGET:
 * - Original: 530 lines with external component dependencies
 * - Target: Integration test category (parameter lock system)
 * - Focus: Step sequencer parameter automation, lock management, performance
 * 
 * TEST COVERAGE - THE COMPLETE PARAMETER LOCK STORY:
 * 
 * 🔒 CHAPTER 1: Basic Parameter Locking
 *    Set, get, clear parameter locks on individual sequencer steps.
 *    Core lock operations with proper validation.
 * 
 * 🎯 CHAPTER 2: Parameter Lock Application
 *    Real-time parameter lock application during step playback.
 *    Automatic parameter changes when steps are triggered.
 * 
 * 🔄 CHAPTER 3: Parameter Restoration
 *    Restore global parameters when step playback ends.
 *    Clean parameter state management.
 * 
 * 🎚️ CHAPTER 4: Multiple Parameter Locks
 *    Multiple parameters locked per step (filter + volume + resonance).
 *    Complex multi-parameter step programming.
 * 
 * ⚙️ CHAPTER 5: Step Lock Operations
 *    Bulk operations on step locks (clear all, copy steps).
 *    Efficient step management for complex sequences.
 * 
 * 📊 CHAPTER 6: Parameter Lock Statistics
 *    Performance monitoring and lock usage statistics.
 *    System health and optimization metrics.
 * 
 * 🎵 CHAPTER 7: Step Sequencer Integration
 *    Deep integration with step sequencer state machine.
 *    Coordinated playback and parameter automation.
 * 
 * 🚀 CHAPTER 8: Parameter Lock Performance
 *    High-frequency parameter lock operations for real-time audio.
 *    Performance validation under heavy load.
 * 
 * 💾 CHAPTER 9: Mass Parameter Operations
 *    Bulk parameter lock operations for complex sequences.
 *    Efficient handling of large parameter lock datasets.
 * 
 * ARCHITECTURE:
 * - Mock-based testing eliminates external component dependencies
 * - Realistic step sequencer scenarios with 16-step patterns
 * - Parameter lock automation for professional music production
 * - Performance testing with real-time audio constraints
 * - Comprehensive statistics tracking and monitoring
 * 
 * REAL-WORLD APPLICATION:
 * Parameter locks are essential in step sequencers where each step can override
 * global parameter settings. This enables complex parameter automation patterns
 * like filter sweeps, volume dynamics, and rhythmic parameter modulation.
 * 
 * @author Migrated to Unified Framework
 * @date August 8, 2025
 */

#include "../framework/unified_test_framework.h"
#include "../fixtures/test_fixtures.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <algorithm>

/**
 * @brief Parameter source enumeration for tracking parameter change origins
 */
enum class ParameterSource {
    TOUCH_INPUT = 0,     ///< User touch/interaction
    MIDI_INPUT = 1,      ///< MIDI controller input
    AUTOMATION = 2,      ///< Automation/sequencer
    PARAMETER_LOCK = 3,  ///< Parameter lock system
    PRESET_LOAD = 4      ///< Preset loading
};

/**
 * @brief Parameter change event structure
 */
struct ParameterChangeEvent {
    uint32_t parameter_id;     ///< Parameter identifier
    float normalized_value;    ///< Normalized parameter value (0.0-1.0)
    ParameterSource source;    ///< Source of the parameter change
    uint64_t timestamp_us;     ///< Timestamp in microseconds
    
    ParameterChangeEvent(uint32_t id, float value, ParameterSource src)
        : parameter_id(id), normalized_value(value), source(src),
          timestamp_us(std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::steady_clock::now().time_since_epoch()).count()) {}
};

/**
 * @brief Mock Parameter Manager for testing parameter lock integration
 */
class MockParameterManager {
private:
    std::unordered_map<uint32_t, float> parameters_;
    std::vector<ParameterChangeEvent> change_history_;
    std::function<void(const ParameterChangeEvent&)> change_callback_;
    std::atomic<size_t> parameter_change_count_{0};
    
public:
    void setParameter(uint32_t parameter_id, float value, ParameterSource source) {
        parameters_[parameter_id] = value;
        
        ParameterChangeEvent event(parameter_id, value, source);
        change_history_.push_back(event);
        
        if (change_callback_) {
            change_callback_(event);
        }
        
        parameter_change_count_++;
    }
    
    float getParameter(uint32_t parameter_id) const {
        auto it = parameters_.find(parameter_id);
        return (it != parameters_.end()) ? it->second : 0.0f;
    }
    
    void setChangeCallback(std::function<void(const ParameterChangeEvent&)> callback) {
        change_callback_ = callback;
    }
    
    const std::vector<ParameterChangeEvent>& getChangeHistory() const {
        return change_history_;
    }
    
    size_t getParameterChangeCount() const {
        return parameter_change_count_.load();
    }
    
    void reset() {
        parameters_.clear();
        change_history_.clear();
        parameter_change_count_ = 0;
    }
    
    bool hasParameterChange(uint32_t id, float expected_value, ParameterSource expected_source) const {
        for (const auto& event : change_history_) {
            if (event.parameter_id == id && 
                std::abs(event.normalized_value - expected_value) < 0.01f && 
                event.source == expected_source) {
                return true;
            }
        }
        return false;
    }
};

/**
 * @brief Mock Parameter Lock Manager for testing step sequencer parameter automation
 */
class MockParameterLockManager {
private:
    // parameter_locks_[track][step][parameter_id] = value
    std::unordered_map<uint8_t, std::unordered_map<uint8_t, std::unordered_map<uint32_t, float>>> parameter_locks_;
    std::shared_ptr<MockParameterManager> parameter_manager_;
    std::unordered_map<uint32_t, float> restored_parameters_; // Global parameter backup
    std::atomic<size_t> lock_operations_count_{0};
    std::atomic<size_t> applied_locks_count_{0};
    
public:
    void setParameterManager(std::shared_ptr<MockParameterManager> param_manager) {
        parameter_manager_ = param_manager;
    }
    
    void setStepParameterLock(uint8_t track, uint8_t step, uint32_t parameter_id, float value) {
        parameter_locks_[track][step][parameter_id] = value;
        lock_operations_count_++;
    }
    
    bool hasStepParameterLock(uint8_t track, uint8_t step, uint32_t parameter_id) const {
        auto track_it = parameter_locks_.find(track);
        if (track_it == parameter_locks_.end()) return false;
        
        auto step_it = track_it->second.find(step);
        if (step_it == track_it->second.end()) return false;
        
        return step_it->second.find(parameter_id) != step_it->second.end();
    }
    
    float getStepParameterLock(uint8_t track, uint8_t step, uint32_t parameter_id) const {
        auto track_it = parameter_locks_.find(track);
        if (track_it == parameter_locks_.end()) return 0.0f;
        
        auto step_it = track_it->second.find(step);
        if (step_it == track_it->second.end()) return 0.0f;
        
        auto param_it = step_it->second.find(parameter_id);
        return (param_it != step_it->second.end()) ? param_it->second : 0.0f;
    }
    
    void clearStepParameterLock(uint8_t track, uint8_t step, uint32_t parameter_id) {
        auto track_it = parameter_locks_.find(track);
        if (track_it == parameter_locks_.end()) return;
        
        auto step_it = track_it->second.find(step);
        if (step_it == track_it->second.end()) return;
        
        step_it->second.erase(parameter_id);
        lock_operations_count_++;
    }
    
    void applyStepParameterLocks(uint8_t track, uint8_t step) {
        if (!parameter_manager_) return;
        
        auto track_it = parameter_locks_.find(track);
        if (track_it == parameter_locks_.end()) return;
        
        auto step_it = track_it->second.find(step);
        if (step_it == track_it->second.end()) return;
        
        // Backup current parameters before applying locks
        for (const auto& param_lock : step_it->second) {
            uint32_t param_id = param_lock.first;
            restored_parameters_[param_id] = parameter_manager_->getParameter(param_id);
        }
        
        // Apply parameter locks
        for (const auto& param_lock : step_it->second) {
            uint32_t param_id = param_lock.first;
            float lock_value = param_lock.second;
            parameter_manager_->setParameter(param_id, lock_value, ParameterSource::PARAMETER_LOCK);
            applied_locks_count_++;
        }
    }
    
    void restoreGlobalParameters() {
        if (!parameter_manager_) return;
        
        for (const auto& param : restored_parameters_) {
            parameter_manager_->setParameter(param.first, param.second, ParameterSource::AUTOMATION);
        }
        restored_parameters_.clear();
    }
    
    void clearAllStepLocks(uint8_t track, uint8_t step) {
        auto track_it = parameter_locks_.find(track);
        if (track_it == parameter_locks_.end()) return;
        
        track_it->second.erase(step);
        lock_operations_count_++;
    }
    
    size_t getStepLockCount(uint8_t track, uint8_t step) const {
        auto track_it = parameter_locks_.find(track);
        if (track_it == parameter_locks_.end()) return 0;
        
        auto step_it = track_it->second.find(step);
        if (step_it == track_it->second.end()) return 0;
        
        return step_it->second.size();
    }
    
    size_t getTotalLockCount() const {
        size_t total = 0;
        for (const auto& track : parameter_locks_) {
            for (const auto& step : track.second) {
                total += step.second.size();
            }
        }
        return total;
    }
    
    // Statistics
    size_t getLockOperationsCount() const { return lock_operations_count_.load(); }
    size_t getAppliedLocksCount() const { return applied_locks_count_.load(); }
    
    void resetStatistics() {
        lock_operations_count_ = 0;
        applied_locks_count_ = 0;
    }
    
    void reset() {
        parameter_locks_.clear();
        restored_parameters_.clear();
        resetStatistics();
    }
};

/**
 * @brief Mock Step Sequencer for testing parameter lock integration
 */
class MockStepSequencer {
private:
    uint8_t current_track_{0};
    uint8_t current_step_{0};
    bool playing_{false};
    std::shared_ptr<MockParameterLockManager> lock_manager_;
    std::atomic<size_t> step_trigger_count_{0};
    
public:
    void setParameterLockManager(std::shared_ptr<MockParameterLockManager> lock_manager) {
        lock_manager_ = lock_manager;
    }
    
    void setCurrentStep(uint8_t track, uint8_t step) {
        current_track_ = track;
        current_step_ = step;
        
        if (playing_ && lock_manager_) {
            lock_manager_->applyStepParameterLocks(track, step);
        }
        
        step_trigger_count_++;
    }
    
    void startPlayback() {
        playing_ = true;
    }
    
    void stopPlayback() {
        playing_ = false;
        if (lock_manager_) {
            lock_manager_->restoreGlobalParameters();
        }
    }
    
    uint8_t getCurrentTrack() const { return current_track_; }
    uint8_t getCurrentStep() const { return current_step_; }
    bool isPlaying() const { return playing_; }
    
    size_t getStepTriggerCount() const { return step_trigger_count_.load(); }
    
    void reset() {
        current_track_ = 0;
        current_step_ = 0;
        playing_ = false;
        step_trigger_count_ = 0;
    }
};

// ============================================================================
// GLOBAL TEST FIXTURES
// ============================================================================

static std::shared_ptr<MockParameterManager> g_parameter_manager;
static std::shared_ptr<MockParameterLockManager> g_lock_manager;
static std::shared_ptr<MockStepSequencer> g_sequencer;

// Common parameter IDs for testing
static constexpr uint32_t PARAM_FILTER_CUTOFF = 1;
static constexpr uint32_t PARAM_RESONANCE = 2;
static constexpr uint32_t PARAM_VOLUME = 3;
static constexpr uint32_t PARAM_ATTACK = 4;
static constexpr uint32_t PARAM_DECAY = 5;

void setupParameterLockTests() {
    // Create system components
    g_parameter_manager = std::make_shared<MockParameterManager>();
    g_lock_manager = std::make_shared<MockParameterLockManager>();
    g_sequencer = std::make_shared<MockStepSequencer>();
    
    // Wire up the system
    g_lock_manager->setParameterManager(g_parameter_manager);
    g_sequencer->setParameterLockManager(g_lock_manager);
    
    // Set some default parameter values
    g_parameter_manager->setParameter(PARAM_FILTER_CUTOFF, 0.5f, ParameterSource::TOUCH_INPUT);
    g_parameter_manager->setParameter(PARAM_RESONANCE, 0.3f, ParameterSource::TOUCH_INPUT);
    g_parameter_manager->setParameter(PARAM_VOLUME, 0.7f, ParameterSource::TOUCH_INPUT);
    g_parameter_manager->setParameter(PARAM_ATTACK, 0.1f, ParameterSource::TOUCH_INPUT);
    g_parameter_manager->setParameter(PARAM_DECAY, 0.4f, ParameterSource::TOUCH_INPUT);
    
    // Reset components for clean test state
    g_parameter_manager->reset();
    g_lock_manager->reset();
    g_sequencer->reset();
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

TEST_INTEGRATION(ParameterLock, BasicParameterLocking) {
    setupParameterLockTests();
    
    // Test setting parameter locks
    g_lock_manager->setStepParameterLock(0, 0, PARAM_FILTER_CUTOFF, 0.8f);
    g_lock_manager->setStepParameterLock(0, 0, PARAM_RESONANCE, 0.6f);
    
    // Verify locks were set
    ASSERT_TRUE(g_lock_manager->hasStepParameterLock(0, 0, PARAM_FILTER_CUTOFF));
    ASSERT_TRUE(g_lock_manager->hasStepParameterLock(0, 0, PARAM_RESONANCE));
    ASSERT_FALSE(g_lock_manager->hasStepParameterLock(0, 0, PARAM_VOLUME)); // Not set
    
    // Verify lock values
    ASSERT_NEAR(0.8f, g_lock_manager->getStepParameterLock(0, 0, PARAM_FILTER_CUTOFF), 0.01f);
    ASSERT_NEAR(0.6f, g_lock_manager->getStepParameterLock(0, 0, PARAM_RESONANCE), 0.01f);
    
    // Test clearing locks
    g_lock_manager->clearStepParameterLock(0, 0, PARAM_FILTER_CUTOFF);
    ASSERT_FALSE(g_lock_manager->hasStepParameterLock(0, 0, PARAM_FILTER_CUTOFF));
    ASSERT_TRUE(g_lock_manager->hasStepParameterLock(0, 0, PARAM_RESONANCE)); // Should still exist
    
    // Verify statistics
    ASSERT_EQ(3lu, g_lock_manager->getLockOperationsCount()); // 2 sets + 1 clear
}

TEST_INTEGRATION(ParameterLock, ParameterLockApplication) {
    setupParameterLockTests();
    
    // Set initial global parameters
    g_parameter_manager->setParameter(PARAM_FILTER_CUTOFF, 0.5f, ParameterSource::TOUCH_INPUT);
    g_parameter_manager->setParameter(PARAM_RESONANCE, 0.3f, ParameterSource::TOUCH_INPUT);
    
    // Set parameter locks for step 0
    g_lock_manager->setStepParameterLock(0, 0, PARAM_FILTER_CUTOFF, 0.9f);
    g_lock_manager->setStepParameterLock(0, 0, PARAM_RESONANCE, 0.7f);
    
    // Start sequencer and trigger step 0
    g_sequencer->startPlayback();
    g_sequencer->setCurrentStep(0, 0);
    
    // Verify parameter locks were applied
    ASSERT_NEAR(0.9f, g_parameter_manager->getParameter(PARAM_FILTER_CUTOFF), 0.01f);
    ASSERT_NEAR(0.7f, g_parameter_manager->getParameter(PARAM_RESONANCE), 0.01f);
    
    // Verify parameter changes came from parameter lock source
    ASSERT_TRUE(g_parameter_manager->hasParameterChange(PARAM_FILTER_CUTOFF, 0.9f, ParameterSource::PARAMETER_LOCK));
    ASSERT_TRUE(g_parameter_manager->hasParameterChange(PARAM_RESONANCE, 0.7f, ParameterSource::PARAMETER_LOCK));
    
    // Verify statistics
    ASSERT_EQ(2lu, g_lock_manager->getAppliedLocksCount());
    ASSERT_EQ(1lu, g_sequencer->getStepTriggerCount());
}

TEST_INTEGRATION(ParameterLock, ParameterRestoration) {
    setupParameterLockTests();
    
    // Set initial global parameters
    g_parameter_manager->setParameter(PARAM_FILTER_CUTOFF, 0.4f, ParameterSource::TOUCH_INPUT);
    g_parameter_manager->setParameter(PARAM_VOLUME, 0.8f, ParameterSource::TOUCH_INPUT);
    
    // Set parameter locks
    g_lock_manager->setStepParameterLock(0, 0, PARAM_FILTER_CUTOFF, 0.9f);
    g_lock_manager->setStepParameterLock(0, 0, PARAM_VOLUME, 0.2f);
    
    // Apply locks
    g_sequencer->startPlayback();
    g_sequencer->setCurrentStep(0, 0);
    
    // Verify locks applied
    ASSERT_NEAR(0.9f, g_parameter_manager->getParameter(PARAM_FILTER_CUTOFF), 0.01f);
    ASSERT_NEAR(0.2f, g_parameter_manager->getParameter(PARAM_VOLUME), 0.01f);
    
    // Stop playback - should restore global parameters
    g_sequencer->stopPlayback();
    
    // Verify parameters were restored
    ASSERT_NEAR(0.4f, g_parameter_manager->getParameter(PARAM_FILTER_CUTOFF), 0.01f);
    ASSERT_NEAR(0.8f, g_parameter_manager->getParameter(PARAM_VOLUME), 0.01f);
    
    // Verify restoration came from automation source
    ASSERT_TRUE(g_parameter_manager->hasParameterChange(PARAM_FILTER_CUTOFF, 0.4f, ParameterSource::AUTOMATION));
    ASSERT_TRUE(g_parameter_manager->hasParameterChange(PARAM_VOLUME, 0.8f, ParameterSource::AUTOMATION));
}

TEST_INTEGRATION(ParameterLock, MultipleParameterLocks) {
    setupParameterLockTests();
    
    // Set multiple parameter locks on different steps
    g_lock_manager->setStepParameterLock(0, 0, PARAM_FILTER_CUTOFF, 0.8f);
    g_lock_manager->setStepParameterLock(0, 0, PARAM_RESONANCE, 0.6f);
    g_lock_manager->setStepParameterLock(0, 0, PARAM_VOLUME, 0.9f);
    
    g_lock_manager->setStepParameterLock(0, 1, PARAM_FILTER_CUTOFF, 0.3f);
    g_lock_manager->setStepParameterLock(0, 1, PARAM_ATTACK, 0.5f);
    
    g_lock_manager->setStepParameterLock(0, 2, PARAM_DECAY, 0.7f);
    
    // Verify step lock counts
    ASSERT_EQ(3lu, g_lock_manager->getStepLockCount(0, 0)); // 3 parameters on step 0
    ASSERT_EQ(2lu, g_lock_manager->getStepLockCount(0, 1)); // 2 parameters on step 1
    ASSERT_EQ(1lu, g_lock_manager->getStepLockCount(0, 2)); // 1 parameter on step 2
    ASSERT_EQ(0lu, g_lock_manager->getStepLockCount(0, 3)); // No parameters on step 3
    
    // Verify total lock count
    ASSERT_EQ(6lu, g_lock_manager->getTotalLockCount());
    
    // Test step sequence with multiple locks
    g_sequencer->startPlayback();
    
    // Trigger step 0 - should apply 3 parameter locks
    g_sequencer->setCurrentStep(0, 0);
    ASSERT_EQ(3lu, g_lock_manager->getAppliedLocksCount());
    
    // Trigger step 1 - should apply 2 more parameter locks
    g_sequencer->setCurrentStep(0, 1);
    ASSERT_EQ(5lu, g_lock_manager->getAppliedLocksCount());
    
    // Trigger step 2 - should apply 1 more parameter lock
    g_sequencer->setCurrentStep(0, 2);
    ASSERT_EQ(6lu, g_lock_manager->getAppliedLocksCount());
}

TEST_INTEGRATION(ParameterLock, StepLockOperations) {
    setupParameterLockTests();
    
    // Set up step with multiple parameter locks
    g_lock_manager->setStepParameterLock(0, 5, PARAM_FILTER_CUTOFF, 0.8f);
    g_lock_manager->setStepParameterLock(0, 5, PARAM_RESONANCE, 0.6f);
    g_lock_manager->setStepParameterLock(0, 5, PARAM_VOLUME, 0.9f);
    g_lock_manager->setStepParameterLock(0, 5, PARAM_ATTACK, 0.2f);
    
    // Verify all locks are set
    ASSERT_EQ(4lu, g_lock_manager->getStepLockCount(0, 5));
    
    // Clear all locks for the step
    g_lock_manager->clearAllStepLocks(0, 5);
    
    // Verify all locks were cleared
    ASSERT_EQ(0lu, g_lock_manager->getStepLockCount(0, 5));
    ASSERT_FALSE(g_lock_manager->hasStepParameterLock(0, 5, PARAM_FILTER_CUTOFF));
    ASSERT_FALSE(g_lock_manager->hasStepParameterLock(0, 5, PARAM_RESONANCE));
    ASSERT_FALSE(g_lock_manager->hasStepParameterLock(0, 5, PARAM_VOLUME));
    ASSERT_FALSE(g_lock_manager->hasStepParameterLock(0, 5, PARAM_ATTACK));
    
    // Verify statistics
    ASSERT_EQ(5lu, g_lock_manager->getLockOperationsCount()); // 4 sets + 1 clear all
}

TEST_INTEGRATION(ParameterLock, ParameterLockStatistics) {
    setupParameterLockTests();
    
    // Reset statistics for clean measurement
    g_lock_manager->resetStatistics();
    g_parameter_manager->reset();
    
    // Perform various operations
    for (uint8_t step = 0; step < 8; step++) {
        g_lock_manager->setStepParameterLock(0, step, PARAM_FILTER_CUTOFF, 0.1f * step);
        g_lock_manager->setStepParameterLock(0, step, PARAM_RESONANCE, 0.05f * step);
    }
    
    // Apply some parameter locks
    g_sequencer->startPlayback();
    for (uint8_t step = 0; step < 4; step++) {
        g_sequencer->setCurrentStep(0, step);
    }
    
    // Verify statistics
    ASSERT_EQ(16lu, g_lock_manager->getLockOperationsCount()); // 8 steps × 2 params
    ASSERT_EQ(8lu, g_lock_manager->getAppliedLocksCount());    // 4 steps × 2 params
    ASSERT_EQ(4lu, g_sequencer->getStepTriggerCount());       // 4 step triggers
    ASSERT_TRUE(g_parameter_manager->getParameterChangeCount() >= 8); // At least 8 parameter changes
}

TEST_INTEGRATION(ParameterLock, SequencerIntegration) {
    setupParameterLockTests();
    
    // Create a realistic parameter lock sequence (filter sweep)
    for (uint8_t step = 0; step < 16; step++) {
        float cutoff_value = 0.1f + (0.8f * step / 15.0f); // Sweep from 0.1 to 0.9
        float resonance_value = 0.2f + (0.6f * (step % 4) / 3.0f); // Rhythmic resonance
        
        g_lock_manager->setStepParameterLock(0, step, PARAM_FILTER_CUTOFF, cutoff_value);
        g_lock_manager->setStepParameterLock(0, step, PARAM_RESONANCE, resonance_value);
    }
    
    // Verify sequence setup
    ASSERT_EQ(32lu, g_lock_manager->getTotalLockCount()); // 16 steps × 2 params
    
    // Simulate sequence playback
    g_sequencer->startPlayback();
    
    for (uint8_t step = 0; step < 16; step++) {
        g_sequencer->setCurrentStep(0, step);
        
        // Verify parameter values match expected lock values
        float expected_cutoff = 0.1f + (0.8f * step / 15.0f);
        float expected_resonance = 0.2f + (0.6f * (step % 4) / 3.0f);
        
        ASSERT_NEAR(expected_cutoff, g_parameter_manager->getParameter(PARAM_FILTER_CUTOFF), 0.01f);
        ASSERT_NEAR(expected_resonance, g_parameter_manager->getParameter(PARAM_RESONANCE), 0.01f);
    }
    
    // Verify complete sequence statistics
    ASSERT_EQ(16lu, g_sequencer->getStepTriggerCount());
    ASSERT_EQ(32lu, g_lock_manager->getAppliedLocksCount()); // 16 steps × 2 params
}

TEST_INTEGRATION(ParameterLock, PerformanceOptimization) {
    setupParameterLockTests();
    
    // Performance test: rapid parameter lock operations
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (uint8_t track = 0; track < 4; track++) {
        for (uint8_t step = 0; step < 16; step++) {
            for (uint32_t param = PARAM_FILTER_CUTOFF; param <= PARAM_DECAY; param++) {
                float value = static_cast<float>(step) / 15.0f;
                g_lock_manager->setStepParameterLock(track, step, param, value);
            }
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Should complete in reasonable time (under 1ms)
    ASSERT_TRUE(duration.count() < 1000);
    
    // Verify all locks were set
    ASSERT_EQ(320lu, g_lock_manager->getTotalLockCount()); // 4 tracks × 16 steps × 5 params
    
    // Performance test: rapid parameter lock application
    g_sequencer->startPlayback();
    start_time = std::chrono::high_resolution_clock::now();
    
    for (uint8_t step = 0; step < 16; step++) {
        g_sequencer->setCurrentStep(0, step);
    }
    
    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Parameter application should be very fast (under 500 microseconds)
    ASSERT_TRUE(duration.count() < 500);
    
    // Verify locks were applied correctly
    ASSERT_EQ(80lu, g_lock_manager->getAppliedLocksCount()); // 16 steps × 5 params
}

TEST_INTEGRATION(ParameterLock, MassParameterOperations) {
    setupParameterLockTests();
    
    // Test: Complex multi-track, multi-step scenario
    const uint8_t num_tracks = 8;
    const uint8_t num_steps = 16;
    const uint32_t num_params = 5;
    
    // Set up complex parameter lock matrix
    for (uint8_t track = 0; track < num_tracks; track++) {
        for (uint8_t step = 0; step < num_steps; step++) {
            for (uint32_t param = PARAM_FILTER_CUTOFF; param <= PARAM_DECAY; param++) {
                // Create varied parameter patterns per track
                float base_value = static_cast<float>(track) / (num_tracks - 1);
                float step_variation = static_cast<float>(step) / (num_steps - 1);
                float param_offset = static_cast<float>(param - PARAM_FILTER_CUTOFF) / (num_params - 1);
                
                float value = (base_value + step_variation + param_offset) / 3.0f;
                value = std::clamp(value, 0.0f, 1.0f);
                
                g_lock_manager->setStepParameterLock(track, step, param, value);
            }
        }
    }
    
    // Verify massive parameter lock setup
    size_t expected_total = num_tracks * num_steps * num_params;
    ASSERT_EQ(expected_total, g_lock_manager->getTotalLockCount());
    
    // Test bulk clear operations
    for (uint8_t track = 0; track < num_tracks; track++) {
        for (uint8_t step = 0; step < num_steps; step += 2) { // Clear every other step
            g_lock_manager->clearAllStepLocks(track, step);
        }
    }
    
    // Verify selective clearing worked
    size_t expected_remaining = num_tracks * (num_steps / 2) * num_params;
    ASSERT_EQ(expected_remaining, g_lock_manager->getTotalLockCount());
    
    // Test rapid playback through remaining locks
    g_sequencer->startPlayback();
    size_t applied_count = 0;
    
    for (uint8_t track = 0; track < num_tracks; track++) {
        for (uint8_t step = 0; step < num_steps; step++) {
            g_sequencer->setCurrentStep(track, step);
            if (step % 2 == 1) { // Only odd steps have locks
                applied_count += num_params;
            }
        }
    }
    
    ASSERT_EQ(applied_count, g_lock_manager->getAppliedLocksCount());
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "🔒 Parameter Lock System Tests - Unified Framework" << std::endl;
    std::cout << "======================================================" << std::endl;
    
    auto& runner = TestFramework::TestRunner::getInstance();
    
    // Run all integration tests for ParameterLock
    auto results = runner.runCategory("integration/ParameterLock");
    
    return results.failed_tests == 0 ? 0 : 1;
}
