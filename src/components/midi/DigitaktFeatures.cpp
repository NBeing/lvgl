#include "DigitaktFeatures.h"
#include <algorithm>
#include <iostream>
#include <random>

namespace MIDI {

// =============================================================================
// ProbabilityEngine Implementation
// =============================================================================

thread_local std::mt19937 ProbabilityEngine::rng_(std::random_device{}());
std::atomic<uint32_t> ProbabilityEngine::global_seed_{0};

bool ProbabilityEngine::shouldTrigger(float probability) {
    // Clamp probability to valid range
    probability = std::clamp(probability, 0.0f, 1.0f);
    
    // Generate random value between 0.0 and 1.0
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float random_value = dist(rng_);
    
    return random_value <= probability;
}

void ProbabilityEngine::setSeed(uint32_t seed) {
    global_seed_.store(seed);
    rng_.seed(seed);
}

uint32_t ProbabilityEngine::getSeed() {
    return global_seed_.load();
}

void ProbabilityEngine::randomizeSeed() {
    uint32_t time_seed = static_cast<uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count()
    );
    setSeed(time_seed);
}

// =============================================================================
// MicroTimingEngine Implementation
// =============================================================================

uint32_t MicroTimingEngine::calculateTiming(uint32_t base_time, int8_t micro_offset) {
    // Clamp offset to valid range
    int8_t clamped_offset = clampMicroTiming(micro_offset);
    
    // Apply offset (can result in negative timing, caller should handle)
    int32_t adjusted_time = static_cast<int32_t>(base_time) + clamped_offset;
    
    // Ensure we don't go negative
    return static_cast<uint32_t>(std::max(0, adjusted_time));
}

uint32_t MicroTimingEngine::calculateSwingTiming(uint32_t base_time, int step_index, float swing_amount) {
    // Clamp swing amount
    swing_amount = std::clamp(swing_amount, 0.0f, 1.0f);
    
    // Only apply swing to off-beats (odd step indices in 16th note patterns)
    if (step_index % 2 == 0 || swing_amount == 0.0f) {
        return base_time;  // No swing on downbeats or when swing is 0
    }
    
    // Calculate swing offset (as fraction of 16th note)
    // At maximum swing, off-beats are delayed by 1/3 of 16th note
    float swing_offset_ticks = swing_amount * MAX_SWING_OFFSET * 24.0f;  // 24 ticks = 1/16 at PPQN=96
    
    return base_time + static_cast<uint32_t>(swing_offset_ticks);
}

int8_t MicroTimingEngine::clampMicroTiming(int8_t offset) {
    return std::clamp(offset, MIN_MICRO_TIMING, MAX_MICRO_TIMING);
}

float MicroTimingEngine::offsetToMilliseconds(int8_t offset, float bpm, int ppqn) {
    if (bpm <= 0.0f || ppqn <= 0) return 0.0f;
    
    // Calculate milliseconds per tick
    float ms_per_beat = 60000.0f / bpm;  // 60000 ms per minute
    float ms_per_tick = ms_per_beat / ppqn;
    
    return offset * ms_per_tick;
}

// =============================================================================
// RetriggerEngine Implementation
// =============================================================================

std::vector<uint32_t> RetriggerEngine::calculateRetriggerTimes(
    uint32_t step_start_time,
    uint32_t step_duration,
    uint8_t retrigger_count,
    RetriggerRate rate
) {
    std::vector<uint32_t> trigger_times;
    
    // Clamp retrigger count
    retrigger_count = clampRetriggerCount(retrigger_count);
    
    // Always include the main trigger at step start
    trigger_times.push_back(step_start_time);
    
    if (retrigger_count == 0) {
        return trigger_times;  // No retriggers
    }
    
    // Calculate spacing between retriggers
    uint32_t retrigger_interval = step_duration / (retrigger_count + 1);
    
    // Add retriggers evenly spaced within step duration
    for (uint8_t i = 1; i <= retrigger_count; ++i) {
        uint32_t retrigger_time = step_start_time + (i * retrigger_interval);
        trigger_times.push_back(retrigger_time);
    }
    
    return trigger_times;
}

uint8_t RetriggerEngine::calculateRetriggerVelocity(
    uint8_t base_velocity,
    int retrigger_index,
    int total_retriggers,
    float velocity_curve
) {
    if (retrigger_index == 0) {
        return base_velocity;  // Main trigger uses full velocity
    }
    
    if (total_retriggers <= 1) {
        return base_velocity;  // No velocity ramping for single retrigger
    }
    
    // Calculate velocity scaling factor
    float position = static_cast<float>(retrigger_index) / total_retriggers;
    
    // Apply velocity curve (0.0 = linear, 1.0 = exponential decay)
    float velocity_scale;
    if (velocity_curve == 0.0f) {
        velocity_scale = 1.0f - position;  // Linear decay
    } else {
        velocity_scale = std::pow(1.0f - position, velocity_curve);  // Curved decay
    }
    
    // Apply scaling to base velocity
    uint8_t scaled_velocity = static_cast<uint8_t>(base_velocity * velocity_scale);
    
    // Ensure minimum velocity for audible retriggers
    return std::max(scaled_velocity, static_cast<uint8_t>(10));
}

uint8_t RetriggerEngine::clampRetriggerCount(uint8_t count) {
    return std::min(count, MAX_RETRIGGERS);
}

