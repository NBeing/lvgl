#include "LVGLMemoryMonitor.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <cstring>

void LVGLMemoryMonitor::startMonitoring(std::chrono::milliseconds interval) {
    if (monitoring_active_) return;
    
    monitoring_active_ = true;
    
    std::cout << "[LVGLMonitor] 🎨 LVGL memory monitoring started (interval: " 
              << interval.count() << "ms)" << std::endl;
    
    // Start monitoring thread
    std::thread([this, interval]() {
        while (monitoring_active_) {
            try {
                auto info = getCurrentMemoryInfo();
                memory_history_.push_back(info);
                
                // Limit history size
                if (memory_history_.size() > MAX_HISTORY_SIZE) {
                    memory_history_.erase(memory_history_.begin());
                }
                
                checkAlerts(info);
                
            } catch (const std::exception& e) {
                std::cerr << "[LVGLMonitor] Error during monitoring: " << e.what() << std::endl;
            }
            
            std::this_thread::sleep_for(interval);
        }
    }).detach();
}

void LVGLMemoryMonitor::stopMonitoring() {
    monitoring_active_ = false;
    std::cout << "[LVGLMonitor] LVGL memory monitoring stopped" << std::endl;
}

LVGLMemoryMonitor::LVGLMemoryInfo LVGLMemoryMonitor::getCurrentMemoryInfo() {
    LVGLMemoryInfo info;
    info.timestamp = std::chrono::steady_clock::now();
    
#if LV_USE_MEM_MONITOR
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    
    info.total_size = mon.total_size;
    info.free_size = mon.free_size;
    info.used_size = mon.total_size - mon.free_size;
    info.free_cnt = mon.free_cnt;
    info.free_biggest_size = mon.free_biggest_size;
    info.used_cnt = mon.used_cnt;
    info.max_used = mon.max_used;
    info.used_pct = mon.used_pct;
    info.frag_pct = mon.frag_pct;
#else
    // Fallback for when memory monitoring is disabled
    info.total_size = 256 * 1024; // 256KB default
    info.used_size = 64 * 1024;   // Estimate 64KB used
    info.free_size = info.total_size - info.used_size;
    info.used_pct = (info.used_size * 100) / info.total_size;
    info.frag_pct = 10; // Estimate 10% fragmentation
#endif
    
    return info;
}

LVGLMemoryMonitor::ObjectStats LVGLMemoryMonitor::getCurrentObjectStats() {
    return current_objects_;
}

bool LVGLMemoryMonitor::isFragmented(uint8_t threshold_pct) {
    auto info = getCurrentMemoryInfo();
    return info.frag_pct > threshold_pct;
}

bool LVGLMemoryMonitor::isLowMemory(uint8_t threshold_pct) {
    auto info = getCurrentMemoryInfo();
    return info.used_pct > threshold_pct;
}

void LVGLMemoryMonitor::analyzeMemoryPattern() {
    if (memory_history_.size() < 3) {
        std::cout << "[LVGLMonitor] Not enough data for pattern analysis" << std::endl;
        return;
    }
    
    auto recent = memory_history_.end() - 3;
    auto current = memory_history_.back();
    
    // Calculate trends
    size_t memory_growth = 0;
    int fragmentation_trend = 0;
    
    for (auto it = recent; it != memory_history_.end(); ++it) {
        if (it != recent) {
            auto prev = it - 1;
            memory_growth += (it->used_size > prev->used_size) ? (it->used_size - prev->used_size) : 0;
            fragmentation_trend += (int)it->frag_pct - (int)prev->frag_pct;
        }
    }
    
    std::cout << "\n🔍 LVGL MEMORY PATTERN ANALYSIS" << std::endl;
    std::cout << "================================" << std::endl;
    std::cout << "📈 Memory growth (last 3 samples): " << (memory_growth / 1024) << " KB" << std::endl;
    std::cout << "🧩 Fragmentation trend: " << (fragmentation_trend > 0 ? "↗️ Increasing" : 
                                              fragmentation_trend < 0 ? "↘️ Decreasing" : "➡️ Stable") << std::endl;
    std::cout << "🎯 Current efficiency: " << (100 - current.frag_pct) << "%" << std::endl;
    
    // Recommendations
    if (current.frag_pct > 25) {
        std::cout << "💡 Recommendation: High fragmentation detected - consider object pooling" << std::endl;
    }
    if (memory_growth > 5120) { // 5KB growth
        std::cout << "💡 Recommendation: Significant memory growth - check for leaks" << std::endl;
    }
    if (current.used_pct > 85) {
        std::cout << "💡 Recommendation: Memory usage high - consider increasing pool size" << std::endl;
    }
    std::cout << "================================\n" << std::endl;
}

