#include "MidiMonitorTab.h"
#include "components/ui/ContainerFactory.h"
#include "FontConfig.h"
#include "Constants.h"
#include "components/midi/UnifiedMidiManager.h"
#include <lvgl.h>
#include <iostream>

MidiMonitorTab::MidiMonitorTab() 
    : Tab("MIDI Monitor")
{
}

void MidiMonitorTab::create(lv_obj_t* parent) {
    if (container_) return; // Already created

    // Create main container for this tab using ContainerFactory
    container_ = UI::createContainer({
        .parent = parent,
        .width_pct = 98,
        .height_pct = 98,
        .align = LV_ALIGN_TOP_LEFT,
        .x_offset = 0,
        .y_offset = 0,
        .bg_color = lv_color_black(),
        .bg_opa = LV_OPA_COVER,
        .border_width = 0,
        .pad_all = 4,
        .use_bg_color = true
    });

    setContainer(container_);

    // Create subtitle with consistent styling
    lv_obj_t* subtitle = lv_label_create(container_);
    lv_label_set_text(subtitle, "Real-time MIDI input and output monitoring");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(SynthConstants::Color::HELP), 0);
    lv_obj_set_style_text_font(subtitle, FontA.small, 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 0);

    // Create monitor with offset for title/subtitle using ContainerFactory
    lv_obj_t* monitor_container = UI::createContainer({
        .parent = container_,
        .width_pct = 98,
        .height_pct = 98,
        .align = LV_ALIGN_TOP_MID,
        .x_offset = 0,
        .y_offset = 0,
        .bg_color = lv_color_black(),
        .bg_opa = LV_OPA_TRANSP,
        .border_width = 0,
        .use_bg_color = true
    });

    monitor_.create(monitor_container);

    // Add control buttons at the bottom
    // Start/Stop monitoring button
    lv_obj_t* monitor_btn = lv_btn_create(container_);
    lv_obj_set_size(monitor_btn, 100, 30);
    lv_obj_align(monitor_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(monitor_btn, lv_color_hex(0x009900), 0); // Green when monitoring
    lv_obj_set_style_border_color(monitor_btn, lv_color_hex(0x00CC00), 0);
    lv_obj_set_style_border_width(monitor_btn, 1, 0);
    
    lv_obj_t* monitor_label = lv_label_create(monitor_btn);
    lv_label_set_text(monitor_label, "Stop Monitor");
    lv_obj_set_style_text_font(monitor_label, FontA.small, 0);
    lv_obj_set_style_text_color(monitor_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(monitor_label);
    
    lv_obj_add_event_cb(monitor_btn, [](lv_event_t* e){
        auto* tab = static_cast<MidiMonitorTab*>(lv_event_get_user_data(e));
        bool current = tab->getMonitor().isActive();
        tab->getMonitor().setActive(!current);
        
        // Update button appearance
        lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target(e));
        lv_obj_t* label = lv_obj_get_child(btn, 0);
        
        if (!current) { // Now monitoring
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x009900), 0); // Green
            lv_label_set_text(label, "Stop Monitor");
            std::cout << "[Monitor Button] Monitoring STARTED" << std::endl;
        } else { // Now stopped
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xCC0000), 0); // Red
            lv_label_set_text(label, "Start Monitor");
            std::cout << "[Monitor Button] Monitoring STOPPED" << std::endl;
        }
    }, LV_EVENT_CLICKED, this);

    // Timestamp toggle button
    lv_obj_t* timestamp_btn = lv_btn_create(container_);
    lv_obj_set_size(timestamp_btn, 80, 30);
    lv_obj_align(timestamp_btn, LV_ALIGN_BOTTOM_LEFT, 210, -10);
    lv_obj_set_style_bg_color(timestamp_btn, lv_color_hex(0x6600CC), 0); // Purple
    lv_obj_set_style_border_color(timestamp_btn, lv_color_hex(0x9900FF), 0);
    lv_obj_set_style_border_width(timestamp_btn, 1, 0);
    
    lv_obj_t* timestamp_label = lv_label_create(timestamp_btn);
    lv_label_set_text(timestamp_label, "Timestamp");
    lv_obj_set_style_text_font(timestamp_label, FontA.small, 0);
    lv_obj_set_style_text_color(timestamp_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(timestamp_label);
    
    lv_obj_add_event_cb(timestamp_btn, [](lv_event_t* e){
        auto* tab = static_cast<MidiMonitorTab*>(lv_event_get_user_data(e));
        bool current = tab->getMonitor().getShowTimestamp();
        tab->getMonitor().setShowTimestamp(!current);
        std::cout << "[Timestamp Button] Toggled to: " << (!current ? "ON" : "OFF") << std::endl;
        tab->getMonitor().update(); // Force refresh
    }, LV_EVENT_CLICKED, this);

    // Hex Data toggle button
    lv_obj_t* hex_btn = lv_btn_create(container_);
    lv_obj_set_size(hex_btn, 80, 30);
    lv_obj_align(hex_btn, LV_ALIGN_BOTTOM_LEFT, 300, -10);
    lv_obj_set_style_bg_color(hex_btn, lv_color_hex(0xCC6600), 0); // Orange
    lv_obj_set_style_border_color(hex_btn, lv_color_hex(0xFF9900), 0);
    lv_obj_set_style_border_width(hex_btn, 1, 0);
    
    lv_obj_t* hex_label = lv_label_create(hex_btn);
    lv_label_set_text(hex_label, "Hex Data");
    lv_obj_set_style_text_font(hex_label, FontA.small, 0);
    lv_obj_set_style_text_color(hex_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(hex_label);
    
    lv_obj_add_event_cb(hex_btn, [](lv_event_t* e){
        auto* tab = static_cast<MidiMonitorTab*>(lv_event_get_user_data(e));
        bool current = tab->getMonitor().getShowHexData();
        tab->getMonitor().setShowHexData(!current);
        std::cout << "[Hex Data Button] Toggled to: " << (!current ? "ON" : "OFF") << std::endl;
        tab->getMonitor().update(); // Force refresh
    }, LV_EVENT_CLICKED, this);

    // Clear button (moved to far right)
    lv_obj_t* clear_btn = lv_btn_create(container_);
    lv_obj_set_size(clear_btn, 80, 30);
    lv_obj_align(clear_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(SynthConstants::Color::BTN_FILTER_OFF), 0);
    lv_obj_set_style_border_color(clear_btn, lv_color_hex(SynthConstants::Color::BTN_FILTER_ON), 0);
    lv_obj_set_style_border_width(clear_btn, 1, 0);
    
    lv_obj_t* label = lv_label_create(clear_btn);
    lv_label_set_text(label, "Clear");
    lv_obj_set_style_text_font(label, FontA.small, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(label);
    
    lv_obj_add_event_cb(clear_btn, [](lv_event_t* e){
        auto* tab = static_cast<MidiMonitorTab*>(lv_event_get_user_data(e));
        tab->getMonitor().clear();
    }, LV_EVENT_CLICKED, this);

    std::cout << "MidiMonitorTab created with monitor and controls" << std::endl;
}

void MidiMonitorTab::onActivated() {
    std::cout << "MidiMonitorTab activated - MIDI monitoring ENABLED" << std::endl;
    
    // Enable monitoring when tab is active
    monitor_.setActive(true);
    std::cout << "Monitor setActive(true) called - isActive: " << monitor_.isActive() << std::endl;
    
    // Force an immediate update to refresh the display
    monitor_.update();
    std::cout << "Forced monitor update called" << std::endl;
}

void MidiMonitorTab::onDeactivated() {
    std::cout << "MidiMonitorTab deactivated - MIDI monitoring DISABLED" << std::endl;
    
    // Disable monitoring when tab is inactive
    monitor_.setActive(false);
    std::cout << "Monitor setActive(false) called" << std::endl;
}

