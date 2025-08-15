#pragma once

#include "DigitaktFeatures.h"
#include "components/parameter/ParameterChangeEvent.h"
#include <unordered_map>
#include <cstdint>
#include <vector>

namespace MIDI {

using ParameterID = Parameters::ParameterID;

/**
 * @brief Enhanced step structure for Digitakt-style step sequencer
 * 
 * This class represents a single step in a step sequencer pattern, containing
 * all the data needed for advanced sequencing features including parameter locks,
 * probability, micro-timing, retriggering, and conditional triggers.
 * 
 * **Key Features:**
 * - Parameter locks: Override any parameter value for this step
 * - Probability: Random step triggering (0.0 = never, 1.0 = always)
 * - Micro-timing: Sub-step timing adjustments (-50 to +50 ticks)
 * - Retriggering: Multiple note triggers within the step
 * - Conditional triggers: Context-dependent step behavior
 * 
 * **Integration:**
 * - Works with ParameterLockManager for parameter overrides
 * - Uses ProbabilityEngine for random triggering
 * - Uses MicroTimingEngine for precise timing adjustments
 * - Uses RetriggerEngine for multiple triggers per step
 * - Uses ConditionalTriggerEngine for context-dependent behavior
 * 
 * **Memory Layout:**
 * - Compact structure optimized for real-time performance
 * - Parameter locks stored separately to minimize memory usage
 * - Atomic-safe for concurrent read/write operations
 * 
 * @see ParameterLockManager For parameter lock functionality
 * @see DigitaktFeatures For advanced sequencing engines
 * @see StepSequencer For sequencer integration
 */
class EnhancedSequencerStep {
public:
    /**
     * @brief Copy constructor for step duplication
     * @param other Source step to copy from
     * @note Deep copies all data including parameter locks
     */
    EnhancedSequencerStep(const EnhancedSequencerStep& other);
    
    /**
     * @brief Assignment operator for step copying
     * @param other Source step to copy from
     * @return Reference to this step
     * @note Deep copies all data including parameter locks
     */
    EnhancedSequencerStep& operator=(const EnhancedSequencerStep& other);

private:
    // =============================================================================
    // CORE STEP DATA (Optimized for cache efficiency)
    // =============================================================================
    
    bool active_;                                    ///< Is this step enabled/triggered
    uint8_t note_;                                   ///< MIDI note number (0-127)
    uint8_t velocity_;                               ///< MIDI velocity (0-127)
    uint8_t length_;                                 ///< Note length as percentage of step (0-100)
    
    // Advanced timing features
    float probability_;                              ///< Trigger probability (0.0-1.0)
    int8_t micro_timing_;                           ///< Micro-timing offset (-50 to +50 ticks)
    
    // Retriggering
    uint8_t retrigger_count_;                       ///< Number of retriggers (0-8)
    RetriggerEngine::RetriggerRate retrigger_rate_; ///< Retrigger subdivision
    
    // Conditional triggering
    ConditionalTriggerEngine::TriggerCondition condition_;  ///< Trigger condition type
    uint8_t condition_param_;                       ///< Parameter for condition (context-dependent)
    
    // Parameter locks (stored separately for memory efficiency)
    std::unordered_map<ParameterID, float> parameter_locks_;

public:
    /**
     * @brief Construct a new step with default values
     * @note Creates an inactive step with sensible defaults
     */
    EnhancedSequencerStep();
    
    // =============================================================================
    // BASIC STEP PROPERTIES
    // =============================================================================
    
    /**
     * @brief Enable or disable this step
     * @param active true to enable step triggering, false to disable
     * @note Disabled steps are completely skipped during playback
     */
    void setActive(bool active) { active_ = active; }
    
    /**
     * @brief Check if this step is active
     * @return true if step will trigger (subject to probability and conditions)
     */
    bool isActive() const { return active_; }
    
    /**
     * @brief Set the MIDI note for this step
     * @param note MIDI note number (0-127, typically 21-108 for piano range)
     */
    void setNote(uint8_t note) { note_ = note; }
    
    /**
     * @brief Get the MIDI note for this step
     * @return MIDI note number (0-127)
     */
    uint8_t getNote() const { return note_; }
    
    /**
     * @brief Set the velocity for this step
     * @param velocity MIDI velocity (0-127, 0=silence, 127=maximum)
     */
    void setVelocity(uint8_t velocity) { velocity_ = velocity; }
    
