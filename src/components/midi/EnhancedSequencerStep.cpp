#include "EnhancedSequencerStep.h"
#include <algorithm>
#include <sstream>

namespace MIDI {

// =============================================================================
// EnhancedSequencerStep Implementation
// =============================================================================

EnhancedSequencerStep::EnhancedSequencerStep()
    : active_(false)
    , note_(60)  // C4
    , velocity_(100)
    , length_(80)  // 80% of step duration
    , probability_(1.0f)  // Always trigger when active
    , micro_timing_(0)   // No timing offset
    , retrigger_count_(0)  // No retriggers
    , retrigger_rate_(RetriggerEngine::RetriggerRate::RATE_1_16)
    , condition_(ConditionalTriggerEngine::TriggerCondition::NONE)
    , condition_param_(0) {
}

EnhancedSequencerStep::EnhancedSequencerStep(const EnhancedSequencerStep& other)
    : active_(other.active_)
    , note_(other.note_)
    , velocity_(other.velocity_)
    , length_(other.length_)
    , probability_(other.probability_)
    , micro_timing_(other.micro_timing_)
    , retrigger_count_(other.retrigger_count_)
    , retrigger_rate_(other.retrigger_rate_)
    , condition_(other.condition_)
    , condition_param_(other.condition_param_)
    , parameter_locks_(other.parameter_locks_) {
}

EnhancedSequencerStep& EnhancedSequencerStep::operator=(const EnhancedSequencerStep& other) {
    if (this != &other) {
        active_ = other.active_;
        note_ = other.note_;
        velocity_ = other.velocity_;
        length_ = other.length_;
        probability_ = other.probability_;
        micro_timing_ = other.micro_timing_;
        retrigger_count_ = other.retrigger_count_;
        retrigger_rate_ = other.retrigger_rate_;
        condition_ = other.condition_;
        condition_param_ = other.condition_param_;
        parameter_locks_ = other.parameter_locks_;
    }
    return *this;
}

// =============================================================================
// PROBABILITY SYSTEM
// =============================================================================

void EnhancedSequencerStep::setProbability(float probability) {
    probability_ = std::clamp(probability, 0.0f, 1.0f);
}

bool EnhancedSequencerStep::shouldTriggerWithProbability() const {
    return ProbabilityEngine::shouldTrigger(probability_);
}

// =============================================================================
// MICRO-TIMING SYSTEM
// =============================================================================

void EnhancedSequencerStep::setMicroTiming(int8_t offset) {
    micro_timing_ = MicroTimingEngine::clampMicroTiming(offset);
}

uint32_t EnhancedSequencerStep::calculateTriggerTime(uint32_t base_time) const {
    return MicroTimingEngine::calculateTiming(base_time, micro_timing_);
}

// =============================================================================
// RETRIGGER SYSTEM
// =============================================================================

void EnhancedSequencerStep::setRetriggerCount(uint8_t count) {
    retrigger_count_ = RetriggerEngine::clampRetriggerCount(count);
}

std::vector<uint32_t> EnhancedSequencerStep::calculateRetriggerTimes(uint32_t base_time, uint32_t step_duration) const {
    return RetriggerEngine::calculateRetriggerTimes(base_time, step_duration, retrigger_count_, retrigger_rate_);
}

std::vector<uint8_t> EnhancedSequencerStep::calculateRetriggerVelocities() const {
    std::vector<uint8_t> velocities;
    int total_triggers = retrigger_count_ + 1;
    
    for (int i = 0; i < total_triggers; ++i) {
        uint8_t velocity = RetriggerEngine::calculateRetriggerVelocity(velocity_, i, total_triggers);
        velocities.push_back(velocity);
    }
    
    return velocities;
}

// =============================================================================
// CONDITIONAL TRIGGER SYSTEM
// =============================================================================

void EnhancedSequencerStep::setTriggerCondition(ConditionalTriggerEngine::TriggerCondition condition, uint8_t param) {
    condition_ = condition;
    condition_param_ = ConditionalTriggerEngine::validateConditionParam(condition, param);
}

bool EnhancedSequencerStep::evaluateTriggerCondition(const SequencerContext& context) const {
    return ConditionalTriggerEngine::evaluateCondition(condition_, condition_param_, context);
}

// =============================================================================
// PARAMETER LOCK SYSTEM
// =============================================================================

void EnhancedSequencerStep::setParameterLock(ParameterID param_id, float value) {
    value = std::clamp(value, 0.0f, 1.0f);
    parameter_locks_[param_id] = value;
}

void EnhancedSequencerStep::removeParameterLock(ParameterID param_id) {
    parameter_locks_.erase(param_id);
}

bool EnhancedSequencerStep::hasParameterLock(ParameterID param_id) const {
    return parameter_locks_.find(param_id) != parameter_locks_.end();
}

float EnhancedSequencerStep::getParameterLock(ParameterID param_id) const {
    auto it = parameter_locks_.find(param_id);
    return (it != parameter_locks_.end()) ? it->second : 0.0f;
}

void EnhancedSequencerStep::clearAllParameterLocks() {
    parameter_locks_.clear();
}

void EnhancedSequencerStep::copyParameterLocksFrom(const EnhancedSequencerStep& other) {
    parameter_locks_ = other.parameter_locks_;
}

// =============================================================================
// COMPREHENSIVE STEP EVALUATION
// =============================================================================

bool EnhancedSequencerStep::shouldTrigger(const SequencerContext& context) const {
    // Step must be active
    if (!active_) {
        return false;
    }
    
    // Evaluate conditional trigger first
    if (!evaluateTriggerCondition(context)) {
        return false;
    }
    
    // Evaluate probability last (most expensive due to RNG)
    return shouldTriggerWithProbability();
}

std::string EnhancedSequencerStep::getConfigurationSummary() const {
    std::ostringstream oss;
    
    oss << "Step: ";
    if (!active_) {
        oss << "INACTIVE";
        return oss.str();
    }
    
    oss << "Note=" << static_cast<int>(note_) 
        << " Vel=" << static_cast<int>(velocity_)
        << " Len=" << static_cast<int>(length_) << "%";
    
    if (probability_ < 1.0f) {
        oss << " Prob=" << (probability_ * 100.0f) << "%";
    }
    
    if (micro_timing_ != 0) {
        oss << " Timing=" << static_cast<int>(micro_timing_);
    }
    
    if (retrigger_count_ > 0) {
        oss << " Retrig=" << static_cast<int>(retrigger_count_);
    }
    
    if (condition_ != ConditionalTriggerEngine::TriggerCondition::NONE) {
        oss << " Condition=" << ConditionalTriggerEngine::getConditionName(condition_);
    }
    
    if (!parameter_locks_.empty()) {
        oss << " Locks=" << parameter_locks_.size();
    }
    
    return oss.str();
}

// =============================================================================
// BULK OPERATIONS AND UTILITIES
// =============================================================================

void EnhancedSequencerStep::reset() {
    active_ = false;
    note_ = 60;
    velocity_ = 100;
    length_ = 80;
    probability_ = 1.0f;
    micro_timing_ = 0;
    retrigger_count_ = 0;
    retrigger_rate_ = RetriggerEngine::RetriggerRate::RATE_1_16;
    condition_ = ConditionalTriggerEngine::TriggerCondition::NONE;
    condition_param_ = 0;
    parameter_locks_.clear();
}

void EnhancedSequencerStep::copyFrom(const EnhancedSequencerStep& other) {
    *this = other;
}

bool EnhancedSequencerStep::hasAdvancedFeatures() const {
    return (probability_ < 1.0f) ||           // Probability enabled
           (micro_timing_ != 0) ||            // Micro-timing enabled
           (retrigger_count_ > 0) ||          // Retriggers enabled
           (condition_ != ConditionalTriggerEngine::TriggerCondition::NONE) ||  // Conditions enabled
           (!parameter_locks_.empty());       // Parameter locks present
}

size_t EnhancedSequencerStep::getMemoryUsage() const {
    size_t base_size = sizeof(EnhancedSequencerStep);
    
    // Add memory for parameter locks
    size_t locks_size = parameter_locks_.size() * (sizeof(ParameterID) + sizeof(float));
    
    // Add hash table overhead (approximate)
    size_t hash_overhead = parameter_locks_.bucket_count() * sizeof(void*);
    
    return base_size + locks_size + hash_overhead;
}

} // namespace MIDI
