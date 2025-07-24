#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <algorithm>

#ifdef ESP32_BUILD
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
    #include <freertos/queue.h>
#else
    #include <thread>
    #include <queue>
    #include <mutex>
    #include <condition_variable>
#endif

namespace Threading {

/**
 * @brief Platform-abstracted thread handle
 */
class ThreadHandle {
public:
    virtual ~ThreadHandle() = default;
    virtual bool isRunning() const = 0;
    virtual void stop() = 0;
};

/**
 * @brief Platform-abstracted task creation
 */
class TaskManager {
public:
    enum class Priority {
        LOW = 1,
        NORMAL = 5,
        HIGH = 10,
        CRITICAL = 24
    };
    
    enum class Core {
        ANY = -1,
        CORE_0 = 0,
        CORE_1 = 1
    };
    
    using TaskFunction = std::function<void()>;
    
    static std::unique_ptr<ThreadHandle> createTask(
        const std::string& name,
        TaskFunction task_func,
        Priority priority = Priority::NORMAL,
        Core core = Core::ANY,
        size_t stack_size = 4096
    );
    
    static void sleep(uint32_t milliseconds);
    static void sleepMicroseconds(uint32_t microseconds);
};

/**
 * @brief Lock-free queue abstraction
 */
template<typename T, size_t Size = 256>
class LockFreeQueue {
private:
    #ifdef ESP32_BUILD
        QueueHandle_t queue_handle_;
    #else
        std::queue<T> queue_;
        mutable std::mutex mutex_;
        std::condition_variable condition_;
    #endif
    
public:
    LockFreeQueue() {
        #ifdef ESP32_BUILD
            queue_handle_ = xQueueCreate(Size, sizeof(T));
        #endif
    }
    
    ~LockFreeQueue() {
        #ifdef ESP32_BUILD
            if (queue_handle_) {
                vQueueDelete(queue_handle_);
            }
        #endif
    }
    
    bool push(const T& item) {
        #ifdef ESP32_BUILD
            return xQueueSend(queue_handle_, &item, 0) == pdTRUE;
        #else
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(item);
            condition_.notify_one();
            return true;
        #endif
    }
    
    bool pop(T& item) {
        #ifdef ESP32_BUILD
            return xQueueReceive(queue_handle_, &item, 0) == pdTRUE;
        #else
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty()) {
                return false;
            }
            item = queue_.front();
            queue_.pop();
            return true;
        #endif
    }
    
    bool empty() const {
        #ifdef ESP32_BUILD
            return uxQueueMessagesWaiting(queue_handle_) == 0;
        #else
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        #endif
    }
};

} // namespace Threading
