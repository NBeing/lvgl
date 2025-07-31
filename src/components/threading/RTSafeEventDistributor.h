#pragma once

#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <chrono>
#include "components/threading/LockFreeQueue.h"

namespace RTSafe {

/**
 * @brief Event types for RT-safe distribution
 */
enum class EventType : uint8_t {
    MIDI_INPUT = 0,
    MIDI_OUTPUT = 1,
    PARAMETER_CHANGE = 2,
    CLOCK_TICK = 3,
    CONTROL_CHANGE = 4
};

/**
 * @brief RT-safe event structure
 * 
 * Fixed-size event that can be passed lock-free between threads
 * No dynamic allocation - all data fits in fixed buffer
 */
struct RTEvent {
    EventType type;
    uint8_t channel;
    uint8_t data1;
    uint8_t data2;
    uint32_t timestamp_us;  // Microsecond timestamp
    uint32_t source_id;     // ID of event source
    
    RTEvent() : type(EventType::MIDI_INPUT), channel(0), data1(0), data2(0), 
                timestamp_us(0), source_id(0) {}
    
    RTEvent(EventType t, uint8_t ch, uint8_t d1, uint8_t d2, uint32_t src = 0) 
        : type(t), channel(ch), data1(d1), data2(d2), source_id(src) {
        // Get microsecond timestamp - RT-safe on most platforms
        auto now = std::chrono::high_resolution_clock::now();
        auto epoch = now.time_since_epoch();
        timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(epoch).count();
    }
    
    // Factory methods for common event types
    static RTEvent midiCC(uint8_t channel, uint8_t controller, uint8_t value) {
        return RTEvent(EventType::CONTROL_CHANGE, channel, controller, value);
    }
    
    static RTEvent parameterChange(uint8_t param_id, uint8_t value) {
        return RTEvent(EventType::PARAMETER_CHANGE, 0, param_id, value);
    }
    
    static RTEvent clockTick() {
        return RTEvent(EventType::CLOCK_TICK, 0, 0, 0);
    }
};

/**
 * @brief RT-safe observer interface
 * 
 * All methods MUST be RT-safe:
 * - No memory allocation
 * - No blocking operations  
 * - Bounded execution time
 * - No mutex locks
 */
class RTObserver {
public:
    virtual ~RTObserver() = default;
    
    /**
     * @brief Handle RT event - MUST BE RT-SAFE!
     * 
     * This is called from the RT thread and must:
     * - Complete in < 100μs (< 50μs on ESP32)
     * - Never allocate memory
     * - Never block or wait
     * - Never use mutexes
     */
    virtual void handleRTEvent(const RTEvent& event) = 0;
    
    /**
     * @brief Get observer priority (higher = processed first)
     */
    virtual int getPriority() const { return 0; }
};

/**
 * @brief UI observer interface
 * 
 * Called from UI thread - can take longer, allocate memory, etc.
 */
class UIObserver {
public:
    virtual ~UIObserver() = default;
    
    /**
     * @brief Handle UI event - NOT time-critical
     * 
     * This is called from the UI thread and can:
     * - Take unlimited time
     * - Allocate memory
     * - Update UI elements
     * - Use mutexes
     */
    virtual void handleUIEvent(const RTEvent& event) = 0;
};

/**
 * @brief RT-Safe Event Distributor
 * 
 * Core system for distributing events between RT and UI threads
 * without blocking or memory allocation in the RT path.
 */
class RTSafeEventDistributor {
private:
    // Observer lists - only modified during initialization
    std::vector<RTObserver*> rt_observers_;
    std::vector<UIObserver*> ui_observers_;
    
    // Lock-free queues for cross-thread communication
    static constexpr size_t QUEUE_SIZE = 1024;
    LockFreeQueue<RTEvent, QUEUE_SIZE> rt_to_ui_queue_;
    LockFreeQueue<RTEvent, QUEUE_SIZE> ui_to_rt_queue_;
    
    // Statistics (atomic for thread safety)
    std::atomic<uint64_t> rt_events_processed_{0};
    std::atomic<uint64_t> ui_events_processed_{0};
    std::atomic<uint64_t> rt_events_dropped_{0};
    std::atomic<uint64_t> ui_events_dropped_{0};
    
    // RT timing validation
    std::atomic<uint32_t> max_rt_processing_time_us_{0};
    std::atomic<uint32_t> last_rt_processing_time_us_{0};
    
    // Initialization state
    std::atomic<bool> initialized_{false};
    
public:
    RTSafeEventDistributor() = default;
    ~RTSafeEventDistributor() = default;
    
    // Non-copyable
    RTSafeEventDistributor(const RTSafeEventDistributor&) = delete;
    RTSafeEventDistributor& operator=(const RTSafeEventDistributor&) = delete;
    
    /**
     * @brief Initialize the distributor
     * 
     * Call this during application startup, NOT from RT thread
     */
    void initialize() {
        rt_observers_.clear();
        ui_observers_.clear();
        
        // Reset statistics
        rt_events_processed_ = 0;
        ui_events_processed_ = 0;
        rt_events_dropped_ = 0;
        ui_events_dropped_ = 0;
        max_rt_processing_time_us_ = 0;
        last_rt_processing_time_us_ = 0;
        
        initialized_ = true;
    }
    
