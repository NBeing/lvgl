#pragma once

#include "components/midi/MidiEvents.h"
#include "components/controls/MidiControlIntegration.h"

/**
 * @brief Clock observer that bridges MIDI clock events to control integration
 * 
 * This observer connects the threaded MIDI clock system to the control
 * integration layer, enabling tempo-synced control updates and synchronization.
 */
class ControlClockObserver : public MIDI::TypedObserver<MIDI::ClockEvent> {
public:
    ControlClockObserver() = default;
    ~ControlClockObserver() = default;
    
    // TypedObserver<ClockEvent> implementation
    void onEvent(const MIDI::ClockEvent& event) override {
        auto& integration = MidiControlIntegration::getInstance();
        
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
    
private:
    int tick_count_ = 0;
};
