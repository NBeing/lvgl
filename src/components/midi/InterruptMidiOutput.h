#pragma once

#include "hardware/MidiHandler.h"
#include <chrono>
#include <queue>
#include <memory>

#ifdef ESP32_BUILD
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#endif

namespace MIDI {

/**
 * @brief Timer interrupt-based MIDI output (simpler than thread)
 * 
 * Uses hardware timer interrupts to send MIDI events at precise moments
 * without needing a dedicated thread. Lower complexity than MidiTimingThread.
 */
class InterruptMidiOutput {
public:
    struct ScheduledEvent {
        enum Type { NOTE_ON, NOTE_OFF, CC };
        Type type;
        uint8_t channel, data1, data2;
        uint64_t send_time_us;  // Absolute timestamp in microseconds
    };
    
    InterruptMidiOutput();
    ~InterruptMidiOutput();
    
    // Lifecycle (much simpler than thread management)
    bool initialize(std::shared_ptr<MidiHandler> midi_handler);
    void shutdown();
    
    // Event scheduling (same interface as before)
    void scheduleNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, 
                       std::chrono::steady_clock::time_point when);
    void scheduleNoteOff(uint8_t channel, uint8_t note, 
                        std::chrono::steady_clock::time_point when);
    
    // Statistics
    struct Stats {
        uint32_t events_scheduled = 0;
        uint32_t events_sent = 0;
        uint32_t timer_overruns = 0;
        float max_latency_us = 0.0f;
    };
    
    const Stats& getStats() const { return stats_; }

private:
    std::shared_ptr<MidiHandler> midi_handler_;
    bool initialized_ = false;
    
    // Event storage (lock-free, interrupt-safe)
    static constexpr size_t MAX_EVENTS = 64;
    ScheduledEvent events_[MAX_EVENTS];
    volatile size_t write_index_ = 0;
    volatile size_t read_index_ = 0;
    volatile size_t event_count_ = 0;
    
    Stats stats_;
    
#ifdef ESP32_BUILD
    esp_timer_handle_t timer_handle_;
    static constexpr uint64_t TIMER_INTERVAL_US = 100; // 100μs = 10kHz
    
    // Interrupt handler (static for C callback)
    static void IRAM_ATTR timerCallback(void* arg);
    void processEventsFromISR();
#else
    // Desktop: Use high-resolution timer
    std::chrono::steady_clock::time_point last_check_;
    void processEventsPoll(); // Called from main loop
#endif
    
    // Internal helpers
    bool addEventToQueue(const ScheduledEvent& event);
    bool getNextDueEvent(ScheduledEvent& event, uint64_t current_time_us);
    uint64_t getCurrentTimeUs() const;
};

} // namespace MIDI