    /**
     * @brief Get the velocity for this step
     * @return MIDI velocity (0-127)
     */
    uint8_t getVelocity() const { return velocity_; }
    
    /**
     * @brief Set note length as percentage of step duration
     * @param length Length percentage (0-100, 100=full step length)
     * @note 50 = staccato, 100 = legato, values >100 may overlap with next step
     */
    void setLength(uint8_t length) { length_ = length; }
    
    /**
     * @brief Get note length as percentage of step duration
     * @return Length percentage (0-100)
     */
    uint8_t getLength() const { return length_; }
    
    // =============================================================================
    // PROBABILITY SYSTEM (Digitakt-style random triggering)
    // =============================================================================
    
    /**
     * @brief Set trigger probability for this step
     * @param probability Chance of triggering (0.0=never, 1.0=always)
     * @note Probability is evaluated each time the step is reached
     * @note Values between 0.0-1.0 create probabilistic triggering
     */
    void setProbability(float probability);
    
    /**
     * @brief Get trigger probability for this step
     * @return Probability value (0.0-1.0)
     */
    float getProbability() const { return probability_; }
    
    /**
     * @brief Evaluate whether step should trigger based on probability
     * @return true if step should trigger this time
     * @note Uses ProbabilityEngine for random evaluation
     * @note Called by sequencer engine during playback
     */
    bool shouldTriggerWithProbability() const;
    
    // =============================================================================
    // MICRO-TIMING SYSTEM (Sub-step timing adjustments)
    // =============================================================================
    
    /**
     * @brief Set micro-timing offset for this step
     * @param offset Timing offset in ticks (-50 to +50)
     * @note Negative values = earlier, positive values = later
     * @note 1 tick ≈ 1/96 of a quarter note (depends on PPQN)
     */
    void setMicroTiming(int8_t offset);
    
    /**
     * @brief Get micro-timing offset for this step
     * @return Timing offset in ticks (-50 to +50)
     */
    int8_t getMicroTiming() const { return micro_timing_; }
    
    /**
     * @brief Calculate actual trigger time with micro-timing applied
     * @param base_time Base step timing in MIDI ticks
     * @return Adjusted timing with micro-timing offset applied
     * @note Uses MicroTimingEngine for precise timing calculation
     */
    uint32_t calculateTriggerTime(uint32_t base_time) const;
    
    // =============================================================================
    // RETRIGGER SYSTEM (Multiple triggers per step)
    // =============================================================================
    
    /**
     * @brief Set number of retriggers for this step
     * @param count Number of additional triggers (0-8, 0=no retriggers)
     * @note 0=single trigger, 1=double trigger, 8=9 total triggers
     */
    void setRetriggerCount(uint8_t count);
    
    /**
     * @brief Get number of retriggers for this step
     * @return Retrigger count (0-8)
     */
    uint8_t getRetriggerCount() const { return retrigger_count_; }
    
    /**
     * @brief Set retrigger timing subdivision
     * @param rate Subdivision for spacing retriggers
     * @note RATE_1_16 = retriggers every 1/16 note within step
     */
    void setRetriggerRate(RetriggerEngine::RetriggerRate rate) { retrigger_rate_ = rate; }
    
    /**
     * @brief Get retrigger timing subdivision
     * @return Current retrigger rate
     */
    RetriggerEngine::RetriggerRate getRetriggerRate() const { return retrigger_rate_; }
    
    /**
     * @brief Calculate all retrigger times for this step
     * @param base_time Base step timing in MIDI ticks
     * @param step_duration Duration of step in MIDI ticks
     * @return Vector of trigger times including main trigger and retriggers
     * @note Uses RetriggerEngine for timing calculations
     */
    std::vector<uint32_t> calculateRetriggerTimes(uint32_t base_time, uint32_t step_duration) const;
    
    /**
     * @brief Calculate velocities for all retriggers
     * @param base_velocity Base step velocity
     * @return Vector of velocities for each retrigger
     * @note First velocity is always base_velocity (main trigger)
     * @note Subsequent velocities may be reduced for natural sound
     */
    std::vector<uint8_t> calculateRetriggerVelocities() const;
    
    // =============================================================================
    // CONDITIONAL TRIGGER SYSTEM (Context-dependent triggering)
    // =============================================================================
    
