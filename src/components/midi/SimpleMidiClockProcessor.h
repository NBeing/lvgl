#pragma once

#include "MidiEvents.h"
#include "components/threading/ThreadingAbstraction.h"
#include <atomic>
#include <memory>
#include <iostream>

namespace MIDI {

/**
 * @brief Minimal MIDI clock processor for initial implementation
 */
class SimpleMidiClockProcessor {
private:
    std::atomic<bool> running_{false};
    std::unique_ptr<Threading::ThreadHandle> midi_thread_;
    
    // Clock state
    std::atomic<bool> clock_running_{false};
    std::atomic<uint32_t> tick_count_{0};
    std::atomic<float> bpm_{120.0f};
    
    // Event subjects
    ThreadSafeSubject<ClockEvent> clock_subject_;
    
    // Simple internal clock generation
    std::atomic<bool> internal_clock_enabled_{true};
    
public:
    SimpleMidiClockProcessor() = default;
    ~SimpleMidiClockProcessor() { stop(); }
    
    // Start/stop
    bool start() {
        if (running_.load()) return true;
        
        running_.store(true);
        
        midi_thread_ = Threading::TaskManager::createTask(
            "MIDI_Clock",
            [this]() { clockLoop(); },
            Threading::TaskManager::Priority::CRITICAL,
            Threading::TaskManager::Core::CORE_0,
            4096
        );
        
        return midi_thread_ != nullptr;
    }
    
    void stop() {
        running_.store(false);
        if (midi_thread_) {
            midi_thread_->stop();
            midi_thread_.reset();
        }
    }
    
    // Clock control
    void startClock() {
        clock_running_.store(true);
        tick_count_.store(0);
        clock_subject_.enqueueEvent(ClockEvent(ClockEvent::START));
    }
    
    void stopClock() {
        clock_running_.store(false);
        clock_subject_.enqueueEvent(ClockEvent(ClockEvent::STOP));
    }
    
    void continueClock() {
        clock_running_.store(true);
        clock_subject_.enqueueEvent(ClockEvent(ClockEvent::CONTINUE));
    }
    
    // Settings
    void setBPM(float bpm) {
        bpm_.store(bpm);
    }
    
    float getBPM() const {
        return bpm_.load();
    }
    
    bool isClockRunning() const {
        return clock_running_.load();
    }
    
    uint32_t getTickCount() const {
        return tick_count_.load();
    }
    
    // Observer management
    void addClockObserver(TypedObserver<ClockEvent>* observer) {
        clock_subject_.addObserver(observer);
    }
    
    void removeClockObserver(TypedObserver<ClockEvent>* observer) {
        clock_subject_.removeObserver(observer);
    }
    
    // Called from UI thread to process events
    void processEvents() {
        clock_subject_.processQueuedEvents();
    }
    
    // Process incoming MIDI (for future external clock sync)
    void processMidiMessage(const MidiMessage& msg) {
        if (!internal_clock_enabled_.load()) {
            if (msg.isClock()) {
                tick_count_.fetch_add(1);
                clock_subject_.enqueueEvent(ClockEvent(ClockEvent::TICK, tick_count_.load()));
            } else if (msg.isStart()) {
                startClock();
            } else if (msg.isStop()) {
                stopClock();
            } else if (msg.isContinue()) {
                continueClock();
            }
        }
    }
    
private:
    void clockLoop() {
        std::cout << "[MIDI Clock] Started real-time clock thread" << std::endl;
        
        while (running_.load()) {
            if (clock_running_.load() && internal_clock_enabled_.load()) {
                generateClockTick();
            }
            
            // Calculate sleep time based on BPM
            // MIDI clock: 24 ticks per quarter note
            float current_bpm = bpm_.load();
            uint32_t tick_interval_us = (60.0f * 1000000.0f) / (current_bpm * 24.0f);
            
            Threading::TaskManager::sleepMicroseconds(tick_interval_us);
        }
        
        std::cout << "[MIDI Clock] Stopped real-time clock thread" << std::endl;
    }
    
    void generateClockTick() {
        uint32_t current_tick = tick_count_.fetch_add(1);
        clock_subject_.enqueueEvent(ClockEvent(ClockEvent::TICK, current_tick));
    }
};

} // namespace MIDI
