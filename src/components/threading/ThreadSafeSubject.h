#pragma once

#include "LockFreeQueue.h"
#include <vector>
#include <mutex>
#include <algorithm>

/**
 * @brief Thread-safe subject for observer pattern
 * 
 * Safely queues events for observers and processes them on the UI thread.
 * Uses a combination of lock-free queues for RT → UI communication and
 * standard containers for UI thread processing.
 */
template<typename EventType>
class ThreadSafeSubject {
private:
    // Event queue for RT → UI thread communication
    LockFreeQueue<EventType, 2048> event_queue_;
    
    // Observer list (UI thread only) - using void* to be compatible with any observer type
    std::vector<void*> observers_;
    std::mutex observers_mutex_;
    
public:
    /**
     * @brief Add observer (call from UI thread)
     */
    template<typename ObserverType>
    void addObserver(ObserverType* observer) {
        if (!observer) return;
        
        std::lock_guard<std::mutex> lock(observers_mutex_);
        observers_.push_back(static_cast<void*>(observer));
    }
    
    /**
     * @brief Remove observer (call from UI thread) 
     */
    template<typename ObserverType>
    void removeObserver(ObserverType* observer) {
        std::lock_guard<std::mutex> lock(observers_mutex_);
        void* observer_ptr = static_cast<void*>(observer);
        observers_.erase(
            std::remove(observers_.begin(), observers_.end(), observer_ptr),
            observers_.end());
    }
    
    /**
     * @brief Enqueue event (RT-safe, call from any thread)
     */
    void enqueueEvent(const EventType& event) {
        if (!event_queue_.enqueue(event)) {
            // Queue full - could log error but avoid blocking
        }
    }
    
    /**
     * @brief Process queued events (call from UI thread only)
     */
    template<typename ObserverType>
    void processQueuedEvents() {
        EventType event;
        int processed = 0;
        
        // Process up to 100 events per call to prevent blocking UI
        while (processed < 100 && event_queue_.dequeue(event)) {
            notifyObservers<ObserverType>(event);
            processed++;
        }
    }
    
    /**
     * @brief Get current queue size
     */
    size_t getQueueSize() const {
        return event_queue_.size();
    }
    
private:
    template<typename ObserverType>
    void notifyObservers(const EventType& event) {
        std::lock_guard<std::mutex> lock(observers_mutex_);
        for (auto* observer_ptr : observers_) {
            if (observer_ptr) {
                // Cast back to proper observer type and call
                auto* observer = static_cast<ObserverType*>(observer_ptr);
                observer->onParameterChanged(event);
            }
        }
    }
};
