#pragma once

#include "lvgl.h"
#include <chrono>
#include <vector>
#include <functional>
#include <string>
#include <cstring>

/**
 * @brief Specialized LVGL memory pool monitoring
 * 
 * Tracks LVGL object creation/destruction, memory pool fragmentation,
 * and provides detailed LVGL-specific memory analysis.
 */
class LVGLMemoryMonitor {
public:
    struct LVGLMemoryInfo {
        size_t total_size = 0;
        size_t free_size = 0;
        size_t used_size = 0;
        size_t free_cnt = 0;
        size_t free_biggest_size = 0;
        size_t used_cnt = 0;
        size_t max_used = 0;
        uint8_t used_pct = 0;
        uint8_t frag_pct = 0;
        std::chrono::steady_clock::time_point timestamp;
    };

    struct ObjectStats {
        uint32_t total_objects = 0;
        uint32_t screens = 0;
        uint32_t containers = 0;
        uint32_t labels = 0;
        uint32_t buttons = 0;
        uint32_t other_widgets = 0;
    };

    using MemoryAlertCallback = std::function<void(const std::string&, const LVGLMemoryInfo&)>;

    static LVGLMemoryMonitor& getInstance() {
        static LVGLMemoryMonitor instance;
        return instance;
    }

    // Monitoring control
    void startMonitoring(std::chrono::milliseconds interval = std::chrono::milliseconds(2000));
    void stopMonitoring();
    
    // Data collection
    LVGLMemoryInfo getCurrentMemoryInfo();
    ObjectStats getCurrentObjectStats();
    
    // Analysis
    bool isFragmented(uint8_t threshold_pct = 20);
    bool isLowMemory(uint8_t threshold_pct = 90);
    void analyzeMemoryPattern();
    
    // Reporting
    void printDetailedReport();
    void printObjectSummary();
    
    // Configuration
    void setAlertCallback(MemoryAlertCallback callback) { alert_callback_ = callback; }
    void setFragmentationThreshold(uint8_t pct) { fragmentation_threshold_ = pct; }
    void setLowMemoryThreshold(uint8_t pct) { low_memory_threshold_ = pct; }

    // Object tracking
    void onObjectCreated(lv_obj_t* obj);
    void onObjectDeleted(lv_obj_t* obj);

private:
    LVGLMemoryMonitor() = default;
    ~LVGLMemoryMonitor() { stopMonitoring(); }

    void monitoringLoop();
    void checkAlerts(const LVGLMemoryInfo& info);
    const char* getObjectTypeName(lv_obj_t* obj);

    bool monitoring_active_ = false;
    MemoryAlertCallback alert_callback_;
    std::vector<LVGLMemoryInfo> memory_history_;
    
    // Thresholds
    uint8_t fragmentation_threshold_ = 20;
    uint8_t low_memory_threshold_ = 90;
    
    // Object tracking
    ObjectStats current_objects_;
    uint32_t total_objects_created_ = 0;
    uint32_t total_objects_deleted_ = 0;

    static constexpr size_t MAX_HISTORY_SIZE = 500;
};
