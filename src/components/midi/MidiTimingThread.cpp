#include "MidiTimingThread.h"
#include <iostream>
#include <algorithm>

namespace MIDI {

MidiTimingThread::MidiTimingThread() {
    resetStats();
}

MidiTimingThread::~MidiTimingThread() {
    stop();
}

bool MidiTimingThread::start() {
#ifdef ESP32_BUILD
    // Temporarily disable RT timing thread on ESP32 to prevent watchdog issues
    std::cout << "[MidiTimingThread] ⚠️ RT timing thread disabled on ESP32 for stability" << std::endl;
    std::cout << "[MidiTimingThread] Using standard timing instead of high-precision RT" << std::endl;
    running_.store(true);
    return true;
#else
    if (running_.load()) return true;
    
    running_.store(true);
    
    std::cout << "[MidiTimingThread] Starting high-precision MIDI timing thread..." << std::endl;
    
    timing_thread_ = Threading::TaskManager::createTask(
        "MidiTiming",
        [this]() { timingThreadLoop(); },
        Threading::TaskManager::Priority::HIGH,       // High priority (was CRITICAL)
        Threading::TaskManager::Core::CORE_0,        // Pin to core 0
        4096  // Small stack - this thread does minimal work
    );
    
    if (!timing_thread_) {
        running_.store(false);
        std::cerr << "[MidiTimingThread] Failed to create timing thread!" << std::endl;
        return false;
    }
    
    std::cout << "[MidiTimingThread] ⚡ High-precision timing thread started (10kHz updates)" << std::endl;
    return true;
#endif
}

void MidiTimingThread::stop() {
    if (!running_.load()) return;
    
    std::cout << "[MidiTimingThread] Stopping timing thread..." << std::endl;
    running_.store(false);
    
    if (timing_thread_) {
        // Note: ThreadingAbstraction doesn't expose join, thread will cleanup automatically
        timing_thread_.reset();
    }
    
    std::cout << "[MidiTimingThread] Timing thread stopped" << std::endl;
}

void MidiTimingThread::timingThreadLoop() {
    std::cout << "[MidiTimingThread] ⚡ Timing thread main loop started" << std::endl;
    
#ifdef ESP32_BUILD
    // Note: Core affinity is set by TaskManager::createTask with Core::CORE_0
    std::cout << "[MidiTimingThread] Thread created for Core 0 (RT core)" << std::endl;
#endif
    
    auto next_update = std::chrono::steady_clock::now();
    
    while (running_.load()) {
        auto loop_start = std::chrono::steady_clock::now();
        
        // Process timestamped MIDI events
        if (timestamped_output_) {
            timestamped_output_->processScheduledEvents();
        }
        
        // Calculate next update time
        next_update += update_interval_;
        
        // High-precision sleep until next update
        preciseSleep(next_update);
        
        // Track timing accuracy
        auto actual_wake = std::chrono::steady_clock::now();
        auto sleep_error = actual_wake - next_update;
        updateTimingStats(sleep_error);
        
        // Adjust next_update if we're falling behind
        if (sleep_error > std::chrono::milliseconds(1)) {
            next_update = actual_wake;
            stats_.missed_deadlines++;
        }
        
#ifdef ESP32_BUILD
        // Periodically yield to prevent watchdog timeout
        static int watchdog_counter = 0;
        if (++watchdog_counter >= 100) { // Every ~100 iterations
            taskYIELD();
            watchdog_counter = 0;
        }
#endif
    }
    
    std::cout << "[MidiTimingThread] Timing thread loop exited" << std::endl;
}

void MidiTimingThread::preciseSleep(std::chrono::steady_clock::time_point target_time) {
    // CONSERVATIVE ESP32 VERSION - prioritizes system stability over precision
    auto now = std::chrono::steady_clock::now();
    auto sleep_duration = target_time - now;
    
    if (sleep_duration <= std::chrono::microseconds(0)) {
        return; // Already past target time
    }

#ifdef ESP32_BUILD
    // ESP32: Sacrifice precision for stability to prevent watchdog issues
    auto sleep_us = std::chrono::duration_cast<std::chrono::microseconds>(sleep_duration).count();
    
    // Always use FreeRTOS delay - no busy waiting!
    if (sleep_us > 1000) { // > 1ms
        vTaskDelay(pdMS_TO_TICKS(sleep_us / 1000));
    } else {
        // For sub-millisecond sleeps, just yield and accept timing error
        vTaskDelay(1); // Minimum delay
    }
    
    // No busy-wait loops - accept reduced precision for stability
    
#else
    // Desktop: Use std::this_thread::sleep_until with busy-wait refinement
    if (sleep_duration > std::chrono::milliseconds(1)) {
        // Sleep most of the time
        std::this_thread::sleep_until(target_time - std::chrono::microseconds(500));
    }
    
    // Busy-wait for final precision
    while (std::chrono::steady_clock::now() < target_time) {
        std::this_thread::yield();
    }
#endif
}void MidiTimingThread::updateTimingStats(std::chrono::nanoseconds sleep_error) {
    // Convert nanoseconds to microseconds
    float error_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::nanoseconds(std::abs(sleep_error.count()))).count();
    
    // Update running average
    if (stats_.processed_cycles == 0) {
        stats_.average_sleep_accuracy_us = error_us;
    } else {
        stats_.average_sleep_accuracy_us = 
            (stats_.average_sleep_accuracy_us * stats_.processed_cycles + error_us) / 
            (stats_.processed_cycles + 1);
    }
    
    // Update max error
    stats_.max_sleep_error_us = std::max(stats_.max_sleep_error_us, error_us);
    
    stats_.processed_cycles++;
}

// ThreadSafeMidiScheduler Implementation
ThreadSafeMidiScheduler::ThreadSafeMidiScheduler() {
    pending_events_.reserve(1000); // Pre-allocate for performance
}

void ThreadSafeMidiScheduler::scheduleEvent(const ScheduledEvent& event) {
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_events_.push_back(event);
        
        // Keep events sorted by time (most recent scheduling)
        if (pending_events_.size() > 1) {
            sortPendingEvents();
        }
    }
    
    total_scheduled_.fetch_add(1);
}

void ThreadSafeMidiScheduler::scheduleNoteOn(uint8_t channel, uint8_t note, 
                                            uint8_t velocity, 
                                            std::chrono::steady_clock::time_point when) {
    scheduleEvent(ScheduledEvent(ScheduledEvent::NOTE_ON, channel, note, velocity, when));
}

void ThreadSafeMidiScheduler::scheduleNoteOff(uint8_t channel, uint8_t note, 
                                             std::chrono::steady_clock::time_point when) {
    scheduleEvent(ScheduledEvent(ScheduledEvent::NOTE_OFF, channel, note, 0, when));
}

bool ThreadSafeMidiScheduler::getNextDueEvent(ScheduledEvent& event) {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    
    if (pending_events_.empty()) return false;
    
    auto now = std::chrono::steady_clock::now();
    
    // Check if the earliest event is due
    if (pending_events_[0].send_time <= now) {
        event = pending_events_[0];
        pending_events_.erase(pending_events_.begin());
        total_sent_.fetch_add(1);
        return true;
    }
    
    return false;
}

int ThreadSafeMidiScheduler::getPendingEventCount() const {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    return pending_events_.size();
}

void ThreadSafeMidiScheduler::sortPendingEvents() {
    std::sort(pending_events_.begin(), pending_events_.end(),
              [](const ScheduledEvent& a, const ScheduledEvent& b) {
                  return a.send_time < b.send_time;
              });
}

} // namespace MIDI
