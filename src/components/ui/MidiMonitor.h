#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <lvgl.h>
#include "MidiLogQueue.h"

namespace UI {

class MidiMonitor {
public:
    MidiMonitor();
    void create(lv_obj_t* parent);
    void logInput(const std::string& msg);   // For compatibility
    void logOutput(const std::string& msg);  // For compatibility
    void clear();
    void update(); // Call this from main loop
    
    // Enable/disable monitoring (only when tab is active)
    void setActive(bool active) { 
        is_active_ = active; 
        if (active) {
            needs_update_ = false;  // Reset flag when tab becomes active
        }
    }
    bool isActive() const { return is_active_; }
    
private:
    static const int MAX_DISPLAY_LINES = 15;
    
    lv_obj_t* label_ = nullptr;  // Changed from textarea_ to label_
    bool is_active_ = false;
    
    // Display buffer for accumulated messages
    std::vector<std::string> display_lines_;
    bool needs_update_ = false;
    
    void updateDisplay();
};

} // namespace UI