    /**
     * @brief Shutdown the distributor
     */
    void shutdown() {
        initialized_ = false;
        rt_observers_.clear();
        ui_observers_.clear();
    }
    
    /**
     * @brief Add RT observer - NOT RT-SAFE
     * 
     * Call during initialization only
     */
    bool addRTObserver(RTObserver* observer) {
        if (!observer) return false;
        
        rt_observers_.push_back(observer);
        
        // Sort by priority (higher priority first)
        std::sort(rt_observers_.begin(), rt_observers_.end(),
                  [](RTObserver* a, RTObserver* b) {
                      return a->getPriority() > b->getPriority();
                  });
        
        return true;
    }
    
    /**
     * @brief Add UI observer - NOT RT-SAFE
     * 
     * Call during initialization only  
     */
    bool addUIObserver(UIObserver* observer) {
        if (!observer) return false;
        
        ui_observers_.push_back(observer);
        return true;
    }
    
    /**
     * @brief Notify RT observers - RT-SAFE
     * 
     * Call this from RT thread. Guarantees:
     * - No memory allocation
     * - No blocking
     * - Bounded execution time
     */
    void notifyRTObservers(const RTEvent& event) {
        if (!initialized_) return;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Notify RT observers immediately (direct call)
        for (auto observer : rt_observers_) {
            observer->handleRTEvent(event);
        }
        
        // Queue event for UI thread (lock-free)
        if (!rt_to_ui_queue_.enqueue(event)) {
            rt_events_dropped_++;
        }
        
        rt_events_processed_++;
        
        // Track RT timing (for debugging/validation)
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time);
        
        uint32_t duration_us = static_cast<uint32_t>(duration.count());
        last_rt_processing_time_us_ = duration_us;
        
        // Update max (atomic compare-and-swap)
        uint32_t current_max = max_rt_processing_time_us_.load();
        while (duration_us > current_max && 
               !max_rt_processing_time_us_.compare_exchange_weak(current_max, duration_us)) {
            // Retry if another thread updated max_rt_processing_time_us_
        }
    }
    
    /**
     * @brief Process UI events - NOT RT-SAFE
     * 
     * Call this from UI thread (e.g., in main loop)
     */
    void processUIEvents() {
        if (!initialized_) return;
        
        RTEvent event;
        int processed_count = 0;
        const int MAX_EVENTS_PER_CALL = 32; // Prevent UI thread starvation
        
        while (processed_count < MAX_EVENTS_PER_CALL && 
               rt_to_ui_queue_.dequeue(event)) {
            
            // Notify all UI observers
            for (auto observer : ui_observers_) {
                observer->handleUIEvent(event);
            }
            
            ui_events_processed_++;
            processed_count++;
        }
    }
    
    /**
     * @brief Send event from UI to RT thread - NOT RT-SAFE
     * 
     * Call this from UI thread to send events to RT thread
     */
    bool sendToRTThread(const RTEvent& event) {
        if (!initialized_) return false;
        
        return ui_to_rt_queue_.enqueue(event);
    }
    
    /**
     * @brief Process events from UI thread - RT-SAFE
     * 
     * Call this from RT thread to handle UI-originated events
     */
    void processRTEvents() {
        if (!initialized_) return;
        
        RTEvent event;
        int processed_count = 0;
        const int MAX_EVENTS_PER_CALL = 16; // Keep RT processing bounded
        
        while (processed_count < MAX_EVENTS_PER_CALL && 
               ui_to_rt_queue_.dequeue(event)) {
            
            notifyRTObservers(event);
            processed_count++;
        }
    }
    
    // Statistics and debugging
    struct Statistics {
        uint64_t rt_events_processed;
        uint64_t ui_events_processed;
        uint64_t rt_events_dropped;
        uint64_t ui_events_dropped;
        uint32_t max_rt_processing_time_us;
        uint32_t last_rt_processing_time_us;
        size_t rt_observer_count;
        size_t ui_observer_count;
        size_t rt_to_ui_queue_size;
        size_t ui_to_rt_queue_size;
    };
    
    Statistics getStatistics() const {
        return {
            rt_events_processed_.load(),
            ui_events_processed_.load(),
            rt_events_dropped_.load(),
            ui_events_dropped_.load(),
            max_rt_processing_time_us_.load(),
            last_rt_processing_time_us_.load(),
            rt_observers_.size(),
            ui_observers_.size(),
            rt_to_ui_queue_.size(),
            ui_to_rt_queue_.size()
        };
    }
    
    void resetStatistics() {
        rt_events_processed_ = 0;
        ui_events_processed_ = 0;
        rt_events_dropped_ = 0;
        ui_events_dropped_ = 0;
        max_rt_processing_time_us_ = 0;
        last_rt_processing_time_us_ = 0;
    }
    
    // RT-safety validation
    bool isRTSafe() const {
        auto stats = getStatistics();
        return stats.max_rt_processing_time_us < 100; // 100μs max
    }
};

} // namespace RTSafe
