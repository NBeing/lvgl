#pragma once

#include "components/midi/StepSequencer.h"
#include "hardware/MidiHandler.h"
#include <memory>

namespace MIDI {

/**
 * @brief Converts sequencer events to MIDI output messages
 * 
 * This observer receives events from the StepSequencer and 
 * translates them into actual MIDI messages sent via MidiHandler.
 */
class SequencerMidiOutput : public TypedObserver<StepSequencer::SequencerEvent> {
public:
    explicit SequencerMidiOutput(std::shared_ptr<MidiHandler> midi_handler);
    ~SequencerMidiOutput() = default;
    
    // TypedObserver interface
    void onEvent(const StepSequencer::SequencerEvent& event) override;
    
    // Configuration
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    // Statistics
    int getNoteOnCount() const { return note_on_count_; }
    int getNoteOffCount() const { return note_off_count_; }
    void resetStats() { note_on_count_ = 0; note_off_count_ = 0; }

private:
    std::shared_ptr<MidiHandler> midi_handler_;
    bool enabled_ = true;
    
    // Statistics
    int note_on_count_ = 0;
    int note_off_count_ = 0;
    
    // MIDI message construction
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    void sendNoteOff(uint8_t channel, uint8_t note);
};

} // namespace MIDI
