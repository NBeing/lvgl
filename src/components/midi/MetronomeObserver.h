#pragma once

#include "components/midi/MidiEvents.h"
#include <iostream>
#include <chrono>
#include <iomanip>

namespace MIDI {

/**
 * @brief Example observer that acts as a visual metronome
 * 
 * This demonstrates how to use the threaded observer pattern
 * to create synchronized components.
 */
class MetronomeObserver : public TypedObserver<ClockEvent> {
public:
    MetronomeObserver() = default;
    ~MetronomeObserver() = default;
    
    // TypedObserver interface
    void onEvent(const ClockEvent& event) override {
        switch (event.type) {
            case ClockEvent::START:
                std::cout << "[Metronome] ▶️  START - Beat tracking started" << std::endl;
                tick_count_ = 0;
                beat_count_ = 0;
                is_running_ = true;
                start_time_ = std::chrono::steady_clock::now();
                break;
                
            case ClockEvent::STOP:
                std::cout << "[Metronome] ⏹️  STOP - Beat tracking stopped" << std::endl;
                is_running_ = false;
                break;
                
            case ClockEvent::CONTINUE:
                std::cout << "[Metronome] ⏯️  CONTINUE - Beat tracking resumed" << std::endl;
                is_running_ = true;
                break;
                
            case ClockEvent::TICK:
                if (is_running_) {
                    handleClockTick(event.tick_count);
                }
                break;
        }
    }
    
    // Status
    bool isRunning() const { return is_running_; }
    int getCurrentBeat() const { return beat_count_; }
    int getCurrentTick() const { return tick_count_; }
    
private:
    void handleClockTick(int tick_count) {
        tick_count_ = tick_count;
        
        // MIDI clock: 24 ticks per quarter note (beat)
        int new_beat = tick_count / 24;
        
        if (new_beat != beat_count_) {
            beat_count_ = new_beat;
            
            // Visual beat indicator
            if (beat_count_ % 4 == 0) {
                // Downbeat (measure)
                std::cout << "[Metronome] 🔴 DOWNBEAT - Beat " << beat_count_ + 1 << std::endl;
            } else {
                // Regular beat
                std::cout << "[Metronome] 🟡 beat " << beat_count_ + 1 << std::endl;
            }
            
            // Calculate and display tempo
            displayTempo();
        }
    }
    
    void displayTempo() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_).count();
        
        if (elapsed > 0 && beat_count_ > 0) {
            float actual_bpm = (beat_count_ * 60000.0f) / elapsed;
            std::cout << "[Metronome]   Actual BPM: " << std::fixed << std::setprecision(1) << actual_bpm << std::endl;
        }
    }
    
    bool is_running_ = false;
    int tick_count_ = 0;
    int beat_count_ = 0;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace MIDI
