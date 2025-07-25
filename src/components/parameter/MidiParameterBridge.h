#pragma once

#include "ParameterManager.h"
#include "components/midi/RTClockObserver.h"
#include <cstdint>

namespace Parameters {

/**
 * @brief Converts MIDI CC messages to parameter change events
 * 
 * This component bridges the MIDI input system with the unified parameter system.
 * It receives MIDI CC messages from the RT thread and converts them into
 * parameter change events with proper mapping and value scaling.
 */
class MidiParameterBridge : public MIDI::RTClockObserver {
public:
    static MidiParameterBridge& getInstance();
    
    // Initialization
    void initialize();
    
    // MIDI input processing (called from RT thread)
    void processMidiCC(uint8_t channel, uint8_t cc, uint8_t value);
    void processMidiNRPN(uint8_t channel, uint16_t nrpn, uint16_t value);
    
    // RT Clock Observer implementation (inherited from RTClockObserver)
    void onRTClockTick(int tick) override {}
    void onRTClockStart() override {}
    void onRTClockStop() override {}
    void onRTClockContinue() override {}
    
    // MIDI feedback (send parameter changes back as MIDI CC)
    void sendMidiFeedback(const ParameterChangeEvent& event);
    
    // Statistics
    struct Statistics {
        std::atomic<uint64_t> midi_cc_received{0};
        std::atomic<uint64_t> midi_cc_mapped{0};
        std::atomic<uint64_t> midi_cc_unmapped{0};
        std::atomic<uint64_t> midi_feedback_sent{0};
    };
    
    const Statistics& getStatistics() const { return stats_; }
    
private:
    MidiParameterBridge() = default;
    
    // Value conversion
    float midiValueToNormalized(uint8_t midi_value);
    uint8_t normalizedToMidiValue(float normalized);
    
    // Statistics
    Statistics stats_;
    
    // Initialization state
    std::atomic<bool> initialized_{false};
};

} // namespace Parameters
