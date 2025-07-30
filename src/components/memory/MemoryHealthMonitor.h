#pragma once

#include <chrono>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include <fstream>

#ifdef __linux__
#include <malloc.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

/**
 * @brief Comprehensive memory health monitoring system
 * 
 * Tracks heap, LVGL memory, stack usage, and detects potential leaks
 * with configurable alerting and automatic reporting.
 */
class MemoryHealthMonitor {
public:
    struct MemoryStats {
        size_t heap_used = 0;
        size_t heap_total = 0;
        size_t lvgl_used = 0;
        size_t lvgl_total = 0;
        size_t stack_used = 0;
        size_t stack_total = 0;
        double heap_percent = 0.0;
        double lvgl_percent = 0.0;
        double stack_percent = 0.0;
        std::chrono::steady_clock::time_point timestamp;
    };

    struct MemoryThresholds {
        double heap_warning = 75.0;    // Warn at 75% heap usage
        double heap_critical = 90.0;   // Critical at 90% heap usage
        double lvgl_warning = 80.0;    // Warn at 80% LVGL memory usage
        double lvgl_critical = 95.0;   // Critical at 95% LVGL memory usage
        size_t leak_detection_bytes = 1024; // Detect leaks > 1KB growth
        std::chrono::seconds leak_detection_window{60}; // Check for leaks every minute
    };

    enum class AlertLevel {
        INFO,
        WARNING,
        CRITICAL
    };

    using AlertCallback = std::function<void(AlertLevel, const std::string&, const MemoryStats&)>;

    static MemoryHealthMonitor& getInstance() {
        static MemoryHealthMonitor instance;
        return instance;
    }

    // Configuration
    void setThresholds(const MemoryThresholds& thresholds) { thresholds_ = thresholds; }
    void setAlertCallback(AlertCallback callback) { alert_callback_ = callback; }
    void enableLogging(const std::string& log_file = "memory_health.log") { 
        log_file_ = log_file; 
        logging_enabled_ = true;
    }

    // Monitoring
    void startMonitoring(std::chrono::milliseconds interval = std::chrono::milliseconds(5000));
    void stopMonitoring();
    MemoryStats getCurrentStats();
    void forceCheck();

    // Manual tracking
    void recordAllocation(size_t bytes, const std::string& context = "");
    void recordDeallocation(size_t bytes, const std::string& context = "");

    // Reporting
    void printMemoryReport();
    void dumpMemoryHistory(size_t last_n_samples = 100);
    std::vector<MemoryStats> getMemoryHistory() const { return memory_history_; }

    // Leak detection
    bool detectPotentialLeak();
    void triggerGarbageCollection();

private:
    MemoryHealthMonitor() = default;
    ~MemoryHealthMonitor() { stopMonitoring(); }

    void monitoringLoop();
    void checkThresholds(const MemoryStats& stats);
    void logMemoryStats(const MemoryStats& stats);
    size_t getHeapUsage();
    size_t getLVGLMemoryUsage();
    size_t getStackUsage();

    MemoryThresholds thresholds_;
    AlertCallback alert_callback_;
    std::vector<MemoryStats> memory_history_;
    bool monitoring_active_ = false;
    bool logging_enabled_ = false;
    std::string log_file_;
    
    // Leak detection
    size_t baseline_heap_ = 0;
    std::chrono::steady_clock::time_point last_leak_check_;
    
    // Manual tracking
    size_t manual_allocations_ = 0;
    size_t manual_deallocations_ = 0;

    static constexpr size_t MAX_HISTORY_SIZE = 1000;
};
