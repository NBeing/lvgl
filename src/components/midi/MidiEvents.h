#pragma once

#include <chrono>
#include <cstdint>
#include <vector>
#include <functional>
#include <algorithm>
#include "components/threading/ThreadingAbstraction.h"

namespace MIDI {

/**
 * @brief Basic MIDI message structure
 */
struct MidiMessage {
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
    std::chrono::high_resolution_clock::time_point timestamp;
    
    MidiMessage() : status(0), data1(0), data2(0) {
        timestamp = std::chrono::high_resolution_clock::now();
    }
    
    MidiMessage(uint8_t s, uint8_t d1, uint8_t d2) 
        : status(s), data1(d1), data2(d2) {
        timestamp = std::chrono::high_resolution_clock::now();
    }
    
    bool isClock() const { return status == 0xF8; }
    bool isStart() const { return status == 0xFA; }
    bool isStop() const { return status == 0xFC; }
    bool isContinue() const { return status == 0xFB; }
    bool isNoteOn() const { return (status & 0xF0) == 0x90 && data2 > 0; }
    bool isNoteOff() const { return (status & 0xF0) == 0x80 || ((status & 0xF0) == 0x90 && data2 == 0); }
    bool isCC() const { return (status & 0xF0) == 0xB0; }
    
    static MidiMessage clock() { return MidiMessage(0xF8, 0, 0); }
    static MidiMessage start() { return MidiMessage(0xFA, 0, 0); }
    static MidiMessage stop() { return MidiMessage(0xFC, 0, 0); }
    static MidiMessage continue_() { return MidiMessage(0xFB, 0, 0); }
};

/**
 * @brief Clock-specific events
 */
struct ClockEvent {
    enum Type {
        TICK,
        START,
        STOP,
        CONTINUE
    };
    
    Type type;
    std::chrono::high_resolution_clock::time_point timestamp;
    uint32_t tick_count; // For TICK events
    
    // Default constructor for queue compatibility
    ClockEvent() : type(TICK), tick_count(0) {
        timestamp = std::chrono::high_resolution_clock::now();
    }
    
    ClockEvent(Type t) : type(t), tick_count(0) {
        timestamp = std::chrono::high_resolution_clock::now();
    }
    
    ClockEvent(Type t, uint32_t ticks) : type(t), tick_count(ticks) {
        timestamp = std::chrono::high_resolution_clock::now();
    }
};

/**
 * @brief Type-safe observer interface
 */
template<typename EventType>
class TypedObserver {
public:
    virtual ~TypedObserver() = default;
    virtual void onEvent(const EventType& event) = 0;
};

/**
 * @brief Thread-safe subject with event queuing
 */
template<typename EventType>
class ThreadSafeSubject {
private:
    std::vector<TypedObserver<EventType>*> observers_;
    Threading::LockFreeQueue<EventType> event_queue_;
    
public:
    void addObserver(TypedObserver<EventType>* observer) {
        // Note: In production, this should be thread-safe
        observers_.push_back(observer);
    }
    
    void removeObserver(TypedObserver<EventType>* observer) {
        // Note: In production, this should be thread-safe
        observers_.erase(
            std::remove(observers_.begin(), observers_.end(), observer),
            observers_.end()
        );
    }
    
    // Called from real-time thread (lock-free)
    void enqueueEvent(const EventType& event) {
        event_queue_.push(event);
    }
    
    // Called from UI thread
    void processQueuedEvents() {
        EventType event;
        while (event_queue_.pop(event)) {
            for (auto* observer : observers_) {
                observer->onEvent(event);
            }
        }
    }
};

} // namespace MIDI
