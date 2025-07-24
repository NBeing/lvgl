#pragma once

#include "components/ui/Window.h"
#include "components/midi/MidiEvents.h"
#include "components/midi/SimpleMidiClockProcessor.h"
#include <lvgl.h>
#include <string>
#include <iostream>

/**
 * @brief Simple clock display tab for testing threaded architecture
 */
class SimpleClockTab : public Tab, public MIDI::TypedObserver<MIDI::ClockEvent> {
private:
    // UI elements
    lv_obj_t* bpm_label_ = nullptr;
    lv_obj_t* status_label_ = nullptr;
    lv_obj_t* tick_label_ = nullptr;
    lv_obj_t* start_btn_ = nullptr;
    lv_obj_t* stop_btn_ = nullptr;
    
    // State
    MIDI::SimpleMidiClockProcessor* midi_clock_ = nullptr;
    uint32_t last_tick_count_ = 0;
    bool clock_running_ = false;
    
public:
    SimpleClockTab(MIDI::SimpleMidiClockProcessor* clock) 
        : Tab("Clock"), midi_clock_(clock) {
        
        // Subscribe to clock events
        if (midi_clock_) {
            midi_clock_->addClockObserver(this);
        }
    }
    
    ~SimpleClockTab() {
        if (midi_clock_) {
            midi_clock_->removeClockObserver(this);
        }
    }
    
    void create(lv_obj_t* parent) override {
        container_ = lv_obj_create(parent);
        lv_obj_set_size(container_, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(container_, lv_color_hex(0x000000), 0);
        
        createUI();
        updateDisplay();
    }
    
    // MIDI::TypedObserver<ClockEvent> implementation
    void onEvent(const MIDI::ClockEvent& event) override {
        // This is called from UI thread (safe to update UI)
        switch (event.type) {
            case MIDI::ClockEvent::START:
                clock_running_ = true;
                updateStatus("RUNNING");
                break;
                
            case MIDI::ClockEvent::STOP:
                clock_running_ = false;
                updateStatus("STOPPED");
                break;
                
            case MIDI::ClockEvent::CONTINUE:
                clock_running_ = true;
                updateStatus("RUNNING");
                break;
                
            case MIDI::ClockEvent::TICK:
                last_tick_count_ = event.tick_count;
                updateTickDisplay();
                break;
        }
    }
    
private:
    void createUI() {
        // Title
        lv_obj_t* title = lv_label_create(container_);
        lv_label_set_text(title, "MIDI Clock");
        lv_obj_set_style_text_color(title, lv_color_hex(0x00FF00), 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
        
        // BPM display
        bpm_label_ = lv_label_create(container_);
        lv_label_set_text(bpm_label_, "BPM: 120.0");
        lv_obj_set_style_text_color(bpm_label_, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(bpm_label_, LV_ALIGN_CENTER, 0, -40);
        
        // Status display
        status_label_ = lv_label_create(container_);
        lv_label_set_text(status_label_, "Status: STOPPED");
        lv_obj_set_style_text_color(status_label_, lv_color_hex(0xFF8000), 0);
        lv_obj_align(status_label_, LV_ALIGN_CENTER, 0, -10);
        
        // Tick count display
        tick_label_ = lv_label_create(container_);
        lv_label_set_text(tick_label_, "Ticks: 0");
        lv_obj_set_style_text_color(tick_label_, lv_color_hex(0x00FFFF), 0);
        lv_obj_align(tick_label_, LV_ALIGN_CENTER, 0, 20);
        
        // Control buttons
        createButtons();
    }
    
    void createButtons() {
        // Start button
        start_btn_ = lv_btn_create(container_);
        lv_obj_set_size(start_btn_, 80, 40);
        lv_obj_align(start_btn_, LV_ALIGN_BOTTOM_LEFT, 20, -20);
        lv_obj_set_style_bg_color(start_btn_, lv_color_hex(0x00AA00), 0);
        
        lv_obj_t* start_label = lv_label_create(start_btn_);
        lv_label_set_text(start_label, "START");
        lv_obj_center(start_label);
        
        lv_obj_add_event_cb(start_btn_, [](lv_event_t* e) {
            if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
                SimpleClockTab* tab = static_cast<SimpleClockTab*>(lv_event_get_user_data(e));
                tab->onStartClicked();
            }
        }, LV_EVENT_CLICKED, this);
        
        // Stop button
        stop_btn_ = lv_btn_create(container_);
        lv_obj_set_size(stop_btn_, 80, 40);
        lv_obj_align(stop_btn_, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
        lv_obj_set_style_bg_color(stop_btn_, lv_color_hex(0xAA0000), 0);
        
        lv_obj_t* stop_label = lv_label_create(stop_btn_);
        lv_label_set_text(stop_label, "STOP");
        lv_obj_center(stop_label);
        
        lv_obj_add_event_cb(stop_btn_, [](lv_event_t* e) {
            if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
                SimpleClockTab* tab = static_cast<SimpleClockTab*>(lv_event_get_user_data(e));
                tab->onStopClicked();
            }
        }, LV_EVENT_CLICKED, this);
    }
    
    void onStartClicked() {
        if (midi_clock_) {
            midi_clock_->startClock();
            std::cout << "[Clock Tab] Start button clicked" << std::endl;
        }
    }
    
    void onStopClicked() {
        if (midi_clock_) {
            midi_clock_->stopClock();
            std::cout << "[Clock Tab] Stop button clicked" << std::endl;
        }
    }
    
    void updateDisplay() {
        if (midi_clock_) {
            // Update BPM
            float bpm = midi_clock_->getBPM();
            char bpm_text[32];
            snprintf(bpm_text, sizeof(bpm_text), "BPM: %.1f", bpm);
            if (bpm_label_) {
                lv_label_set_text(bpm_label_, bpm_text);
            }
            
            // Update status
            const char* status = midi_clock_->isClockRunning() ? "RUNNING" : "STOPPED";
            updateStatus(status);
            
            // Update tick count
            uint32_t ticks = midi_clock_->getTickCount();
            updateTickCount(ticks);
        }
    }
    
    void updateStatus(const char* status) {
        if (status_label_) {
            char status_text[64];
            snprintf(status_text, sizeof(status_text), "Status: %s", status);
            lv_label_set_text(status_label_, status_text);
            
            // Color coding
            lv_color_t color = clock_running_ ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF8000);
            lv_obj_set_style_text_color(status_label_, color, 0);
        }
    }
    
    void updateTickDisplay() {
        updateTickCount(last_tick_count_);
    }
    
    void updateTickCount(uint32_t ticks) {
        if (tick_label_) {
            char tick_text[32];
            snprintf(tick_text, sizeof(tick_text), "Ticks: %u", ticks);
            lv_label_set_text(tick_label_, tick_text);
        }
    }
};