void LVGLMemoryMonitor::printDetailedReport() {
    auto info = getCurrentMemoryInfo();
    auto objects = getCurrentObjectStats();
    
    std::cout << "\n🎨 DETAILED LVGL MEMORY REPORT" << std::endl;
    std::cout << "===============================" << std::endl;
    
    // Memory statistics
    std::cout << "📊 Memory Pool:" << std::endl;
    std::cout << "   Total:     " << std::setw(8) << (info.total_size / 1024) << " KB" << std::endl;
    std::cout << "   Used:      " << std::setw(8) << (info.used_size / 1024) << " KB (" 
              << (int)info.used_pct << "%)" << std::endl;
    std::cout << "   Free:      " << std::setw(8) << (info.free_size / 1024) << " KB" << std::endl;
    std::cout << "   Max Used:  " << std::setw(8) << (info.max_used / 1024) << " KB" << std::endl;
    std::cout << "   Largest:   " << std::setw(8) << (info.free_biggest_size / 1024) << " KB" << std::endl;
    
    // Fragmentation analysis
    std::cout << "\n🧩 Fragmentation:" << std::endl;
    std::cout << "   Level:     " << std::setw(8) << (int)info.frag_pct << "%" << std::endl;
    std::cout << "   Status:    " << (info.frag_pct > fragmentation_threshold_ ? "⚠️ HIGH" : "✅ OK") << std::endl;
    std::cout << "   Free Cnt:  " << std::setw(8) << info.free_cnt << " blocks" << std::endl;
    std::cout << "   Used Cnt:  " << std::setw(8) << info.used_cnt << " blocks" << std::endl;
    
    // Object statistics
    std::cout << "\n🎯 Object Statistics:" << std::endl;
    std::cout << "   Total:     " << std::setw(8) << objects.total_objects << std::endl;
    std::cout << "   Screens:   " << std::setw(8) << objects.screens << std::endl;
    std::cout << "   Containers:" << std::setw(8) << objects.containers << std::endl;
    std::cout << "   Labels:    " << std::setw(8) << objects.labels << std::endl;
    std::cout << "   Buttons:   " << std::setw(8) << objects.buttons << std::endl;
    std::cout << "   Others:    " << std::setw(8) << objects.other_widgets << std::endl;
    
    // Lifecycle statistics
    std::cout << "\n♻️ Object Lifecycle:" << std::endl;
    std::cout << "   Created:   " << std::setw(8) << total_objects_created_ << std::endl;
    std::cout << "   Deleted:   " << std::setw(8) << total_objects_deleted_ << std::endl;
    std::cout << "   Active:    " << std::setw(8) << (total_objects_created_ - total_objects_deleted_) << std::endl;
    
    // Health indicators
    std::string memory_status = info.used_pct > 90 ? "🚨 CRITICAL" :
                               info.used_pct > 75 ? "⚠️ WARNING" : "✅ HEALTHY";
    std::string frag_status = info.frag_pct > 25 ? "⚠️ HIGH" :
                             info.frag_pct > 15 ? "🟡 MEDIUM" : "✅ LOW";
    
    std::cout << "\n🏥 Health Status:" << std::endl;
    std::cout << "   Memory:    " << memory_status << std::endl;
    std::cout << "   Fragment:  " << frag_status << std::endl;
    std::cout << "===============================\n" << std::endl;
}

void LVGLMemoryMonitor::printObjectSummary() {
    auto objects = getCurrentObjectStats();
    
    std::cout << "[LVGLMonitor] 🎯 Objects: " << objects.total_objects 
              << " (Screens:" << objects.screens 
              << ", Containers:" << objects.containers
              << ", Labels:" << objects.labels 
              << ", Buttons:" << objects.buttons 
              << ", Others:" << objects.other_widgets << ")" << std::endl;
}

void LVGLMemoryMonitor::onObjectCreated(lv_obj_t* obj) {
    if (!obj) return;
    
    current_objects_.total_objects++;
    total_objects_created_++;
    
    // Categorize by type
    const char* type_name = getObjectTypeName(obj);
    if (strstr(type_name, "screen")) {
        current_objects_.screens++;
    } else if (strstr(type_name, "cont") || strstr(type_name, "obj")) {
        current_objects_.containers++;
    } else if (strstr(type_name, "label")) {
        current_objects_.labels++;
    } else if (strstr(type_name, "btn")) {
        current_objects_.buttons++;
    } else {
        current_objects_.other_widgets++;
    }
}

void LVGLMemoryMonitor::onObjectDeleted(lv_obj_t* obj) {
    if (!obj) return;
    
    if (current_objects_.total_objects > 0) {
        current_objects_.total_objects--;
        total_objects_deleted_++;
        
        // Note: We can't easily determine which type was deleted,
        // so we'll just track totals for deletion
    }
}

void LVGLMemoryMonitor::checkAlerts(const LVGLMemoryInfo& info) {
    // Low memory alert
    if (info.used_pct > low_memory_threshold_) {
        if (alert_callback_) {
            alert_callback_("Low LVGL memory: " + std::to_string(info.used_pct) + "% used", info);
        }
    }
    
    // High fragmentation alert
    if (info.frag_pct > fragmentation_threshold_) {
        if (alert_callback_) {
            alert_callback_("High LVGL fragmentation: " + std::to_string(info.frag_pct) + "%", info);
        }
    }
}

const char* LVGLMemoryMonitor::getObjectTypeName(lv_obj_t* obj) {
    if (!obj) return "unknown";
    
    // Simplified approach - just return generic type
    // This avoids accessing incomplete type structures
    return "lvgl_object";
}
