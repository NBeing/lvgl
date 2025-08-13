#include "TimestampedMidiOutput.h"
#include <iostream>
#include <algorithm>

namespace MIDI {

TimestampedMidiOutput::TimestampedMidiOutput(std::shared_ptr<MidiHandler> midi_handler)
    : midi_handler_(midi_handler) {
    resetStats();
}

void TimestampedMidiOutput::onEvent(const StepSequencer::SequencerEvent& event) {
    if (!enabled_ || !midi_handler_) return;
    
    auto event_time = calculateEventTime(event);
    
    switch (event.type) {
        case StepSequencer::SequencerEvent::NOTE_ON:
            scheduleNoteEvent(ScheduledMidiEvent::NOTE_ON, event.channel, 
                             event.note, event.velocity, event_time);
            std::cout << "[Timestamped MIDI] 📅 Scheduled Note ON Ch:" << static_cast<int>(event.channel) 
                      << " Note:" << static_cast<int>(event.note) 
                      << " at +" << std::chrono::duration_cast<std::chrono::microseconds>(
                          event_time - std::chrono::steady_clock::now()).count() << "μs" << std::endl;
            break;
            
        case StepSequencer::SequencerEvent::NOTE_OFF:
            scheduleNoteEvent(ScheduledMidiEvent::NOTE_OFF, event.channel, 
                             event.note, 0, event_time);
            std::cout << "[Timestamped MIDI] 📅 Scheduled Note OFF Ch:" << static_cast<int>(event.channel) 
                      << " Note:" << static_cast<int>(event.note) 
                      << " at +" << std::chrono::duration_cast<std::chrono::microseconds>(
                          event_time - std::chrono::steady_clock::now()).count() << "μs" << std::endl;
            break;
            
        case StepSequencer::SequencerEvent::STEP_ADVANCE:
            // Update our clock tick reference for timing calculations
            last_clock_tick_time_ = std::chrono::steady_clock::now();
            break;
            
        case StepSequencer::SequencerEvent::PATTERN_COMPLETE:
            // Optional: Reset timing compensation
            break;
    }
}

void TimestampedMidiOutput::processScheduledEvents() {
    auto now = std::chrono::steady_clock::now();
    int events_processed = 0;
    
    // Process all events that are due
    while (!scheduled_events_.empty() && 
           scheduled_events_.top().send_time <= now) {
        
        const auto& event = scheduled_events_.top();
        
        // Calculate actual latency
        auto latency = now - event.send_time;
        updateLatencyStats(latency);
        
        // Send MIDI message
        switch (event.type) {
            case ScheduledMidiEvent::NOTE_ON:
                midi_handler_->sendNoteOn(event.channel, event.note, event.velocity);
                std::cout << "[Timestamped MIDI] 🎼 Note ON sent (latency: " 
                          << std::chrono::duration_cast<std::chrono::microseconds>(latency).count() 
                          << "μs)" << std::endl;
                break;
                
            case ScheduledMidiEvent::NOTE_OFF:
                midi_handler_->sendNoteOff(event.channel, event.note, 0);
                std::cout << "[Timestamped MIDI]   Note OFF sent (latency: " 
                          << std::chrono::duration_cast<std::chrono::microseconds>(latency).count() 
                          << "μs)" << std::endl;
                break;
        }
        
        scheduled_events_.pop();
        events_processed++;
        
        // Update timing for next iteration
        now = std::chrono::steady_clock::now();
    }
    
    if (events_processed > 0) {
        std::cout << "[Timestamped MIDI] ⚡ Processed " << events_processed 
                  << " scheduled events. Queued: " << scheduled_events_.size() << std::endl;
    }
}

void TimestampedMidiOutput::scheduleNoteEvent(ScheduledMidiEvent::Type type, 
                                             uint8_t channel, uint8_t note, 
                                             uint8_t velocity, 
                                             std::chrono::steady_clock::time_point exact_time) {
    scheduled_events_.emplace(type, channel, note, velocity, exact_time);
}

std::chrono::steady_clock::time_point TimestampedMidiOutput::calculateEventTime(
    const StepSequencer::SequencerEvent& event) {
    
    // Base time: when the clock tick was received
    auto base_time = last_clock_tick_time_;
    
    // Add compensation for UI processing delay
    auto compensated_time = base_time + ui_processing_delay_;
    
    // For note-offs, add the note length duration
    if (event.type == StepSequencer::SequencerEvent::NOTE_OFF) {
        // Assuming standard note length timing (this would come from sequencer)
        auto note_length = std::chrono::milliseconds(100); // Example: 100ms notes
        compensated_time += note_length;
    }
    
    return compensated_time;
}

void TimestampedMidiOutput::updateLatencyStats(std::chrono::nanoseconds latency) {
    float latency_us = std::chrono::duration_cast<std::chrono::microseconds>(latency).count();
    
    // Update running average
    if (processed_events_ == 0) {
        average_latency_us_ = latency_us;
    } else {
        average_latency_us_ = (average_latency_us_ * processed_events_ + latency_us) / (processed_events_ + 1);
    }
    
    // Update max latency
    max_latency_us_ = std::max(max_latency_us_, latency_us);
    
    processed_events_++;
}

void TimestampedMidiOutput::resetStats() {
    average_latency_us_ = 0.0f;
    max_latency_us_ = 0.0f;
    processed_events_ = 0;
}

} // namespace MIDI
