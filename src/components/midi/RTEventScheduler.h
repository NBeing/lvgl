#pragma once

#include "RTEventQueue.h"
#include <vector>
#include <chrono>

namespace MIDI {

/**
 * @brief Batch event submission to minimize RT thread interruption
 * 
 * UI thread collects events in batches, then submits atomically.
 * Reduces queue contention and RT thread cache misses.
 */
class BatchEventSubmitter {
public:
    BatchEventSubmitter() {
        // Pre-allocate batch storage
        pending_events_.reserve(64);
    }
    
    // UI thread: Collect events in batch (no RT thread interaction)
    void addToBatch(const RTMidiEvent& event) {
        pending_events_.push_back(event);
        
        // Auto-submit when batch is full or time threshold reached
        if (pending_events_.size() >= MAX_BATCH_SIZE || 
            shouldFlushBatch()) {
            flushBatch();
        }
    }
    
    // UI thread: Force submit current batch
    void flushBatch() {
        if (pending_events_.empty()) return;
        
        // Sort by timestamp before submitting
        std::sort(pending_events_.begin(), pending_events_.end());
        
        // Submit all events atomically
        size_t submitted = 0;
        for (const auto& event : pending_events_) {
            if (rt_queue_->submitEvent(event, getEventPriority(event))) {
                submitted++;
            } else {
                events_dropped_ += (pending_events_.size() - submitted);
                break;
            }
        }
        
        events_submitted_ += submitted;
        pending_events_.clear();
        last_flush_time_ = std::chrono::steady_clock::now();
        
        std::cout << "[BatchSubmitter] Flushed " << submitted << " events to RT queue" << std::endl;
    }
    
    // Setup
    void setRTQueue(std::shared_ptr<PriorityRTQueues> queue) { 
        rt_queue_ = queue; 
    }
    
    // Statistics
    struct BatchStats {
        size_t events_submitted = 0;
        size_t events_dropped = 0;
        size_t batches_submitted = 0;
        float average_batch_size = 0.0f;
    };
    
    const BatchStats getStats() const {
        BatchStats stats;
        stats.events_submitted = events_submitted_;
        stats.events_dropped = events_dropped_;
        stats.batches_submitted = batches_submitted_;
        if (batches_submitted_ > 0) {
            stats.average_batch_size = static_cast<float>(events_submitted_) / batches_submitted_;
        }
        return stats;
    }

private:
    static constexpr size_t MAX_BATCH_SIZE = 32;
    static constexpr auto MAX_BATCH_TIME = std::chrono::milliseconds(5);
    
    std::shared_ptr<PriorityRTQueues> rt_queue_;
    std::vector<RTMidiEvent> pending_events_;
    
    // Timing
    std::chrono::steady_clock::time_point last_flush_time_;
    
    // Statistics
    size_t events_submitted_ = 0;
    size_t events_dropped_ = 0;
    size_t batches_submitted_ = 0;
    
    bool shouldFlushBatch() const {
        auto now = std::chrono::steady_clock::now();
        return (now - last_flush_time_) >= MAX_BATCH_TIME;
    }
    
    int getEventPriority(const RTMidiEvent& event) const {
        switch (event.type) {
            case RTMidiEvent::NOTE_ON:
            case RTMidiEvent::NOTE_OFF:
                return 0; // High priority
            case RTMidiEvent::CC:
                return 1; // Medium priority
            case RTMidiEvent::PROGRAM_CHANGE:
                return 2; // Low priority
            default:
                return 1;
        }
    }
};

/**
 * @brief High-level event scheduling interface
 * 
 * Provides simple API that hides the complexity of RT queue management.
 */
class RTEventScheduler {
public:
    RTEventScheduler() {
        rt_queues_ = std::make_shared<PriorityRTQueues>();
        batch_submitter_.setRTQueue(rt_queues_);
    }
    
    // Simple scheduling interface (called from UI thread)
    void scheduleNoteOn(uint8_t channel, uint8_t note, uint8_t velocity,
                       std::chrono::steady_clock::time_point when) {
        auto time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            when.time_since_epoch()).count();
        
        RTMidiEvent event(RTMidiEvent::NOTE_ON, time_us, channel, note, velocity);
        batch_submitter_.addToBatch(event);
    }
    
    void scheduleNoteOff(uint8_t channel, uint8_t note,
                        std::chrono::steady_clock::time_point when) {
        auto time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            when.time_since_epoch()).count();
        
        RTMidiEvent event(RTMidiEvent::NOTE_OFF, time_us, channel, note, 0);
        batch_submitter_.addToBatch(event);
    }
    
    void scheduleCC(uint8_t channel, uint8_t cc, uint8_t value,
                   std::chrono::steady_clock::time_point when) {
        auto time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            when.time_since_epoch()).count();
        
        RTMidiEvent event(RTMidiEvent::CC, time_us, channel, cc, value);
        batch_submitter_.addToBatch(event);
    }
    
    // RT thread interface (called from RT thread)
    bool getNextDueEvent(RTMidiEvent& event, uint64_t current_time_us) {
        return rt_queues_->getNextDueEvent(event, current_time_us);
    }
    
    // Force flush (call before RT-critical sections)
    void flush() {
        batch_submitter_.flushBatch();
    }
    
    // Statistics
    struct SchedulerStats {
        PriorityRTQueues::QueueStats queue_stats;
        BatchEventSubmitter::BatchStats batch_stats;
    };
    
    SchedulerStats getStats() const {
        SchedulerStats stats;
        stats.queue_stats = rt_queues_->getStats();
        stats.batch_stats = batch_submitter_.getStats();
        return stats;
    }

private:
    std::shared_ptr<PriorityRTQueues> rt_queues_;
    BatchEventSubmitter batch_submitter_;
};

} // namespace MIDI
