#pragma once

#include "components/parameter/Parameter.h"
#include "components/midi/MidiClockManager.h"
#include "components/midi/UnifiedMidiManager.h"
#include "components/parameter/MidiParameterBridge.h"
#include <memory>
#include <functional>

/**
 * @brief Enhanced MIDI integration for UI controls
 * 
 * This class provides improved MIDI integration for dials and buttons,
 * connecting them to the cleaned MIDI architecture with proper feedback
 * and synchronization.
 */
class MidiControlIntegration {
public:
    using ParameterChangedCallback = std::function<void(uint8_t value, const Parameter* param)>;
    
    static MidiControlIntegration& getInstance();
    
    // Initialization
    void initialize();
    
    // Control value change handling
    void handleControlValueChange(const Parameter* parameter, uint8_t value);
    
    // MIDI feedback configuration
    void enableMidiFeedback(bool enable) { midi_feedback_enabled_ = enable; }
    bool isMidiFeedbackEnabled() const { return midi_feedback_enabled_; }
    
    // Parameter change callback (for UI updates)
    void setParameterChangedCallback(ParameterChangedCallback callback) {
        parameter_changed_callback_ = callback;
    }
    
    // Clock integration for tempo-synced controls
    void onClockTick();
    void onClockStart();
    void onClockStop();
    
    // Statistics
    struct Statistics {
        uint64_t control_changes_sent = 0;
        uint64_t midi_feedback_sent = 0;
        uint64_t parameter_updates = 0;
        uint64_t clock_synced_updates = 0;
    };
    
    const Statistics& getStatistics() const { return stats_; }
    void resetStatistics() { stats_ = Statistics{}; }
    
private:
    MidiControlIntegration() = default;
    
    void sendMidiOutput(const Parameter* parameter, uint8_t value);
    void sendMidiFeedback(const Parameter* parameter, uint8_t value);
    
    bool midi_feedback_enabled_ = true;
    ParameterChangedCallback parameter_changed_callback_;
    Statistics stats_;
};
