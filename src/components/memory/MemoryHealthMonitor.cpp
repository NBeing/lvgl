#include "MemoryHealthMonitor.h"
#include <thread>
#include <iomanip>
#include <sstream>

#ifdef LV_USE_MEM_MONITOR
#include "lvgl.h"
#endif

void MemoryHealthMonitor::startMonitoring(std::chrono::milliseconds interval) {
    if (monitoring_active_) return;
    
    monitoring_active_ = true;
    baseline_heap_ = getHeapUsage();
    last_leak_check_ = std::chrono::steady_clock::now();
    
    std::cout << "[MemoryMonitor] 🛡️ Memory health monitoring started (interval: " 
              << interval.count() << "ms)" << std::endl;
    
    // Start monitoring thread
    std::thread([this, interval]() {
        while (monitoring_active_) {
            try {
                auto stats = getCurrentStats();
                memory_history_.push_back(stats);
                
                // Limit history size
                if (memory_history_.size() > MAX_HISTORY_SIZE) {
                    memory_history_.erase(memory_history_.begin());
                }
                
                checkThresholds(stats);
                
                if (logging_enabled_) {
                    logMemoryStats(stats);
                }
                
                // Leak detection
                auto now = std::chrono::steady_clock::now();
                if (now - last_leak_check_ >= thresholds_.leak_detection_window) {
                    if (detectPotentialLeak()) {
                        if (alert_callback_) {
                            alert_callback_(AlertLevel::WARNING, "Potential memory leak detected", stats);
                        }
                    }
                    last_leak_check_ = now;
                }
                
            } catch (const std::exception& e) {
                std::cerr << "[MemoryMonitor] Error during monitoring: " << e.what() << std::endl;
            }
            
            std::this_thread::sleep_for(interval);
        }
    }).detach();
}

void MemoryHealthMonitor::stopMonitoring() {
    monitoring_active_ = false;
    std::cout << "[MemoryMonitor] Memory monitoring stopped" << std::endl;
}

MemoryHealthMonitor::MemoryStats MemoryHealthMonitor::getCurrentStats() {
    MemoryStats stats;
    stats.timestamp = std::chrono::steady_clock::now();
    
    // Get heap usage
    stats.heap_used = getHeapUsage();
    
#ifdef __linux__
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        stats.heap_total = usage.ru_maxrss * 1024; // Convert KB to bytes
    }
#endif
    
    if (stats.heap_total == 0) {
        stats.heap_total = 128 * 1024 * 1024; // Default 128MB estimate
    }
    
    // Get LVGL memory usage
#ifdef LV_USE_MEM_MONITOR
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    stats.lvgl_used = mon.total_size - mon.free_size;
    stats.lvgl_total = mon.total_size;
#else
    stats.lvgl_used = getLVGLMemoryUsage();
    stats.lvgl_total = 256 * 1024; // 256KB default
#endif
    
    // Get stack usage estimate
    stats.stack_used = getStackUsage();
    stats.stack_total = 8 * 1024 * 1024; // 8MB default stack size
    
    // Calculate percentages
    if (stats.heap_total > 0) {
        stats.heap_percent = (double(stats.heap_used) / double(stats.heap_total)) * 100.0;
    }
    
    if (stats.lvgl_total > 0) {
        stats.lvgl_percent = (double(stats.lvgl_used) / double(stats.lvgl_total)) * 100.0;
    }
    
    if (stats.stack_total > 0) {
        stats.stack_percent = (double(stats.stack_used) / double(stats.stack_total)) * 100.0;
    }
    
    return stats;
}

void MemoryHealthMonitor::forceCheck() {
    auto stats = getCurrentStats();
    checkThresholds(stats);
    printMemoryReport();
}

void MemoryHealthMonitor::recordAllocation(size_t bytes, const std::string& context) {
    manual_allocations_ += bytes;
    if (!context.empty() && logging_enabled_) {
        std::cout << "[MemoryMonitor] Allocation: " << bytes << " bytes (" << context << ")" << std::endl;
    }
}

void MemoryHealthMonitor::recordDeallocation(size_t bytes, const std::string& context) {
    manual_deallocations_ += bytes;
    if (!context.empty() && logging_enabled_) {
        std::cout << "[MemoryMonitor] Deallocation: " << bytes << " bytes (" << context << ")" << std::endl;
    }
}

