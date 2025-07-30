#pragma once

#include "RTEventScheduler.h"
#include "MemoryMappedEventBuffer.h"
#include <memory>

namespace MIDI {

/**
 * @brief Production-ready RT event system
 * 
 * Combines the best of all approaches:
 * - Memory mapped buffer for ultra-low latency
 * - Fallback to queue system for overflow
 * - Batch submission for efficiency
 * - Statistics and monitoring
 */
class ProductionRTEventSystem {
public:
    ProductionRTEventSystem() {
        memory_buffer_ = std::make_unique<MemoryMappedEventBuffer>();
        queue_scheduler_ = std::make_unique<RTEventScheduler>();
    }
    
    // UI thread interface - automatically chooses best submission method
    bool scheduleNoteOn(uint8_t channel, uint8_t note, uint8_t velocity,
                       std::chrono::steady_clock::time_point when) {
        auto time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            when.time_since_epoch()).count();
        
        // Create MIDI event data
        uint8_t midi_data[3] = {
            static_cast<uint8_t>(0x90 | (channel & 0x0F)),  // Note On
            note,
            velocity
        };
        
        // Try memory buffer first (fastest)
        if (memory_buffer_->submitEvent(time_us, midi_data, 3)) {
            stats_.memory_buffer_events++;
            return true;
        }
        
        // Fallback to queue system
        queue_scheduler_->scheduleNoteOn(channel, note, velocity, when);
        stats_.queue_events++;
        return true;
    }
    
    bool scheduleNoteOff(uint8_t channel, uint8_t note,
                        std::chrono::steady_clock::time_point when) {
        auto time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            when.time_since_epoch()).count();
        
        uint8_t midi_data[3] = {
            static_cast<uint8_t>(0x80 | (channel & 0x0F)),  // Note Off
            note,
            0
        };
        
        if (memory_buffer_->submitEvent(time_us, midi_data, 3)) {
            stats_.memory_buffer_events++;
            return true;
        }
        
        queue_scheduler_->scheduleNoteOff(channel, note, when);
        stats_.queue_events++;
        return true;
    }
    
    // RT thread interface - processes from both sources
    bool getNextDueEvent(uint64_t current_time_us, uint8_t* midi_data, size_t& size) {
        // Check memory buffer first (lowest latency)
        if (memory_buffer_->getNextDueEvent(current_time_us, midi_data, size)) {
            stats_.memory_buffer_processed++;
            return true;
        }
        
        // Check queue system
        RTMidiEvent queue_event;
        if (queue_scheduler_->getNextDueEvent(queue_event, current_time_us)) {
            // Convert to MIDI data
            convertToMidiData(queue_event, midi_data, size);
            stats_.queue_processed++;
            return true;
        }
        
        return false;
    }
    
    // Force flush all pending events
    void flush() {
        queue_scheduler_->flush();
    }
    
    // Comprehensive statistics
    struct SystemStats {
        // Event distribution
        size_t memory_buffer_events = 0;
        size_t queue_events = 0;
        size_t memory_buffer_processed = 0;
        size_t queue_processed = 0;
        
        // Performance metrics
        float memory_buffer_utilization = 0.0f;
        float queue_utilization = 0.0f;
        float total_throughput_events_per_sec = 0.0f;
        
        // Health indicators
        bool is_memory_buffer_healthy = true;
        bool is_queue_healthy = true;
        bool is_system_healthy = true;
    };
    
    SystemStats getStats() const {
        SystemStats sys_stats = stats_;
        
        // Calculate utilization
        auto buffer_stats = memory_buffer_->getStats(getCurrentTimeUs());
        sys_stats.memory_buffer_utilization = 
            static_cast<float>(buffer_stats.slots_used) / 
            (buffer_stats.slots_used + buffer_stats.slots_available);
        
        auto queue_stats = queue_scheduler_->getStats();
        sys_stats.queue_utilization = 
            static_cast<float>(queue_stats.queue_stats.total_size) / 1024.0f;
        
        // Health check
        sys_stats.is_memory_buffer_healthy = sys_stats.memory_buffer_utilization < 0.8f;
        sys_stats.is_queue_healthy = sys_stats.queue_utilization < 0.8f;
        sys_stats.is_system_healthy = sys_stats.is_memory_buffer_healthy && sys_stats.is_queue_healthy;
        
        return sys_stats;
    }
    
    void printPerformanceReport() const {
        auto stats = getStats();
        
        std::cout << "\n=== Production RT Event System Report ===" << std::endl;
        std::cout << "Memory Buffer Events: " << stats.memory_buffer_events << std::endl;
        std::cout << "Queue Events: " << stats.queue_events << std::endl;
        std::cout << "Memory Buffer Utilization: " << (stats.memory_buffer_utilization * 100) << "%" << std::endl;
        std::cout << "Queue Utilization: " << (stats.queue_utilization * 100) << "%" << std::endl;
        std::cout << "System Health: " << (stats.is_system_healthy ? "✅ Healthy" : "⚠️ Stressed") << std::endl;
        
        if (!stats.is_system_healthy) {
            std::cout << "Recommendations:" << std::endl;
            if (!stats.is_memory_buffer_healthy) {
                std::cout << "  - Consider increasing memory buffer size" << std::endl;
            }
            if (!stats.is_queue_healthy) {
                std::cout << "  - Consider reducing event generation rate" << std::endl;
            }
        }
    }

private:
    std::unique_ptr<MemoryMappedEventBuffer> memory_buffer_;
    std::unique_ptr<RTEventScheduler> queue_scheduler_;
    
    mutable SystemStats stats_;
    
    void convertToMidiData(const RTMidiEvent& event, uint8_t* midi_data, size_t& size) const {
        switch (event.type) {
            case RTMidiEvent::NOTE_ON:
                midi_data[0] = 0x90 | (event.channel & 0x0F);
                midi_data[1] = event.data1;
                midi_data[2] = event.data2;
                size = 3;
                break;
                
            case RTMidiEvent::NOTE_OFF:
                midi_data[0] = 0x80 | (event.channel & 0x0F);
                midi_data[1] = event.data1;
                midi_data[2] = 0;
                size = 3;
                break;
                
            case RTMidiEvent::CC:
                midi_data[0] = 0xB0 | (event.channel & 0x0F);
                midi_data[1] = event.data1;
                midi_data[2] = event.data2;
                size = 3;
                break;
                
            default:
                size = 0;
                break;
        }
    }
    
    uint64_t getCurrentTimeUs() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
    }
};

} // namespace MIDI