    /**
     * @brief Set trigger condition for this step
     * @param condition Type of conditional triggering
     * @param param Additional parameter for condition (usage depends on condition type)
     * @note FIRST: param ignored
     * @note NEI: param = which neighbor to check (0=previous, 1=next)
     * @note A_B: param = A/B pattern selector
     */
    void setTriggerCondition(ConditionalTriggerEngine::TriggerCondition condition, uint8_t param = 0);
    
    /**
     * @brief Get trigger condition type
     * @return Current trigger condition
     */
    ConditionalTriggerEngine::TriggerCondition getTriggerCondition() const { return condition_; }
    
    /**
     * @brief Get trigger condition parameter
     * @return Condition-specific parameter value
     */
    uint8_t getTriggerConditionParam() const { return condition_param_; }
    
    /**
     * @brief Evaluate whether step should trigger based on condition
     * @param context Sequencer context for condition evaluation
     * @return true if condition allows triggering
     * @note Uses ConditionalTriggerEngine for evaluation
     * @note Called by sequencer engine before probability evaluation
     */
    bool evaluateTriggerCondition(const SequencerContext& context) const;
    
    // =============================================================================
    // PARAMETER LOCK SYSTEM (Per-step parameter overrides)
    // =============================================================================
    
    /**
     * @brief Set a parameter lock for this step
     * @param param_id Parameter to lock
     * @param value Normalized value to lock parameter to (0.0-1.0)
     * @note Overwrites any existing lock for this parameter
     * @note Parameter will be set to this value when step triggers
     */
    void setParameterLock(ParameterID param_id, float value);
    
    /**
     * @brief Remove a parameter lock from this step
     * @param param_id Parameter to unlock
     * @note Safe to call even if no lock exists
     */
    void removeParameterLock(ParameterID param_id);
    
    /**
     * @brief Check if parameter is locked on this step
     * @param param_id Parameter to check
     * @return true if parameter has a lock on this step
     */
    bool hasParameterLock(ParameterID param_id) const;
    
    /**
     * @brief Get locked value for a parameter
     * @param param_id Parameter to query
     * @return Locked value (0.0-1.0), or 0.0 if no lock exists
     */
    float getParameterLock(ParameterID param_id) const;
    
    /**
     * @brief Get all parameter locks for this step
     * @return Map of parameter IDs to locked values
     * @note Used by ParameterLockManager for bulk operations
     */
    const std::unordered_map<ParameterID, float>& getParameterLocks() const { return parameter_locks_; }
    
    /**
     * @brief Clear all parameter locks from this step
     * @note Removes all parameter overrides, step reverts to track defaults
     */
    void clearAllParameterLocks();
    
    /**
     * @brief Copy parameter locks from another step
     * @param other Source step to copy locks from
     * @note Overwrites all existing locks on this step
     */
    void copyParameterLocksFrom(const EnhancedSequencerStep& other);
    
    /**
     * @brief Get count of parameter locks on this step
     * @return Number of locked parameters
     * @note Useful for UI display and memory usage monitoring
     */
    size_t getParameterLockCount() const { return parameter_locks_.size(); }
    
    // =============================================================================
    // COMPREHENSIVE STEP EVALUATION
    // =============================================================================
    
    /**
     * @brief Evaluate if step should trigger considering all conditions
     * @param context Sequencer context for evaluation
     * @return true if step should trigger this time
     * @note Combines condition evaluation and probability evaluation
     * @note This is the main entry point for step trigger evaluation
     */
    bool shouldTrigger(const SequencerContext& context) const;
    
    /**
     * @brief Get human-readable summary of step configuration
     * @return String describing step settings
     * @note Useful for debugging and UI display
     */
    std::string getConfigurationSummary() const;
    
    // =============================================================================
    // BULK OPERATIONS AND UTILITIES
    // =============================================================================
    
    /**
     * @brief Reset step to default inactive state
     * @note Clears all locks, conditions, and resets to default values
     */
    void reset();
    
    /**
     * @brief Copy all settings from another step
     * @param other Source step to copy from
     * @note Deep copy including parameter locks and all settings
     */
    void copyFrom(const EnhancedSequencerStep& other);
    
    /**
     * @brief Check if step has any advanced features enabled
     * @return true if step uses probability, retriggers, conditions, or locks
     * @note Useful for optimization - simple steps can use faster code paths
     */
    bool hasAdvancedFeatures() const;
    
    /**
     * @brief Get memory usage of this step in bytes
     * @return Approximate memory usage including parameter locks
     * @note Useful for memory monitoring and optimization
     */
    size_t getMemoryUsage() const;
};

} // namespace MIDI
