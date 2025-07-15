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
    
    lv_obj_t* scroll_container_ = nullptr;  // Container for scrollable log lines
    bool is_active_ = false;
    
    // Display buffer for accumulated messages
    std::vector<std::string> display_lines_;
    std::vector<lv_obj_t*> line_containers_;  // Store line containers for cleanup
    bool needs_update_ = false;
    
    void updateDisplay();
    void createLogLine(const std::string& text, uint32_t text_color, bool is_alternate_bg);
};

} // namespace UI
