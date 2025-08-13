#include "EventVisualizerManager.h"

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)

namespace Debug {

// Static instance definition
EventVisualizerManager* EventVisualizerManager::instance_ = nullptr;

void EventVisualizerManager::updateEventsCallback(lv_timer_t* timer) {
    auto* manager = static_cast<EventVisualizerManager*>(lv_timer_get_user_data(timer));
    if (manager) {
        manager->updateRealTimeEvents();
    }
}

void EventVisualizerManager::updateRealTimeEvents() {
    if (!event_log_area_) return;
    
    auto& tracer = Debug::RTEventTracer::getInstance();
    Debug::RTEventTracer::EventTrace event;
    
    // Process up to 5 events per update to avoid blocking UI
    int events_processed = 0;
    while (events_processed < 5 && tracer.popTrace(event)) {
        
        // Clear old events if we have too many (keep scrolling effect)
        lv_obj_t* child = lv_obj_get_child(event_log_area_, -1); // Get last child (oldest)
        if (lv_obj_get_child_cnt(event_log_area_) > 7) { // Keep max 7 events (title + 6 events)
            if (child && child != lv_obj_get_child(event_log_area_, 0)) { // Don't delete the title
                lv_obj_del(child);
            }
        }
        
        // Create event display
        std::string event_text = std::string(event.source_name) + " → " + 
                               std::string(event.target_name) + ": " + 
                               std::string(event.event_name);
        
        if (strlen(event.event_data) > 0) {
            event_text += " (" + std::string(event.event_data) + ")";
        }
        
        // Add timestamp
        auto ms = (event.timestamp_us / 1000) % 10000;
        event_text = "[" + std::to_string(ms) + "ms] " + event_text;
        
        // Create label for this event
        lv_obj_t* event_label = lv_label_create(event_log_area_);
        lv_label_set_text(event_label, event_text.c_str());
        lv_obj_set_style_text_font(event_label, &lv_font_montserrat_12, 0);
        
        // Color based on event type
        lv_color_t color = lv_color_hex(0xCCCCCC);
        switch (static_cast<Debug::RTEventTracer::EventType>(event.event_type)) {
            case Debug::RTEventTracer::EventType::MIDI_EVENT:
                color = lv_color_hex(0x00FF88);
                break;
            case Debug::RTEventTracer::EventType::UI_EVENT:
                color = lv_color_hex(0x88AAFF);
                break;
            case Debug::RTEventTracer::EventType::PARAMETER_EVENT:
                color = lv_color_hex(0xFFAA00);
                break;
            case Debug::RTEventTracer::EventType::CLOCK_EVENT:
                color = lv_color_hex(0xFF4488);
                break;
            default:
                break;
        }
        lv_obj_set_style_text_color(event_label, color, 0);
        
        // Move the event to the top (newest events at top)
        lv_obj_move_to_index(event_label, 1); // After the title (index 0)
        
        events_processed++;
    }
    
    // Reposition all event labels to create scrolling effect
    if (events_processed > 0) {
        uint32_t child_count = lv_obj_get_child_cnt(event_log_area_);
        for (uint32_t i = 1; i < child_count; i++) { // Skip title (index 0)
            lv_obj_t* event_label = lv_obj_get_child(event_log_area_, i);
            if (event_label) {
                lv_obj_set_pos(event_label, 10, 25 + ((i-1) * 18));
            }
        }
    }
}

void EventVisualizerManager::createComponentBox(int x, int y, int w, int h, uint32_t color, const char* title, const char* tooltip) {
    lv_obj_t* box = lv_obj_create(container_);
    lv_obj_set_size(box, w, h);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_style_bg_color(box, lv_color_hex(color), 0);
    lv_obj_set_style_border_color(box, lv_color_hex(color & 0x808080), 0);
    lv_obj_set_style_border_width(box, 2, 0);
    lv_obj_set_style_radius(box, 6, 0);
    
    lv_obj_t* label = lv_label_create(box);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
    lv_obj_center(label);
}

} // namespace Debug

#endif // DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER
