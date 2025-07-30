#pragma once

#include "components/midi/MidiEvents.h"
#include "SafeMidiControlIntegration.h"

/**
 * @brief Memory-safe clock observer (simplified)
 * 
 * Simplified version that avoids complex memory allocations
 * and reduces potential for memory leaks.
 */
class SafeControlClockObserver : public MIDI::TypedObserver<MIDI::ClockEvent> {
public:
    SafeControlClockObserver() = default;
    ~SafeControlClockObserver() = default;
    
    // TypedObserver<ClockEvent> implementation (simplified)
    void onEvent(const MIDI::ClockEvent& event) override {
        // Get instance without storing references to avoid circular dependencies
        auto& integration = SafeMidiControlIntegration::getInstance();
        
        switch (event.type) {
            case MIDI::ClockEvent::TICK:
                integration.onClockTick();
                tick_count_++;
                break;
                
            case MIDI::ClockEvent::START:
                integration.onClockStart();
                tick_count_ = 0;
                break;
                
            case MIDI::ClockEvent::STOP:
                integration.onClockStop();
                break;
                
            case MIDI::ClockEvent::CONTINUE:
                integration.onClockStart(); // Same as start for controls
                break;
        }
    }
    
    int getTickCount() const { return tick_count_; }
    void resetTickCount() { tick_count_ = 0; }
    
private:
    int tick_count_ = 0;
};
