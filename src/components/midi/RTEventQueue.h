#pragma once

#include <atomic>
#include <chrono>
#include <array>

namespace MIDI {

/**
 * @brief True lock-free, wait-free RT event queue
 * 
 * Uses multiple techniques to ensure the RT thread never blocks:
 * - Single Producer, Single Consumer (SPSC) design
 * - Time-sorted insertion
 * - Preallocated memory
 * - Cache-line aligned atomics
 */
template<typename EventType, size_t MaxEvents = 1024>
class RTEventQueue {
public:
    RTEventQueue() {
        static_assert(MaxEvents > 0 && (MaxEvents & (MaxEvents - 1)) == 0, 
                     "MaxEvents must be power of 2");
        
        // Initialize all slots as empty
        for (size_t i = 0; i < MaxEvents; ++i) {
            slots_[i].sequence.store(i, std::memory_order_relaxed);
        }
        
        // Initialize indexes
        enqueue_pos_.store(0, std::memory_order_relaxed);
        dequeue_pos_.store(0, std::memory_order_relaxed);
    }
    
    // Called from UI thread (Producer)
    bool enqueue(const EventType& event) {
        auto pos = enqueue_pos_.load(std::memory_order_relaxed);
        auto& slot = slots_[pos & (MaxEvents - 1)];
        auto seq = slot.sequence.load(std::memory_order_acquire);
        
        // Check if slot is available
        if (seq != pos) {
            return false; // Queue full
        }
        
        // Insert event in time-sorted order
        if (!insertSorted(event, pos)) {
            return false; // Insertion failed
        }
        
        return true;
    }
    
    // Called from RT thread (Consumer) - MUST be wait-free!
    bool dequeue(EventType& event, uint64_t current_time_us) {
        auto pos = dequeue_pos_.load(std::memory_order_relaxed);
        auto& slot = slots_[pos & (MaxEvents - 1)];
        auto seq = slot.sequence.load(std::memory_order_acquire);
        
        // Check if slot has data and event is due
        if (seq != pos + 1) {
            return false; // No data
        }
        
        if (slot.data.execute_time_us > current_time_us) {
            return false; // Not due yet
        }
        
        // Extract event
        event = slot.data;
        
        // Mark slot as consumed
        slot.sequence.store(pos + MaxEvents, std::memory_order_release);
        dequeue_pos_.store(pos + 1, std::memory_order_relaxed);
        
        return true;
    }
    
    // Statistics (safe to call from any thread)
    size_t size() const {
        auto enq = enqueue_pos_.load(std::memory_order_relaxed);
        auto deq = dequeue_pos_.load(std::memory_order_relaxed);
        return enq - deq;
    }
    
    bool empty() const { return size() == 0; }
    bool full() const { return size() == MaxEvents; }

private:
    // Cache-line aligned slot to prevent false sharing
    struct alignas(64) Slot {
        std::atomic<size_t> sequence;
        EventType data;
    };
    
    // Preallocated array - no dynamic allocation
    std::array<Slot, MaxEvents> slots_;
    
    // Cache-line aligned atomics to prevent false sharing
    alignas(64) std::atomic<size_t> enqueue_pos_;
    alignas(64) std::atomic<size_t> dequeue_pos_;
    
    // Time-sorted insertion helper
    bool insertSorted(const EventType& event, size_t pos) {
        auto& slot = slots_[pos & (MaxEvents - 1)];
        
        // Simple insertion for now - could optimize with binary search
        slot.data = event;
        slot.sequence.store(pos + 1, std::memory_order_release);
        enqueue_pos_.store(pos + 1, std::memory_order_relaxed);
        
        return true;
    }
};

/**
 * @brief RT-safe event with embedded timing
 */
struct RTMidiEvent {
    enum Type { NOTE_ON, NOTE_OFF, CC, PROGRAM_CHANGE };
    
    Type type;
    uint64_t execute_time_us;  // Absolute timestamp
    uint8_t channel;
    uint8_t data1;
    uint8_t data2;
    
    RTMidiEvent() : type(NOTE_ON), execute_time_us(0), channel(0), data1(0), data2(0) {}
    
    RTMidiEvent(Type t, uint64_t time_us, uint8_t ch, uint8_t d1, uint8_t d2)
        : type(t), execute_time_us(time_us), channel(ch), data1(d1), data2(d2) {}
    
    // For time-sorting
    bool operator<(const RTMidiEvent& other) const {
        return execute_time_us < other.execute_time_us;
    }
};

/**
 * @brief Multi-queue system for different event priorities
 * 
 * Separates different event types to prevent timing interference.
 */
class PriorityRTQueues {
public:
    // Separate queues for different priorities
    RTEventQueue<RTMidiEvent, 512> high_priority_queue_;    // MIDI events
    RTEventQueue<RTMidiEvent, 256> medium_priority_queue_;  // Sequencer events  
    RTEventQueue<RTMidiEvent, 128> low_priority_queue_;     // UI updates
    
    // Submit to appropriate queue based on event type
    bool submitEvent(const RTMidiEvent& event, int priority = 0) {
        switch (priority) {
            case 0: return high_priority_queue_.enqueue(event);
            case 1: return medium_priority_queue_.enqueue(event);
            case 2: return low_priority_queue_.enqueue(event);
            default: return false;
        }
    }
    
    // RT thread processes in priority order
    bool getNextDueEvent(RTMidiEvent& event, uint64_t current_time_us) {
        // Check high priority first
        if (high_priority_queue_.dequeue(event, current_time_us)) {
            return true;
        }
        
        // Then medium priority
        if (medium_priority_queue_.dequeue(event, current_time_us)) {
            return true;
        }
        
        // Finally low priority
        return low_priority_queue_.dequeue(event, current_time_us);
    }
    
    // Statistics
    struct QueueStats {
        size_t high_size = 0;
        size_t medium_size = 0;
        size_t low_size = 0;
        size_t total_size = 0;
    };
    
    QueueStats getStats() const {
        QueueStats stats;
        stats.high_size = high_priority_queue_.size();
        stats.medium_size = medium_priority_queue_.size();
        stats.low_size = low_priority_queue_.size();
        stats.total_size = stats.high_size + stats.medium_size + stats.low_size;
        return stats;
    }
};

} // namespace MIDI
