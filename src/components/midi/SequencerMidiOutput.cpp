#include "SequencerMidiOutput.h"
#include <iostream>

namespace MIDI {

SequencerMidiOutput::SequencerMidiOutput(std::shared_ptr<MidiHandler> midi_handler)
    : midi_handler_(midi_handler) {
}

void SequencerMidiOutput::onEvent(const StepSequencer::SequencerEvent& event) {
    if (!enabled_ || !midi_handler_) return;
    
    switch (event.type) {
        case StepSequencer::SequencerEvent::NOTE_ON:
            sendNoteOn(event.channel, event.note, event.velocity);
            note_on_count_++;
            std::cout << "[MIDI Out] 🎼 Note ON  Ch:" << static_cast<int>(event.channel) 
                      << " Note:" << static_cast<int>(event.note) 
                      << " Vel:" << static_cast<int>(event.velocity) << std::endl;
            break;
            
        case StepSequencer::SequencerEvent::NOTE_OFF:
            sendNoteOff(event.channel, event.note);
            note_off_count_++;
            std::cout << "[MIDI Out]   Note OFF Ch:" << static_cast<int>(event.channel) 
                      << " Note:" << static_cast<int>(event.note) << std::endl;
            break;
            
        case StepSequencer::SequencerEvent::STEP_ADVANCE:
            // Could trigger LED updates, visual feedback, etc.
            break;
            
        case StepSequencer::SequencerEvent::PATTERN_COMPLETE:
            std::cout << "[MIDI Out] 🔄 Pattern completed, looping..." << std::endl;
            break;
    }
}

void SequencerMidiOutput::sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (!midi_handler_) return;
    
    // Use MidiHandler's direct note on method
    midi_handler_->sendNoteOn(channel, note, velocity);
}

void SequencerMidiOutput::sendNoteOff(uint8_t channel, uint8_t note) {
    if (!midi_handler_) return;
    
    // Use MidiHandler's direct note off method
    midi_handler_->sendNoteOff(channel, note, 0);
}

} // namespace MIDI
