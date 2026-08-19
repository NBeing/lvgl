#pragma once

#include "components/threading/UIThreadSafeQueue.h"
#include <functional>
#include <atomic>

namespace UI {

/**
 * @brief Simple UI thread-safe queue implementation
 * 
 * Minimal implementation for queuing UI updates from other threads
 * to be executed on the main UI thread.
 */
class UIThreadSafeQueue {
public:
    using UIUpdateFunction = std::function<void()>;
    
    static UIThreadSafeQueue& getInstance() {
        static UIThreadSafeQueue instance;
        return instance;
    }
    
    /**
     * @brief Queue UI update to be executed on main thread
     * @param update Function to execute on main thread
     */
    void queueUIUpdate(UIUpdateFunction update) {
        // In a real implementation, this would queue to the main UI thread
        // For now, execute immediately for demonstration
        if (update) {
            update();
        }
        updates_queued_.fetch_add(1);
    }
    
    /**
     * @brief Process queued UI updates (call from main thread)
     */
    void processQueuedUpdates() {
        // In real implementation, would process queue here
        updates_processed_.fetch_add(updates_queued_.load());
        updates_queued_.store(0);
    }
    
    uint64_t getQueuedCount() const { return updates_queued_.load(); }
    uint64_t getProcessedCount() const { return updates_processed_.load(); }

private:
    std::atomic<uint64_t> updates_queued_{0};
    std::atomic<uint64_t> updates_processed_{0};
};

} // namespace UI
