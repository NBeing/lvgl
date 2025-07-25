#pragma once

#include "components/midi/TimestampedMidiOutput.h"
#include "components/threading/ThreadingAbstraction.h"
#include <atomic>
#include <memory>
#include <chrono>

namespace MIDI {

/**
 * @brief Dedicated high-frequency thread for precise MIDI output timing
 * 
 * Runs independently of UI thread to ensure MIDI events are sent at exact
 * timestamps regardless of UI processing delays (even 50ms+).
 */
class MidiTimingThread {
public:
    MidiTimingThread();
    ~MidiTimingThread();
    
    // Lifecycle
    bool start();
    void stop();
    bool isRunning() const { return running_.load(); }
    
    // Integration with timestamped output
    void setTimestampedOutput(std::shared_ptr<TimestampedMidiOutput> output) {
        timestamped_output_ = output;
    }
    
    // Configuration
    void setUpdateInterval(std::chrono::microseconds interval) {
        update_interval_ = interval;
    }
    
    // Statistics
    struct TimingStats {
        float average_sleep_accuracy_us = 0.0f;
        float max_sleep_error_us = 0.0f;
        int processed_cycles = 0;
        int missed_deadlines = 0;
        
        void reset() {
            average_sleep_accuracy_us = 0.0f;
            max_sleep_error_us = 0.0f;
            processed_cycles = 0;
            missed_deadlines = 0;
        }
    };
    
    const TimingStats& getStats() const { return stats_; }
    void resetStats() { stats_.reset(); }

private:
    // Thread management
    std::atomic<bool> running_{false};
    std::unique_ptr<Threading::ThreadHandle> timing_thread_;
    
    // MIDI output processing
    std::shared_ptr<TimestampedMidiOutput> timestamped_output_;
    
    // Timing configuration
    std::chrono::microseconds update_interval_{100}; // 100μs = 10kHz updates
    
    // Statistics
    TimingStats stats_;
    
    // Thread main loop
    void timingThreadLoop();
    
    // High-precision sleep
    void preciseSleep(std::chrono::steady_clock::time_point target_time);
    
    // Statistics tracking
    void updateTimingStats(std::chrono::nanoseconds sleep_error);
};

/**
 * @brief Queue-based communication between UI thread and timing thread
 * 
 * Allows UI thread to schedule MIDI events while timing thread
 * sends them at precise moments.
 */
class ThreadSafeMidiScheduler {
public:
    struct ScheduledEvent {
        enum Type { NOTE_ON, NOTE_OFF, CC, PROGRAM_CHANGE };
        
        Type type;
        uint8_t channel;
        uint8_t data1;
        uint8_t data2;
        std::chrono::steady_clock::time_point send_time;
        
        ScheduledEvent(Type t, uint8_t c, uint8_t d1, uint8_t d2, 
                      std::chrono::steady_clock::time_point time)
            : type(t), channel(c), data1(d1), data2(d2), send_time(time) {}
        
        bool operator>(const ScheduledEvent& other) const {
            return send_time > other.send_time;
        }
    };
    
    ThreadSafeMidiScheduler();
    ~ThreadSafeMidiScheduler() = default;
    
    // Called from UI thread (thread-safe)
    void scheduleEvent(const ScheduledEvent& event);
    void scheduleNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, 
                       std::chrono::steady_clock::time_point when);
    void scheduleNoteOff(uint8_t channel, uint8_t note, 
                        std::chrono::steady_clock::time_point when);
    
    // Called from timing thread (thread-safe)
    bool getNextDueEvent(ScheduledEvent& event);
    int getPendingEventCount() const;
    
    // Statistics
    int getTotalEventsScheduled() const { return total_scheduled_.load(); }
    int getTotalEventsSent() const { return total_sent_.load(); }

private:
    // Thread-safe priority queue using lock-free techniques
    Threading::LockFreeQueue<ScheduledEvent> event_queue_;
    
    // Statistics
    std::atomic<int> total_scheduled_{0};
    std::atomic<int> total_sent_{0};
    
    // Internal event management
    std::vector<ScheduledEvent> pending_events_;
    mutable std::mutex pending_mutex_;
    
    void sortPendingEvents();
};

} // namespace MIDI
