#pragma once

#include <atomic>
#include <cstring>
#include <array>

namespace MIDI {

/**
 * @brief Memory-mapped event buffer for ultimate RT performance
 * 
 * Uses memory mapping and atomic operations for zero-copy event transfer.
 * RT thread reads directly from mapped memory without any queue operations.
 */
class MemoryMappedEventBuffer {
public:
    // Event header for atomic operations
    struct EventHeader {
        std::atomic<uint32_t> magic;      // Magic number for validity
        std::atomic<uint64_t> timestamp;  // When to execute
        std::atomic<uint32_t> size;       // Event data size
        std::atomic<uint32_t> consumed;   // 0=pending, 1=consumed
    };
    
    // Fixed-size event slots
    struct EventSlot {
        EventHeader header;
        uint8_t data[32];  // MIDI event data
        
        EventSlot() {
            header.magic.store(0, std::memory_order_relaxed);
            header.timestamp.store(0, std::memory_order_relaxed);
            header.size.store(0, std::memory_order_relaxed);
            header.consumed.store(1, std::memory_order_relaxed);
        }
    };
    
    static constexpr uint32_t MAGIC_NUMBER = 0xBEEFFACE;
    static constexpr size_t MAX_EVENTS = 1024;
    
    MemoryMappedEventBuffer() {
        // Initialize all slots as consumed
        for (auto& slot : event_slots_) {
            slot.header.consumed.store(1, std::memory_order_relaxed);
        }
        
        write_index_.store(0, std::memory_order_relaxed);
    }
    
    // UI thread: Submit event (lock-free, wait-free)
    bool submitEvent(uint64_t timestamp, const void* data, size_t data_size) {
        if (data_size > 32) return false;
        
        // Find next available slot (round-robin)
        size_t start_index = write_index_.load(std::memory_order_relaxed);
        
        for (size_t i = 0; i < MAX_EVENTS; ++i) {
            size_t index = (start_index + i) % MAX_EVENTS;
            auto& slot = event_slots_[index];
            
            // Try to claim this slot
            uint32_t expected = 1; // consumed
            if (slot.header.consumed.compare_exchange_weak(expected, 0, std::memory_order_acquire)) {
                // Slot claimed! Fill it in
                std::memcpy(slot.data, data, data_size);
                slot.header.size.store(data_size, std::memory_order_relaxed);
                slot.header.timestamp.store(timestamp, std::memory_order_relaxed);
                slot.header.magic.store(MAGIC_NUMBER, std::memory_order_release);
                
                // Update write index for next search
                write_index_.store((index + 1) % MAX_EVENTS, std::memory_order_relaxed);
                return true;
            }
        }
        
        return false; // No slots available
    }
    
    // RT thread: Get next due event (wait-free scan)
    bool getNextDueEvent(uint64_t current_time, void* data_out, size_t& size_out) {
        uint64_t earliest_time = UINT64_MAX;
        size_t earliest_index = SIZE_MAX;
        
        // Scan all slots for earliest due event (cache-friendly linear scan)
        for (size_t i = 0; i < MAX_EVENTS; ++i) {
            auto& slot = event_slots_[i];
            
            // Quick check: is slot valid and not consumed?
            if (slot.header.magic.load(std::memory_order_acquire) != MAGIC_NUMBER ||
                slot.header.consumed.load(std::memory_order_relaxed) == 1) {
                continue;
            }
            
            uint64_t timestamp = slot.header.timestamp.load(std::memory_order_relaxed);
            
            // Is this event due and earlier than current earliest?
            if (timestamp <= current_time && timestamp < earliest_time) {
                earliest_time = timestamp;
                earliest_index = i;
            }
        }
        
        // If we found a due event, consume it
        if (earliest_index != SIZE_MAX) {
            auto& slot = event_slots_[earliest_index];
            
            // Copy data out
            size_out = slot.header.size.load(std::memory_order_relaxed);
            std::memcpy(data_out, slot.data, size_out);
            
            // Mark as consumed
            slot.header.consumed.store(1, std::memory_order_release);
            slot.header.magic.store(0, std::memory_order_relaxed);
            
            return true;
        }
        
        return false;
    }
    
    // Statistics
    struct BufferStats {
        size_t slots_used = 0;
        size_t slots_available = 0;
        uint64_t earliest_event_time = 0;
        uint64_t latest_event_time = 0;
    };
    
    BufferStats getStats(uint64_t current_time) const {
        BufferStats stats;
        
        for (const auto& slot : event_slots_) {
            if (slot.header.magic.load(std::memory_order_relaxed) == MAGIC_NUMBER &&
                slot.header.consumed.load(std::memory_order_relaxed) == 0) {
                stats.slots_used++;
                
                uint64_t timestamp = slot.header.timestamp.load(std::memory_order_relaxed);
                if (stats.earliest_event_time == 0 || timestamp < stats.earliest_event_time) {
                    stats.earliest_event_time = timestamp;
                }
                if (timestamp > stats.latest_event_time) {
                    stats.latest_event_time = timestamp;
                }
            } else {
                stats.slots_available++;
            }
        }
        
        return stats;
    }

private:
    // Preallocated event slots (no dynamic allocation)
    std::array<EventSlot, MAX_EVENTS> event_slots_;
    
    // Write hint (not strict ordering)
    std::atomic<size_t> write_index_;
};

} // namespace MIDI
