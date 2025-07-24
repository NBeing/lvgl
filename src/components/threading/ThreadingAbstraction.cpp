#include "ThreadingAbstraction.h"
#include <iostream>

#ifdef ESP32_BUILD
    #include <Arduino.h>
#else
    #include <chrono>
    #include <thread>
#endif

namespace Threading {

#ifdef ESP32_BUILD
/**
 * @brief ESP32 FreeRTOS thread handle implementation
 */
class ESP32ThreadHandle : public ThreadHandle {
private:
    TaskHandle_t task_handle_;
    std::atomic<bool> running_;
    
public:
    ESP32ThreadHandle(TaskHandle_t handle) 
        : task_handle_(handle), running_(true) {}
    
    ~ESP32ThreadHandle() {
        stop();
    }
    
    bool isRunning() const override {
        return running_.load() && task_handle_ != nullptr;
    }
    
    void stop() override {
        if (running_.load() && task_handle_) {
            running_.store(false);
            vTaskDelete(task_handle_);
            task_handle_ = nullptr;
        }
    }
    
    void setRunning(bool running) {
        running_.store(running);
    }
};

// ESP32 task wrapper
struct TaskData {
    TaskManager::TaskFunction function;
    ESP32ThreadHandle* handle;
};

static void esp32TaskWrapper(void* parameter) {
    TaskData* data = static_cast<TaskData*>(parameter);
    
    // Run the task function
    data->function();
    
    // Mark as not running
    data->handle->setRunning(false);
    
    // Clean up
    delete data;
    vTaskDelete(nullptr);
}

#else
/**
 * @brief Desktop std::thread handle implementation
 */
class DesktopThreadHandle : public ThreadHandle {
private:
    std::thread thread_;
    std::atomic<bool> running_;
    
public:
    DesktopThreadHandle(std::thread&& thread) 
        : thread_(std::move(thread)), running_(true) {}
    
    ~DesktopThreadHandle() {
        stop();
    }
    
    bool isRunning() const override {
        return running_.load() && thread_.joinable();
    }
    
    void stop() override {
        if (running_.load()) {
            running_.store(false);
            if (thread_.joinable()) {
                thread_.join();
            }
        }
    }
    
    void setRunning(bool running) {
        running_.store(running);
    }
};
#endif

std::unique_ptr<ThreadHandle> TaskManager::createTask(
    const std::string& name,
    TaskFunction task_func,
    Priority priority,
    Core core,
    size_t stack_size
) {
    #ifdef ESP32_BUILD
        TaskHandle_t task_handle;
        auto handle = std::make_unique<ESP32ThreadHandle>(nullptr);
        
        // Create task data
        TaskData* data = new TaskData{task_func, handle.get()};
        
        BaseType_t result;
        if (core == Core::ANY) {
            result = xTaskCreate(
                esp32TaskWrapper,
                name.c_str(),
                stack_size,
                data,
                static_cast<UBaseType_t>(priority),
                &task_handle
            );
        } else {
            result = xTaskCreatePinnedToCore(
                esp32TaskWrapper,
                name.c_str(),
                stack_size,
                data,
                static_cast<UBaseType_t>(priority),
                &task_handle,
                static_cast<BaseType_t>(core)
            );
        }
        
        if (result == pdPASS) {
            // Update the handle with the actual task handle
            handle = std::make_unique<ESP32ThreadHandle>(task_handle);
            return std::move(handle);
        } else {
            delete data;
            return nullptr;
        }
        
    #else
        // Desktop implementation
        auto handle = std::make_unique<DesktopThreadHandle>(
            std::thread([task_func, handle_ptr = std::weak_ptr<DesktopThreadHandle>{}]() {
                task_func();
            })
        );
        
        // Set thread priority (platform-specific)
        // Note: This is simplified - real implementation would use pthread_setschedparam
        
        return std::move(handle);
    #endif
}

void TaskManager::sleep(uint32_t milliseconds) {
    #ifdef ESP32_BUILD
        vTaskDelay(pdMS_TO_TICKS(milliseconds));
    #else
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    #endif
}

void TaskManager::sleepMicroseconds(uint32_t microseconds) {
    #ifdef ESP32_BUILD
        if (microseconds >= 1000) {
            vTaskDelay(pdMS_TO_TICKS(microseconds / 1000));
        } else {
            delayMicroseconds(microseconds);
        }
    #else
        std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
    #endif
}

} // namespace Threading
