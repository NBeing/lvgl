#include "MidiMonitor.h"
#include "components/ui/ContainerFactory.h"
#include "FontConfig.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring>

namespace UI {

MidiMonitor::MidiMonitor() {}

void MidiMonitor::create(lv_obj_t* parent) {
    // Create a scrollable container for the text using ContainerFactory
    lv_obj_t* container = UI::createContainer({
        .parent = parent,
        .width_pct = 98,
        .height_pct = 90,
        .align = LV_ALIGN_TOP_LEFT,
        .x_offset = 0,
        .y_offset = 0,
        .bg_color = lv_color_hex(0x000000),
        .bg_opa = LV_OPA_COVER,
        .border_width = 1,
        .pad_all = 2,
        .use_bg_color = true
    });
    
    // Add border color manually since it's not in ContainerOptions
    lv_obj_set_style_border_color(container, lv_color_hex(0x333333), 0);
    
    // Create a scrollable container for the log lines
    scroll_container_ = lv_obj_create(container);
    lv_obj_set_size(scroll_container_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(scroll_container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(scroll_container_, 0, 0);
    lv_obj_set_style_pad_all(scroll_container_, 2, 0);
    lv_obj_set_scroll_dir(scroll_container_, LV_DIR_VER);
    
    // Set up flex layout for vertical stacking
    lv_obj_set_layout(scroll_container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scroll_container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scroll_container_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    
    // Create initial inactive message
    createLogLine("MIDI Monitor (Inactive)", 0x657b83, false);
    createLogLine("Switch to this tab to enable monitoring", 0x657b83, false);
}

void MidiMonitor::createLogLine(const std::string& text, uint32_t text_color, bool is_alternate_bg) {
    // Create container for this line with alternating background
    lv_obj_t* line_container = lv_obj_create(scroll_container_);
    lv_obj_set_size(line_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_border_width(line_container, 0, 0);
    lv_obj_set_style_pad_all(line_container, 4, 0);
    lv_obj_set_style_pad_ver(line_container, 2, 0);
    
    // Alternating background colors - darker gray for alternate rows
    if (is_alternate_bg) {
        lv_obj_set_style_bg_color(line_container, lv_color_hex(0x1a1a1a), 0);  // Slightly lighter than black
        lv_obj_set_style_bg_opa(line_container, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_bg_opa(line_container, LV_OPA_TRANSP, 0);  // Transparent (black background shows through)
    }
    
    // Create label for the text
    lv_obj_t* line_label = lv_label_create(line_container);
    lv_obj_set_width(line_label, LV_PCT(100));
    lv_label_set_long_mode(line_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(line_label, lv_color_hex(text_color), 0);
    lv_obj_set_style_text_font(line_label, FontA.small, 0);
    lv_label_set_text(line_label, text.c_str());
    
    // Store the line container for potential cleanup
    line_containers_.push_back(line_container);
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
        std::cout << "[MIDI Monitor] *** PROCESSING QUEUE MESSAGE (#" << update_counter << "): " << entry.friendly_name << std::endl;
        
        // Format the entry for display using the new enhanced format
        std::string formatted_line = formatLogEntry(entry);
        display_lines_.push_back(formatted_line);
        
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
        if (scroll_container_ && update_counter % 100 == 0) {
            // Update inactive display periodically
            updateDisplay();
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
    std::cout << "[MIDI Monitor] *** updateDisplay() called - scroll_container_: " << (scroll_container_ ? "valid" : "NULL") 
              << " - active: " << is_active_ << std::endl;
    
    if (!scroll_container_) {
        std::cout << "[MIDI Monitor] ERROR: No scroll container!" << std::endl;
        return;
    }
    
    // Clear existing line containers
    for (auto* container : line_containers_) {
        lv_obj_del(container);
    }
    line_containers_.clear();
    
    if (!is_active_) {
        std::cout << "[MIDI Monitor] Setting inactive text" << std::endl;
        createLogLine("MIDI Monitor (Inactive)", 0x657b83, false);
        createLogLine("Switch to this tab to enable monitoring", 0x657b83, false);
        return;
    }
    
    // Header
    createLogLine("MIDI Monitor (Active)", 0x268bd2, false);
    
    if (display_lines_.empty()) {
        createLogLine("Waiting for MIDI activity...", 0x657b83, true);
    } else {
        std::cout << "[MIDI Monitor] Adding " << display_lines_.size() << " lines to display" << std::endl;
        for (size_t i = 0; i < display_lines_.size(); ++i) {
            // Determine color based on message content
            uint32_t color = 0x839496;  // Default gray
            
            if (display_lines_[i].find(" IN ") != std::string::npos) {
                // Input messages in cooler colors (cyan, blue, green)
                const uint32_t input_colors[] = {0x2aa198, 0x268bd2, 0x859900};
                color = input_colors[i % 3];
            } else if (display_lines_[i].find(" OUT ") != std::string::npos) {
                // Output messages in warmer colors (yellow, orange tones)
                const uint32_t output_colors[] = {0xb58900, 0xcb4b16, 0xdc322f};
                color = output_colors[i % 3];
            }
            
            // Alternate background every other line (starting from line 1, after header)
            bool is_alternate = ((i + 1) % 2) == 0;
            createLogLine(display_lines_[i], color, is_alternate);
        }
    }
    
    // Scroll to bottom to show latest messages
    lv_obj_scroll_to_y(scroll_container_, LV_COORD_MAX, LV_ANIM_ON);
    
    std::cout << "[MIDI Monitor] *** Display updated with alternating backgrounds" << std::endl;
}

std::string MidiMonitor::formatLogEntry(const MidiLogQueue::LogEntry& entry) {
    std::ostringstream oss;
    
    // Timestamp (optional)
    if (show_timestamp_) {
        oss << "[" << std::setw(5) << std::setfill('0') << (entry.timestamp % 100000) << "] ";
    }
    
    // Source
    const char* source_str = "HW";
    switch (entry.source) {
        case MidiLogQueue::LogEntry::HARDWARE: source_str = "HW"; break;
        case MidiLogQueue::LogEntry::USB: source_str = "USB"; break;
        case MidiLogQueue::LogEntry::INTERNAL: source_str = "INT"; break;
    }
    oss << source_str << " ";
    
    // Direction
    oss << (entry.type == MidiLogQueue::LogEntry::MIDI_INPUT ? "IN " : "OUT") << " ";
    
    // Hex data (optional)
    if (show_hex_data_ && strlen(entry.hex_data) > 0) {
        oss << std::setw(8) << std::left << entry.hex_data << " ";
    }
    
    // Friendly name
    oss << entry.friendly_name;
    
    return oss.str();
}

void MidiMonitor::clear() {
    display_lines_.clear();
    MidiLogQueue::getInstance().clear();
    needs_update_ = false;
    updateDisplay();
}

} // namespace UI
