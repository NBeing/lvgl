#include "MidiMonitorTab.h"
#include "components/ui/ContainerFactory.h"
#include "FontConfig.h"
#include "Constants.h"
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

    // Add a clear button with consistent styling
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
    
    // Add a simple welcome message
    UI::MidiLogQueue::getInstance().logOutput("Monitoring started");
    
    // Force an immediate update to show the welcome message
    monitor_.update();
    std::cout << "Forced monitor update called" << std::endl;
}

void MidiMonitorTab::onDeactivated() {
    std::cout << "MidiMonitorTab deactivated - MIDI monitoring DISABLED" << std::endl;
    
    // Disable monitoring when tab is inactive
    monitor_.setActive(false);
    std::cout << "Monitor setActive(false) called" << std::endl;
}
