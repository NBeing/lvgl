#include "MidiMonitor.h"
#include <sstream>
#include <iostream>

namespace UI {

MidiMonitor::MidiMonitor() {}

void MidiMonitor::create(lv_obj_t* parent) {
    // Create a scrollable container for the text
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(98), LV_PCT(90));
    lv_obj_set_style_bg_color(container, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_color(container, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(container, 1, 0);
    lv_obj_set_style_pad_all(container, 8, 0);
    
    // Create label for text display
    label_ = lv_label_create(container);
    lv_obj_set_width(label_, LV_PCT(100));
    lv_label_set_long_mode(label_, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(label_, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(label_, &lv_font_montserrat_12, 0);
    lv_label_set_text(label_, "MIDI Monitor (Inactive)\nSwitch to this tab to enable monitoring");
}

void MidiMonitor::logInput(const std::string& msg) {
    // For compatibility - but we use the global queue now
    if (is_active_) {
        MidiLogQueue::getInstance().logInput(msg.c_str());
    }
}

void MidiMonitor::logOutput(const std::string& msg) {
    // For compatibility - but we use the global queue now  
    if (is_active_) {
        MidiLogQueue::getInstance().logOutput(msg.c_str());
    }
}

void MidiMonitor::update() {
    static int update_counter = 0;
    static int last_debug_counter = 0;
    update_counter++;
    
    // Debug: Show we're being called every 200 calls, but also show when we get messages
    bool show_debug = (update_counter % 200 == 0);
    
    // ALWAYS show debug when active and periodically
    if (is_active_ && (update_counter - last_debug_counter) >= 200) {
        std::cout << "[MIDI Monitor] *** MAIN LOOP UPDATE #" << update_counter << " - ACTIVE - checking queue..." << std::endl;
        last_debug_counter = update_counter;
        show_debug = true;
    }
    
    // Always process queue messages regardless of active state
    MidiLogQueue::LogEntry entry;
    bool got_messages = false;
    int message_count = 0;
    
    while (MidiLogQueue::getInstance().popEntry(entry) && message_count < 10) {
        std::cout << "[MIDI Monitor] *** PROCESSING QUEUE MESSAGE (#" << update_counter << "): " << entry.message << std::endl;
        std::string line = (entry.type == MidiLogQueue::LogEntry::MIDI_INPUT ? "[IN]  " : "[OUT] ");
        line += entry.message;
        
        display_lines_.push_back(line);
        if (display_lines_.size() > MAX_DISPLAY_LINES) {
            display_lines_.erase(display_lines_.begin());
        }
        
        got_messages = true;
        message_count++;
        show_debug = true; // Always show debug when we get messages
    }
    
    if (show_debug && is_active_) {
        std::cout << "[MIDI Monitor] Update #" << update_counter << " - active: " << is_active_ 
                  << " - got " << message_count << " messages - total lines: " << display_lines_.size() << std::endl;
    }
    
    // Update display when active
    if (!is_active_) {
        if (label_ && update_counter % 100 == 0) {
            lv_label_set_text(label_, "MIDI Monitor (Inactive)\nSwitch to this tab to enable monitoring");
        }
        return;
    }
    
    // ALWAYS update display when we get new messages, regardless of needs_update_ flag
    if (got_messages) {
        std::cout << "[MIDI Monitor] *** UPDATING DISPLAY with " << display_lines_.size() << " lines (got " << message_count << " new messages)" << std::endl;
        updateDisplay();
        needs_update_ = true;
    }
    // Also update on first call when tab becomes active
    else if (!needs_update_) {
        std::cout << "[MIDI Monitor] Initial display update (first time active)" << std::endl;
        updateDisplay();
        needs_update_ = true;
    }
}

void MidiMonitor::updateDisplay() {
    std::cout << "[MIDI Monitor] *** updateDisplay() called - label_: " << (label_ ? "valid" : "NULL") 
              << " - active: " << is_active_ << std::endl;
    
    if (!label_) {
        std::cout << "[MIDI Monitor] ERROR: No label!" << std::endl;
        return;
    }
    
    if (!is_active_) {
        std::cout << "[MIDI Monitor] Setting inactive text" << std::endl;
        lv_label_set_text(label_, "MIDI Monitor (Inactive)\nSwitch to this tab to enable monitoring");
        lv_obj_invalidate(label_);  // Force redraw
        return;
    }
    
    std::ostringstream oss;
    oss << "MIDI Monitor (Active)\n";
    
    if (display_lines_.empty()) {
        oss << "Waiting for MIDI activity...\n";
    } else {
        std::cout << "[MIDI Monitor] Adding " << display_lines_.size() << " lines to display" << std::endl;
        for (const auto& line : display_lines_) {
            oss << line << "\n";
        }
    }
    
    std::string final_text = oss.str();
    std::cout << "[MIDI Monitor] *** Setting text (" << final_text.length() << " chars): " 
              << final_text.substr(0, 80) << "..." << std::endl;
    lv_label_set_text(label_, final_text.c_str());
    
    // Force LVGL to redraw the label immediately
    lv_obj_invalidate(label_);
    
    // Also invalidate the parent container to ensure proper refresh
    if (lv_obj_get_parent(label_)) {
        lv_obj_invalidate(lv_obj_get_parent(label_));
    }
    
    std::cout << "[MIDI Monitor] *** UI invalidated for redraw" << std::endl;
}

void MidiMonitor::clear() {
    display_lines_.clear();
    MidiLogQueue::getInstance().clear();
    needs_update_ = false;
    updateDisplay();
}

} // namespace UI
