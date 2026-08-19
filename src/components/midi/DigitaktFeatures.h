#pragma once

#include "components/parameter/ParameterChangeEvent.h"
#include <cstdint>
#include <vector>
#include <random>
#include <chrono>
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace MIDI {

// Forward declarations
class SequencerContext;

/**
 * @brief Probability engine for random step triggering (Digitakt-style)
 * 
 * Provides deterministic random number generation for step probability evaluation.
 * Uses a seeded PRNG to ensure reproducible behavior for pattern playback while
 * still providing pseudo-random triggering based on step probability values.
 * 
 * **Key Features:**
 * - Thread-safe probability evaluation
 * - Seeded random generation for reproducible patterns
 * - Optimized for real-time audio thread usage
 * - Per-step probability evaluation (0.0 = never, 1.0 = always)
 * 
 * **Integration:**
 * - Called by StepSequencer during step evaluation
 * - Works with SequencerStep probability values
 * - Thread-safe for concurrent step processing
 * 
 * **Usage Example:**
 * ```cpp
 * ProbabilityEngine::setSeed(12345);  // Set reproducible seed
 * bool trigger = ProbabilityEngine::shouldTrigger(0.75f);  // 75% chance
 * ```
 * 
 * @see SequencerStep For per-step probability configuration
 * @see StepSequencer For sequencer integration
 */
class ProbabilityEngine {
public:
    /**
     * @brief Evaluate whether a step should trigger based on probability
     * @param probability Trigger probability (0.0 = never, 1.0 = always)
     * @return true if step should trigger this time
     * @note Thread-safe, can be called from sequencer thread
     * @note Uses internal thread-local random state for performance
     */
    static bool shouldTrigger(float probability);
    
    /**
     * @brief Set random seed for reproducible pattern behavior
     * @param seed Random seed value
     * @note Affects all subsequent probability evaluations
     * @note Use same seed for identical pattern behavior across sessions
     */
    static void setSeed(uint32_t seed);
    
    /**
     * @brief Get current random seed
     * @return Current seed value
     * @note Useful for pattern save/load functionality
     */
    static uint32_t getSeed();
    
    /**
     * @brief Reset random state to time-based seed
     * @note Creates truly random behavior based on current time
     * @note Call when user wants non-reproducible random patterns
     */
    static void randomizeSeed();

private:
    static thread_local std::mt19937 rng_;          ///< Thread-local random generator
    static std::atomic<uint32_t> global_seed_;      ///< Global seed for new threads
};

/**
 * @brief Micro-timing engine for sub-step timing adjustments
 * 
 * Provides precise timing calculations for step micro-timing, allowing steps
 * to be triggered slightly early or late relative to their quantized position.
 * This creates more human-like timing and groove in sequenced patterns.
 * 
 * **Key Features:**
 * - Sub-tick timing resolution (1/96th note precision)
 * - Positive and negative timing offsets (-50 to +50 ticks)
 * - Integration with MIDI clock timing
 * - Swing timing calculation support
 * 
 * **Integration:**
 * - Works with StepSequencer for precise timing
 * - Uses MIDI clock PPQN (pulses per quarter note) for calculations
 * - Thread-safe for real-time audio thread usage
 * 
 * **Usage Example:**
 * ```cpp
 * uint32_t base_time = 480;  // Quarter note at 480 PPQN
 * int8_t micro_offset = -10; // 10 ticks early
 * uint32_t actual_time = MicroTimingEngine::calculateTiming(base_time, micro_offset);
 * ```
 * 
 * @see SequencerStep For per-step micro-timing configuration
 * @see StepSequencer For timing integration
 */
class MicroTimingEngine {
public:
    /**
     * @brief Calculate actual trigger time with micro-timing applied
     * @param base_time Base quantized timing in MIDI ticks
     * @param micro_offset Timing offset in ticks (-50 to +50)
     * @return Adjusted timing with micro-timing offset applied
     * @note Negative offsets = earlier timing, positive = later timing
     * @note Thread-safe, no state modification
     */
    static uint32_t calculateTiming(uint32_t base_time, int8_t micro_offset);
    
    /**
     * @brief Calculate swing timing adjustment
     * @param base_time Base quantized timing in MIDI ticks
     * @param step_index Step number in pattern (0-based)
     * @param swing_amount Swing percentage (0.0 = straight, 1.0 = maximum swing)
     * @return Timing with swing adjustment applied
     * @note Swing affects every other step (8th note swing on 16th patterns)
     * @note Thread-safe, stateless calculation
     */
    static uint32_t calculateSwingTiming(uint32_t base_time, int step_index, float swing_amount);
    
    /**
     * @brief Validate micro-timing offset value
     * @param offset Timing offset to validate
     * @return Clamped offset within valid range (-50 to +50)
     * @note Ensures offset values stay within hardware-realistic limits
     */
    static int8_t clampMicroTiming(int8_t offset);
    
    /**
     * @brief Convert micro-timing offset to milliseconds
     * @param offset Timing offset in ticks
     * @param bpm Current tempo in beats per minute
     * @param ppqn Pulses per quarter note (typically 24 or 96)
     * @return Timing offset in milliseconds
     * @note Useful for display and debugging purposes
     */
    static float offsetToMilliseconds(int8_t offset, float bpm, int ppqn = 24);

private:
    static constexpr int8_t MIN_MICRO_TIMING = -50;  ///< Minimum timing offset (early)
    static constexpr int8_t MAX_MICRO_TIMING = 50;   ///< Maximum timing offset (late)
    static constexpr float MAX_SWING_OFFSET = 0.33f; ///< Maximum swing as fraction of beat
};

/**
 * @brief Retrigger engine for multiple note triggers per step
 * 
 * Calculates timing for multiple note triggers within a single step duration.
 * This allows for rapid-fire note repetitions, tremolo effects, and rhythmic
 * subdivisions within individual sequencer steps.
 * 
 * **Key Features:**
 * - Configurable retrigger count (0-8 additional triggers)
 * - Multiple subdivision rates (1/32, 1/16, 1/8, 1/4, 1/2 notes)
 * - Even spacing within step duration
 * - Velocity ramping support for natural-sounding retriggers
 * 
 * **Integration:**
 * - Called by StepSequencer for retrigger scheduling
 * - Works with SequencerStep retrigger configuration
 * - Generates multiple MIDI events per step
 * 
 * **Usage Example:**
 * ```cpp
 * uint32_t step_start = 0;
 * uint32_t step_duration = 96;  // 1/16 note at 384 PPQN
 * auto times = RetriggerEngine::calculateRetriggerTimes(step_start, step_duration, 3, RetriggerRate::RATE_1_32);
 * // Returns: [0, 24, 48, 72] - 4 total triggers evenly spaced
 * ```
 * 
 * @see SequencerStep For retrigger configuration
 * @see StepSequencer For retrigger scheduling
 */
class RetriggerEngine {
public:
    /**
     * @brief Retrigger timing subdivision rates
     * 
     * Defines how retriggers are spaced within the step duration.
     * Higher values = faster retriggering.
     */
    enum class RetriggerRate : uint8_t {
        RATE_1_32 = 32,  ///< 1/32 note retriggering (very fast)
        RATE_1_16 = 16,  ///< 1/16 note retriggering (fast)
        RATE_1_8 = 8,    ///< 1/8 note retriggering (medium)
        RATE_1_4 = 4,    ///< 1/4 note retriggering (slow)
        RATE_1_2 = 2     ///< 1/2 note retriggering (very slow)
    };
    
    /**
     * @brief Calculate all retrigger times for a step
     * @param step_start_time Start time of step in MIDI ticks
     * @param step_duration Duration of step in MIDI ticks
     * @param retrigger_count Number of additional triggers (0-8)
     * @param rate Subdivision rate for retrigger spacing
     * @return Vector of trigger times including main trigger and retriggers
     * @note First time is always step_start_time (main trigger)
     * @note Additional times are evenly spaced based on rate and count
     */
    static std::vector<uint32_t> calculateRetriggerTimes(
        uint32_t step_start_time,
        uint32_t step_duration,
        uint8_t retrigger_count,
        RetriggerRate rate
    );
    
    /**
     * @brief Calculate velocity for each retrigger
     * @param base_velocity Original step velocity (0-127)
     * @param retrigger_index Index of retrigger (0 = main trigger)
     * @param total_retriggers Total number of retriggers
     * @param velocity_curve Velocity ramping curve (0.0 = linear, 1.0 = exponential)
     * @return Calculated velocity for this retrigger
     * @note Velocities typically decrease for each retrigger to sound natural
     */
    static uint8_t calculateRetriggerVelocity(
        uint8_t base_velocity,
        int retrigger_index,
        int total_retriggers,
        float velocity_curve = 0.7f
    );
    
    /**
     * @brief Validate retrigger count
     * @param count Retrigger count to validate
     * @return Clamped count within valid range (0-8)
     */
    static uint8_t clampRetriggerCount(uint8_t count);
    
    /**
     * @brief Convert retrigger rate to subdivision ticks
     * @param rate Retrigger rate enum
     * @param ppqn Pulses per quarter note
     * @return Number of ticks for one subdivision at this rate
     */
    static uint32_t rateToTicks(RetriggerRate rate, int ppqn = 24);

private:
    static constexpr uint8_t MAX_RETRIGGERS = 8;     ///< Maximum retrigger count
    static constexpr float DEFAULT_VELOCITY_CURVE = 0.7f;  ///< Default velocity ramping
};

/**
 * @brief Conditional trigger engine for context-dependent step behavior
 * 
 * Evaluates whether steps should trigger based on sequencer context such as
 * pattern loop count, fill mode state, neighboring step activity, and pattern
 * chain position. This enables complex conditional sequencing patterns.
 * 
 * **Key Features:**
 * - Multiple condition types (FIRST, NOT_FIRST, FILL, etc.)
 * - Context-aware evaluation based on sequencer state
 * - Pattern loop counting for evolving sequences
 * - Fill mode integration for live performance
 * 
 * **Condition Types:**
 * - FIRST: Only trigger on first pattern loop
 * - NOT_FIRST: Trigger on all loops except first
 * - FILL: Only trigger during fill mode
 * - NOT_FILL: Never trigger during fill mode
 * - NEI: Trigger based on neighboring step state
 * 
 * **Integration:**
 * - Called by StepSequencer before probability evaluation
 * - Requires SequencerContext for state information
 * - Works with pattern loop counting and fill mode
 * 
 * @see SequencerStep For condition configuration
 * @see SequencerContext For context information
 * @see FillModeManager For fill mode integration
 */
class ConditionalTriggerEngine {
public:
    /**
     * @brief Conditional trigger types
     * 
     * Defines different types of conditional step triggering based on
     * sequencer context and state.
     */
    enum class TriggerCondition : uint8_t {
        NONE = 0,        ///< Always trigger (if active and probability succeeds)
        FIRST = 1,       ///< Only trigger on first pattern loop
        NOT_FIRST = 2,   ///< Trigger on all loops except first
        FILL = 3,        ///< Only trigger during fill mode
        NOT_FILL = 4,    ///< Never trigger during fill mode
        PRE = 5,         ///< Trigger one step before main trigger
        NEI = 6,         ///< Trigger based on neighboring step state
        A_B = 7,         ///< Alternate between A and B patterns
        NOT_A_B = 8      ///< Trigger when not in A/B pattern mode
    };
    
    /**
     * @brief Evaluate whether step should trigger based on condition
     * @param condition Trigger condition type
     * @param condition_param Additional parameter for condition (usage varies)
     * @param context Current sequencer context for evaluation
     * @return true if condition allows triggering
     * @note Called before probability evaluation
     * @note Thread-safe, no state modification
     */
    static bool evaluateCondition(
        TriggerCondition condition,
        uint8_t condition_param,
        const SequencerContext& context
    );
    
    /**
     * @brief Get human-readable name for condition type
     * @param condition Condition type to describe
     * @return String description of condition
     * @note Useful for UI display and debugging
     */
    static const char* getConditionName(TriggerCondition condition);
    
    /**
     * @brief Validate condition parameter for given condition type
     * @param condition Condition type
     * @param param Parameter value to validate
     * @return Validated parameter value
     * @note Ensures parameter values are within valid ranges for each condition
     */
    static uint8_t validateConditionParam(TriggerCondition condition, uint8_t param);

private:
    /**
     * @brief Evaluate FIRST condition (only on first loop)
     * @param context Sequencer context
     * @return true if this is the first pattern loop
     */
    static bool evaluateFirstCondition(const SequencerContext& context);
    
    /**
     * @brief Evaluate NOT_FIRST condition (all loops except first)
     * @param context Sequencer context
     * @return true if this is not the first pattern loop
     */
    static bool evaluateNotFirstCondition(const SequencerContext& context);
    
    /**
     * @brief Evaluate FILL condition (only during fill mode)
     * @param context Sequencer context
     * @return true if fill mode is active
     */
    static bool evaluateFillCondition(const SequencerContext& context);
    
    /**
     * @brief Evaluate NOT_FILL condition (never during fill)
     * @param context Sequencer context
     * @return true if fill mode is not active
     */
    static bool evaluateNotFillCondition(const SequencerContext& context);
    
    /**
     * @brief Evaluate NEI condition (neighbor-dependent)
     * @param condition_param Which neighbor to check (0=previous, 1=next)
     * @param context Sequencer context
     * @return true if neighbor condition is met
     */
    static bool evaluateNeighborCondition(uint8_t condition_param, const SequencerContext& context);
};

/**
 * @brief Fill mode manager for live performance fills
 * 
 * Manages fill mode state for live performance scenarios. Fill mode temporarily
 * modifies pattern behavior, typically adding or changing step activity to create
 * drum fills, breakdown sections, or build-ups during live performance.
 * 
 * **Key Features:**
 * - Global fill mode state management
 * - Per-track fill pattern storage
 * - Automatic return to normal pattern after fill
 * - Integration with conditional triggers
 * 
 * **Integration:**
 * - Used by ConditionalTriggerEngine for FILL/NOT_FILL conditions
 * - Integrates with StepSequencer for pattern modification
 * - Thread-safe for live performance use
 * 
 * **Usage Example:**
 * ```cpp
 * FillModeManager::getInstance().enterFill();  // Start fill mode
 * // Fill patterns are now active
 * FillModeManager::getInstance().exitFill();   // Return to normal
 * ```
 * 
 * @see ConditionalTriggerEngine For fill-based conditional triggers
 * @see StepSequencer For pattern integration
 */
class FillModeManager {
public:
    /**
     * @brief Get singleton instance of fill mode manager
     * @return Reference to global fill mode manager
     * @note Thread-safe singleton implementation
     */
    static FillModeManager& getInstance();
    
    /**
     * @brief Enter fill mode
     * @note Activates fill patterns across all tracks
     * @note Thread-safe, can be called from UI thread
     */
    void enterFill();
    
    /**
     * @brief Exit fill mode and return to normal patterns
     * @note Restores original pattern state
     * @note Thread-safe, can be called from UI thread
     */
    void exitFill();
    
    /**
     * @brief Check if fill mode is currently active
     * @return true if in fill mode
     * @note Thread-safe atomic read
     */
    bool isActive() const { return active_.load(); }
    
    /**
     * @brief Set fill pattern for a specific track
     * @param track_id Track number (0-based)
     * @param fill_pattern Fill pattern steps (typically more active than normal)
     * @note Fill pattern is applied when fill mode is entered
     */
    void setTrackFillPattern(int track_id, const std::vector<bool>& fill_pattern);
    
    /**
     * @brief Get fill pattern for a specific track
     * @param track_id Track number (0-based)
     * @return Fill pattern for this track
     */
    const std::vector<bool>& getTrackFillPattern(int track_id) const;
    
    /**
     * @brief Clear fill pattern for a specific track
     * @param track_id Track number (0-based)
     * @note Track will use normal pattern during fills
     */
    void clearTrackFillPattern(int track_id);

private:
    FillModeManager() = default;
    ~FillModeManager() = default;
    FillModeManager(const FillModeManager&) = delete;
    FillModeManager& operator=(const FillModeManager&) = delete;
    
    std::atomic<bool> active_{false};              ///< Fill mode active state
    std::unordered_map<int, std::vector<bool>> fill_patterns_;  ///< Per-track fill patterns
    mutable std::mutex fill_patterns_mutex_;       ///< Protect fill pattern storage
};

/**
 * @brief Sequencer context for condition evaluation
 * 
 * Provides all necessary context information for conditional trigger evaluation.
 * This includes current pattern state, loop count, fill mode status, and
 * neighboring step information.
 * 
 * **Context Information:**
 * - Current step and track being evaluated
 * - Pattern loop count for FIRST/NOT_FIRST conditions
 * - Fill mode state for FILL/NOT_FILL conditions
 * - Neighboring step states for NEI conditions
 * - Pattern chain state for A_B conditions
 * 
 * **Usage:**
 * Created by StepSequencer and passed to conditional trigger evaluation.
 * Provides read-only access to sequencer state for condition evaluation.
 * 
 * @see ConditionalTriggerEngine For condition evaluation
 * @see StepSequencer For context creation
 */
class SequencerContext {
public:
    /**
     * @brief Construct sequencer context
     * @param current_track Currently evaluating track
     * @param current_step Currently evaluating step
     * @param loop_count Number of times pattern has looped
     * @param is_fill_active Whether fill mode is active
     */
    SequencerContext(int current_track, int current_step, int loop_count, bool is_fill_active);
    
    /**
     * @brief Get current track being evaluated
     * @return Track number (0-based)
     */
    int getCurrentTrack() const { return current_track_; }
    
    /**
     * @brief Get current step being evaluated
     * @return Step number (0-based)
     */
    int getCurrentStep() const { return current_step_; }
    
    /**
     * @brief Get pattern loop count
     * @return Number of times pattern has completed (0-based)
     */
    int getLoopCount() const { return loop_count_; }
    
    /**
     * @brief Check if fill mode is active
     * @return true if in fill mode
     */
    bool isFillActive() const { return is_fill_active_; }
    
    /**
     * @brief Set neighbor step states for NEI condition evaluation
     * @param previous_active Whether previous step is active
     * @param next_active Whether next step is active
     */
    void setNeighborStates(bool previous_active, bool next_active);
    
    /**
     * @brief Get previous step active state
     * @return true if previous step is active
     */
    bool isPreviousStepActive() const { return previous_step_active_; }
    
    /**
     * @brief Get next step active state
     * @return true if next step is active
     */
    bool isNextStepActive() const { return next_step_active_; }
    
    /**
     * @brief Set pattern chain state for A_B conditions
     * @param pattern_a_active Whether pattern A is active
     * @param pattern_b_active Whether pattern B is active
     */
    void setPatternChainState(bool pattern_a_active, bool pattern_b_active);
    
    /**
     * @brief Check if pattern A is active in chain
     * @return true if pattern A is active
     */
    bool isPatternAActive() const { return pattern_a_active_; }
    
    /**
     * @brief Check if pattern B is active in chain
     * @return true if pattern B is active
     */
    bool isPatternBActive() const { return pattern_b_active_; }

private:
    int current_track_;           ///< Currently evaluating track
    int current_step_;            ///< Currently evaluating step
    int loop_count_;              ///< Pattern loop count
    bool is_fill_active_;         ///< Fill mode state
    
    // Neighbor step states for NEI conditions
    bool previous_step_active_ = false;   ///< Previous step active state
    bool next_step_active_ = false;       ///< Next step active state
    
    // Pattern chain states for A_B conditions
    bool pattern_a_active_ = false;       ///< Pattern A active in chain
    bool pattern_b_active_ = false;       ///< Pattern B active in chain
};

} // namespace MIDI
