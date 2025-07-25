#pragma once

#include "components/midi/StepSequencer.h"
#include "hardware/MidiHandler.h"
#include <queue>
#include <chrono>
#include <memory>

namespace MIDI {

/**
 * @brief Timestamped MIDI output with precise scheduling
 * 
 * Ensures MIDI messages are sent at exact timestamps, compensating
 * for UI thread timing jitter.
 */
class TimestampedMidiOutput : public TypedObserver<StepSequencer::SequencerEvent> {
public:
    explicit TimestampedMidiOutput(std::shared_ptr<MidiHandler> midi_handler);
    ~TimestampedMidiOutput() = default;
    
    // TypedObserver interface
    void onEvent(const StepSequencer::SequencerEvent& event) override;
    
    // Process timestamped events (call from main loop)
    void processScheduledEvents();
    
    // Configuration
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    // Statistics
    int getScheduledEventCount() const { return scheduled_events_.size(); }
    float getAverageLatency() const { return average_latency_us_; }
    float getMaxLatency() const { return max_latency_us_; }
    void resetStats();

private:
    struct ScheduledMidiEvent {
        enum Type { NOTE_ON, NOTE_OFF };
        
        Type type;
        uint8_t channel;
        uint8_t note;
        uint8_t velocity;
        std::chrono::steady_clock::time_point send_time;
        
        ScheduledMidiEvent(Type t, uint8_t c, uint8_t n, uint8_t v, 
                          std::chrono::steady_clock::time_point time)
            : type(t), channel(c), note(n), velocity(v), send_time(time) {}
        
        // For priority queue (earliest events first)
        bool operator>(const ScheduledMidiEvent& other) const {
            return send_time > other.send_time;
        }
    };
    
    std::shared_ptr<MidiHandler> midi_handler_;
    bool enabled_ = true;
    
    // Priority queue for scheduled events (earliest first)
    std::priority_queue<ScheduledMidiEvent, 
                       std::vector<ScheduledMidiEvent>, 
                       std::greater<ScheduledMidiEvent>> scheduled_events_;
    
    // Timing compensation
    std::chrono::nanoseconds ui_processing_delay_{0};
    std::chrono::steady_clock::time_point last_clock_tick_time_;
    
    // Statistics
    float average_latency_us_ = 0.0f;
    float max_latency_us_ = 0.0f;
    int processed_events_ = 0;
    
    // Event scheduling
    void scheduleNoteEvent(ScheduledMidiEvent::Type type, uint8_t channel, 
                          uint8_t note, uint8_t velocity, 
                          std::chrono::steady_clock::time_point exact_time);
    
    // Latency measurement
    void updateLatencyStats(std::chrono::nanoseconds latency);
    
    // Calculate precise event timing
    std::chrono::steady_clock::time_point calculateEventTime(
        const StepSequencer::SequencerEvent& event);
};

} // namespace MIDI
