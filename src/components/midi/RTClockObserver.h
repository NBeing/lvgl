#pragma once

/**
 * @file RTClockObserver.h
 * @brief Observer interface for real-time clock events
 * 
 * This interface allows objects to receive notifications for MIDI clock events
 * in a real-time safe manner. All methods are called from the RT thread and
 * must be lock-free and fast.
 */

#include <cstdint>

namespace MIDI {

/**
 * @brief Interface for observing real-time clock events
 * 
 * Classes implementing this interface can receive notifications about
 * MIDI clock start, stop, continue, and tick events. All callbacks
 * are executed in the real-time thread context.
 */
class RTClockObserver {
public:
    virtual ~RTClockObserver() = default;
    
    /**
     * @brief Called on each MIDI clock tick (24 ticks per quarter note)
     * @param tick Current tick number since start
     * 
     * This method is called from the RT thread and must be lock-free.
     */
    virtual void onRTClockTick(int tick) = 0;
    
    /**
     * @brief Called when MIDI clock starts
     * 
     * This method is called from the RT thread and must be lock-free.
     */
    virtual void onRTClockStart() = 0;
    
    /**
     * @brief Called when MIDI clock stops
     * 
     * This method is called from the RT thread and must be lock-free.
     */
    virtual void onRTClockStop() = 0;
    
    /**
     * @brief Called when MIDI clock continues from pause
     * 
     * This method is called from the RT thread and must be lock-free.
     */
    virtual void onRTClockContinue() = 0;
};

} // namespace MIDI
