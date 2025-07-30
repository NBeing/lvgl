#pragma once

#include "components/parameter/Parameter.h"
#include <functional>
#include <memory>

/**
 * @brief Memory-safe MIDI control integration (simplified)
 * 
 * This is a simplified version that avoids memory allocation issues
 * by reducing singleton dependencies and complex observer patterns.
 */
class SafeMidiControlIntegration {
public:
    struct Statistics {
        size_t parameter_updates = 0;
        size_t control_changes_sent = 0;
        size_t clock_ticks_processed = 0;
    };

    static SafeMidiControlIntegration& getInstance() {
        static SafeMidiControlIntegration instance;
        return instance;
    }

    // Simple initialization without complex dependencies
    void initialize();
    
    // Basic control value change handling
    void handleControlValueChange(const Parameter* parameter, uint8_t value);
    
    // Clock events (simplified)
    void onClockTick() { stats_.clock_ticks_processed++; }
    void onClockStart() { /* simplified */ }
    void onClockStop() { /* simplified */ }
    
    // Statistics
    const Statistics& getStatistics() const { return stats_; }
    void resetStatistics() { stats_ = {}; }
    
    // Configuration
    void setMidiFeedbackEnabled(bool enabled) { midi_feedback_enabled_ = enabled; }
    bool isMidiFeedbackEnabled() const { return midi_feedback_enabled_; }

private:
    SafeMidiControlIntegration() = default;
    ~SafeMidiControlIntegration() = default;
    
    SafeMidiControlIntegration(const SafeMidiControlIntegration&) = delete;
    SafeMidiControlIntegration& operator=(const SafeMidiControlIntegration&) = delete;
    
    Statistics stats_;
    bool midi_feedback_enabled_ = false;
    bool initialized_ = false;
    
    // Simple MIDI output without complex dependencies
    void sendMidiOutput(const Parameter* parameter, uint8_t value);
};
