#pragma once

#ifdef MEMORY_LEAK_DETECTION

#include <chrono>
#include <iostream>

/**
 * @brief Simplified memory monitoring for initial implementation
 * 
 * This is a basic version that provides essential memory leak detection
 * without complex dependencies that might cause compilation issues.
 */
class SimpleMemoryMonitor {
public:
    static SimpleMemoryMonitor& getInstance() {
        static SimpleMemoryMonitor instance;
        return instance;
    }

    void startMonitoring() {
        if (monitoring_active_) return;
        
        monitoring_active_ = true;
        start_time_ = std::chrono::steady_clock::now();
        
        std::cout << "[SimpleMemoryMonitor] 🛡️ Basic memory monitoring started" << std::endl;
    }

    void stopMonitoring() {
        monitoring_active_ = false;
        std::cout << "[SimpleMemoryMonitor] Memory monitoring stopped" << std::endl;
    }

    void checkMemory(const std::string& checkpoint = "") {
        if (!monitoring_active_) return;
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
        
        std::cout << "[SimpleMemoryMonitor] Checkpoint (" << elapsed << "s): " << checkpoint << std::endl;
    }

    void recordAllocation(size_t bytes, const std::string& context = "") {
        total_allocated_ += bytes;
        if (!context.empty()) {
            std::cout << "[SimpleMemoryMonitor] Alloc: " << bytes << " bytes (" << context << ")" << std::endl;
        }
    }

    void recordDeallocation(size_t bytes, const std::string& context = "") {
        total_deallocated_ += bytes;
        if (!context.empty()) {
            std::cout << "[SimpleMemoryMonitor] Dealloc: " << bytes << " bytes (" << context << ")" << std::endl;
        }
    }

    void printSummary() {
        std::cout << "\n📊 SIMPLE MEMORY SUMMARY" << std::endl;
        std::cout << "=========================" << std::endl;
        std::cout << "Allocated:   " << (total_allocated_ / 1024) << " KB" << std::endl;
        std::cout << "Deallocated: " << (total_deallocated_ / 1024) << " KB" << std::endl;
        std::cout << "Net:         " << ((total_allocated_ - total_deallocated_) / 1024) << " KB" << std::endl;
        
        if (total_allocated_ > total_deallocated_) {
            size_t leak = total_allocated_ - total_deallocated_;
            if (leak > 1024) {
                std::cout << "⚠️ Potential leak: " << (leak / 1024) << " KB" << std::endl;
            } else {
                std::cout << "✅ Memory usage looks healthy" << std::endl;
            }
        } else {
            std::cout << "✅ No leaks detected" << std::endl;
        }
        std::cout << "=========================\n" << std::endl;
    }

private:
    SimpleMemoryMonitor() = default;
    
    bool monitoring_active_ = false;
    std::chrono::steady_clock::time_point start_time_;
    size_t total_allocated_ = 0;
    size_t total_deallocated_ = 0;
};

// Convenience macros
#define MEMORY_CHECKPOINT(name) SimpleMemoryMonitor::getInstance().checkMemory(name)
#define MEMORY_ALLOC(bytes, context) SimpleMemoryMonitor::getInstance().recordAllocation(bytes, context)
#define MEMORY_DEALLOC(bytes, context) SimpleMemoryMonitor::getInstance().recordDeallocation(bytes, context)

#else

// No-op macros when memory monitoring is disabled
#define MEMORY_CHECKPOINT(name)
#define MEMORY_ALLOC(bytes, context) 
#define MEMORY_DEALLOC(bytes, context)

class SimpleMemoryMonitor {
public:
    static SimpleMemoryMonitor& getInstance() {
        static SimpleMemoryMonitor instance;
        return instance;
    }
    void startMonitoring() {}
    void stopMonitoring() {}
    void printSummary() {}
};

#endif