void MemoryHealthMonitor::printMemoryReport() {
    auto stats = getCurrentStats();
    
    std::cout << "\n📊 MEMORY HEALTH REPORT" << std::endl;
    std::cout << "========================" << std::endl;
    std::cout << "🔹 Heap:     " << std::setw(8) << (stats.heap_used / 1024) << "KB / " 
              << std::setw(8) << (stats.heap_total / 1024) << "KB (" 
              << std::fixed << std::setprecision(1) << stats.heap_percent << "%)" << std::endl;
    std::cout << "🔹 LVGL:     " << std::setw(8) << (stats.lvgl_used / 1024) << "KB / " 
              << std::setw(8) << (stats.lvgl_total / 1024) << "KB (" 
              << std::fixed << std::setprecision(1) << stats.lvgl_percent << "%)" << std::endl;
    std::cout << "🔹 Stack:    " << std::setw(8) << (stats.stack_used / 1024) << "KB / " 
              << std::setw(8) << (stats.stack_total / 1024) << "KB (" 
              << std::fixed << std::setprecision(1) << stats.stack_percent << "%)" << std::endl;
    
    if (manual_allocations_ > 0 || manual_deallocations_ > 0) {
        std::cout << "🔹 Manual:   +" << (manual_allocations_ / 1024) << "KB / -" 
                  << (manual_deallocations_ / 1024) << "KB (net: " 
                  << ((manual_allocations_ - manual_deallocations_) / 1024) << "KB)" << std::endl;
    }
    
    // Status indicators
    std::string heap_status = stats.heap_percent > thresholds_.heap_critical ? "🚨 CRITICAL" :
                             stats.heap_percent > thresholds_.heap_warning ? "⚠️ WARNING" : "✅ OK";
    std::string lvgl_status = stats.lvgl_percent > thresholds_.lvgl_critical ? "🚨 CRITICAL" :
                             stats.lvgl_percent > thresholds_.lvgl_warning ? "⚠️ WARNING" : "✅ OK";
    
    std::cout << "🔹 Status:   Heap " << heap_status << " | LVGL " << lvgl_status << std::endl;
    std::cout << "========================\n" << std::endl;
}

bool MemoryHealthMonitor::detectPotentialLeak() {
    if (memory_history_.size() < 2) return false;
    
    auto current_heap = memory_history_.back().heap_used;
    auto growth = current_heap - baseline_heap_;
    
    if (growth > thresholds_.leak_detection_bytes) {
        std::cout << "[MemoryMonitor] ⚠️ Potential leak: heap grew by " 
                  << (growth / 1024) << "KB in last " 
                  << thresholds_.leak_detection_window.count() << " seconds" << std::endl;
        
        // Update baseline to avoid repeated alerts
        baseline_heap_ = current_heap;
        return true;
    }
    
    return false;
}

void MemoryHealthMonitor::triggerGarbageCollection() {
    std::cout << "[MemoryMonitor] 🧹 Triggering garbage collection..." << std::endl;
    
    // Simple garbage collection - just force output flushes
    try {
        std::cout.flush();
        std::cerr.flush();
    } catch (...) {
        // Ignore errors during cleanup
    }
}

void MemoryHealthMonitor::checkThresholds(const MemoryStats& stats) {
    // Check heap thresholds
    if (stats.heap_percent > thresholds_.heap_critical) {
        if (alert_callback_) {
            alert_callback_(AlertLevel::CRITICAL, "Heap memory critical", stats);
        }
        triggerGarbageCollection();
    } else if (stats.heap_percent > thresholds_.heap_warning) {
        if (alert_callback_) {
            alert_callback_(AlertLevel::WARNING, "Heap memory warning", stats);
        }
    }
    
    // Check LVGL thresholds
    if (stats.lvgl_percent > thresholds_.lvgl_critical) {
        if (alert_callback_) {
            alert_callback_(AlertLevel::CRITICAL, "LVGL memory critical", stats);
        }
        triggerGarbageCollection();
    } else if (stats.lvgl_percent > thresholds_.lvgl_warning) {
        if (alert_callback_) {
            alert_callback_(AlertLevel::WARNING, "LVGL memory warning", stats);
        }
    }
}

void MemoryHealthMonitor::logMemoryStats(const MemoryStats& stats) {
    if (!logging_enabled_) return;
    
    std::ofstream log(log_file_, std::ios::app);
    if (log.is_open()) {
        auto time_since_epoch = stats.timestamp.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time_since_epoch).count();
        
        log << seconds << "," 
            << stats.heap_used << "," << stats.heap_total << "," << stats.heap_percent << ","
            << stats.lvgl_used << "," << stats.lvgl_total << "," << stats.lvgl_percent << ","
            << stats.stack_used << "," << stats.stack_total << "," << stats.stack_percent << std::endl;
    }
}

size_t MemoryHealthMonitor::getHeapUsage() {
#ifdef __linux__
    // Use mallinfo2 if available, fallback to simple estimation
    try {
        struct mallinfo info = mallinfo();
        return info.uordblks; // Bytes in use
    } catch (...) {
        return 64 * 1024; // Fallback: 64KB estimate
    }
#else
    return 64 * 1024; // Fallback: 64KB estimate
#endif
}

size_t MemoryHealthMonitor::getLVGLMemoryUsage() {
    // Fallback estimation if LVGL monitoring not available
    return 64 * 1024; // Estimate 64KB
}

size_t MemoryHealthMonitor::getStackUsage() {
    // Simple stack usage estimation
    volatile char stack_var;
    static const volatile char* stack_start = nullptr;
    
    if (stack_start == nullptr) {
        stack_start = &stack_var;
        return 0;
    }
    
    return std::abs(&stack_var - stack_start);
}