uint32_t RetriggerEngine::rateToTicks(RetriggerRate rate, int ppqn) {
    // Convert retrigger rate to ticks based on PPQN
    int rate_value = static_cast<int>(rate);
    return ppqn / (rate_value / 4);  // Convert to quarter note fractions
}

// =============================================================================
// ConditionalTriggerEngine Implementation
// =============================================================================

bool ConditionalTriggerEngine::evaluateCondition(
    TriggerCondition condition,
    uint8_t condition_param,
    const SequencerContext& context
) {
    switch (condition) {
        case TriggerCondition::NONE:
            return true;  // Always trigger
            
        case TriggerCondition::FIRST:
            return evaluateFirstCondition(context);
            
        case TriggerCondition::NOT_FIRST:
            return evaluateNotFirstCondition(context);
            
        case TriggerCondition::FILL:
            return evaluateFillCondition(context);
            
        case TriggerCondition::NOT_FILL:
            return evaluateNotFillCondition(context);
            
        case TriggerCondition::NEI:
            return evaluateNeighborCondition(condition_param, context);
            
        case TriggerCondition::A_B:
            return context.isPatternAActive() || context.isPatternBActive();
            
        case TriggerCondition::NOT_A_B:
            return !context.isPatternAActive() && !context.isPatternBActive();
            
        case TriggerCondition::PRE:
            // PRE condition would need additional sequencer integration
            return true;  // Placeholder implementation
            
        default:
            return true;  // Unknown condition, default to trigger
    }
}

const char* ConditionalTriggerEngine::getConditionName(TriggerCondition condition) {
    switch (condition) {
        case TriggerCondition::NONE: return "None";
        case TriggerCondition::FIRST: return "First";
        case TriggerCondition::NOT_FIRST: return "Not First";
        case TriggerCondition::FILL: return "Fill";
        case TriggerCondition::NOT_FILL: return "Not Fill";
        case TriggerCondition::PRE: return "Pre";
        case TriggerCondition::NEI: return "Neighbor";
        case TriggerCondition::A_B: return "A/B";
        case TriggerCondition::NOT_A_B: return "Not A/B";
        default: return "Unknown";
    }
}

uint8_t ConditionalTriggerEngine::validateConditionParam(TriggerCondition condition, uint8_t param) {
    switch (condition) {
        case TriggerCondition::NEI:
            return std::min(param, static_cast<uint8_t>(1));  // 0=previous, 1=next
            
        case TriggerCondition::A_B:
        case TriggerCondition::NOT_A_B:
            return std::min(param, static_cast<uint8_t>(1));  // 0=A, 1=B
            
        default:
            return param;  // No validation needed for other conditions
    }
}

bool ConditionalTriggerEngine::evaluateFirstCondition(const SequencerContext& context) {
    return context.getLoopCount() == 0;
}

bool ConditionalTriggerEngine::evaluateNotFirstCondition(const SequencerContext& context) {
    return context.getLoopCount() > 0;
}

bool ConditionalTriggerEngine::evaluateFillCondition(const SequencerContext& context) {
    return context.isFillActive();
}

bool ConditionalTriggerEngine::evaluateNotFillCondition(const SequencerContext& context) {
    return !context.isFillActive();
}

bool ConditionalTriggerEngine::evaluateNeighborCondition(uint8_t condition_param, const SequencerContext& context) {
    if (condition_param == 0) {
        return context.isPreviousStepActive();  // Check previous step
    } else {
        return context.isNextStepActive();      // Check next step
    }
}

// =============================================================================
// FillModeManager Implementation
// =============================================================================

FillModeManager& FillModeManager::getInstance() {
    static FillModeManager instance;
    return instance;
}

void FillModeManager::enterFill() {
    active_.store(true);
    std::cout << "[FillMode] Entering fill mode" << std::endl;
}

void FillModeManager::exitFill() {
    active_.store(false);
    std::cout << "[FillMode] Exiting fill mode" << std::endl;
}

void FillModeManager::setTrackFillPattern(int track_id, const std::vector<bool>& fill_pattern) {
    std::lock_guard<std::mutex> lock(fill_patterns_mutex_);
    fill_patterns_[track_id] = fill_pattern;
}

const std::vector<bool>& FillModeManager::getTrackFillPattern(int track_id) const {
    std::lock_guard<std::mutex> lock(fill_patterns_mutex_);
    
    static const std::vector<bool> empty_pattern;
    auto it = fill_patterns_.find(track_id);
    return (it != fill_patterns_.end()) ? it->second : empty_pattern;
}

void FillModeManager::clearTrackFillPattern(int track_id) {
    std::lock_guard<std::mutex> lock(fill_patterns_mutex_);
    fill_patterns_.erase(track_id);
}

// =============================================================================
// SequencerContext Implementation
// =============================================================================

SequencerContext::SequencerContext(int current_track, int current_step, int loop_count, bool is_fill_active)
    : current_track_(current_track)
    , current_step_(current_step)
    , loop_count_(loop_count)
    , is_fill_active_(is_fill_active) {
}

void SequencerContext::setNeighborStates(bool previous_active, bool next_active) {
    previous_step_active_ = previous_active;
    next_step_active_ = next_active;
}

void SequencerContext::setPatternChainState(bool pattern_a_active, bool pattern_b_active) {
    pattern_a_active_ = pattern_a_active;
    pattern_b_active_ = pattern_b_active;
}

} // namespace MIDI
