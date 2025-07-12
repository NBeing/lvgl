#include "TransportControl.h"
#include "Constants.h"
#include "FontConfig.h"
#include <iostream>

TransportControl::TransportControl()
    : container_(nullptr)
    , buttons_container_(nullptr)
    , display_container_(nullptr)
    , play_btn_(nullptr)
    , pause_btn_(nullptr)
    , stop_btn_(nullptr)
    , bpm_label_(nullptr)
    , bpm_value_label_(nullptr)
    , tick_label_(nullptr)
    , beat_label_(nullptr)
    , state_label_(nullptr)
    , current_state_(MidiClockManager::TransportState::STOPPED)
    , current_bpm_(120.0f)
    , current_tick_(0)
    , enabled_(true)
{
}

void TransportControl::create(lv_obj_t* parent) {
    if (container_) return; // Already created

    // Create main container
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(container_, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_border_color(container_, lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_width(container_, 1, 0);
    lv_obj_set_style_radius(container_, 8, 0);
    lv_obj_set_style_pad_all(container_, 12, 0);

    // Set up flex layout
    lv_obj_set_layout(container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    createButtons();
    createTempoDisplay();
    createTickDisplay();
    
    updateButtonStates();

    std::cout << "TransportControl created" << std::endl;
}

void TransportControl::destroy() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
        buttons_container_ = nullptr;
        display_container_ = nullptr;
        play_btn_ = nullptr;
        pause_btn_ = nullptr;
        stop_btn_ = nullptr;
        bpm_label_ = nullptr;
        bpm_value_label_ = nullptr;
        tick_label_ = nullptr;
        beat_label_ = nullptr;
        state_label_ = nullptr;
    }
}

void TransportControl::createButtons() {
    // Create button container
    buttons_container_ = lv_obj_create(container_);
    lv_obj_set_size(buttons_container_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(buttons_container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(buttons_container_, 0, 0);
    lv_obj_set_style_pad_all(buttons_container_, 5, 0);

    // Set up flex layout for buttons
    lv_obj_set_layout(buttons_container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(buttons_container_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(buttons_container_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(buttons_container_, 8, 0);

    // Play button
    play_btn_ = lv_btn_create(buttons_container_);
    lv_obj_set_size(play_btn_, 50, 50);
    lv_obj_set_style_bg_color(play_btn_, lv_color_hex(0x00AA00), 0);
    lv_obj_set_style_border_color(play_btn_, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_border_width(play_btn_, 2, 0);
    lv_obj_set_style_radius(play_btn_, 25, 0); // Round button
    lv_obj_add_event_cb(play_btn_, onPlayButtonClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* play_label = lv_label_create(play_btn_);
    lv_label_set_text(play_label, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_color(play_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(play_label);

    // Pause button
    pause_btn_ = lv_btn_create(buttons_container_);
    lv_obj_set_size(pause_btn_, 50, 50);
    lv_obj_set_style_bg_color(pause_btn_, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_style_border_color(pause_btn_, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_border_width(pause_btn_, 2, 0);
    lv_obj_set_style_radius(pause_btn_, 25, 0); // Round button
    lv_obj_add_event_cb(pause_btn_, onPauseButtonClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* pause_label = lv_label_create(pause_btn_);
    lv_label_set_text(pause_label, LV_SYMBOL_PAUSE);
    lv_obj_set_style_text_color(pause_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(pause_label);

    // Stop button
    stop_btn_ = lv_btn_create(buttons_container_);
    lv_obj_set_size(stop_btn_, 50, 50);
    lv_obj_set_style_bg_color(stop_btn_, lv_color_hex(0xAA0000), 0);
    lv_obj_set_style_border_color(stop_btn_, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_border_width(stop_btn_, 2, 0);
    lv_obj_set_style_radius(stop_btn_, 25, 0); // Round button
    lv_obj_add_event_cb(stop_btn_, onStopButtonClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* stop_label = lv_label_create(stop_btn_);
    lv_label_set_text(stop_label, LV_SYMBOL_STOP);
    lv_obj_set_style_text_color(stop_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(stop_label);
}

void TransportControl::createTempoDisplay() {
    // Create tempo display container
    lv_obj_t* tempo_container = lv_obj_create(container_);
    lv_obj_set_size(tempo_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(tempo_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tempo_container, 0, 0);
    lv_obj_set_style_pad_all(tempo_container, 5, 0);

    // BPM label
    bpm_label_ = lv_label_create(tempo_container);
    lv_label_set_text(bpm_label_, "BPM");
    lv_obj_set_style_text_color(bpm_label_, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(bpm_label_, FontA.small, 0);
    lv_obj_align(bpm_label_, LV_ALIGN_TOP_MID, 0, 0);

    // BPM value
    bpm_value_label_ = lv_label_create(tempo_container);
    lv_label_set_text_fmt(bpm_value_label_, "%.1f", current_bpm_);
    lv_obj_set_style_text_color(bpm_value_label_, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(bpm_value_label_, FontA.med, 0);
    lv_obj_align(bpm_value_label_, LV_ALIGN_BOTTOM_MID, 0, 0);
}

void TransportControl::createTickDisplay() {
    // Create display container
    display_container_ = lv_obj_create(container_);
    lv_obj_set_size(display_container_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(display_container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(display_container_, 0, 0);
    lv_obj_set_style_pad_all(display_container_, 5, 0);

    // Set up flex layout
    lv_obj_set_layout(display_container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(display_container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(display_container_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Transport state label
    state_label_ = lv_label_create(display_container_);
    lv_label_set_text(state_label_, "STOPPED");
    lv_obj_set_style_text_color(state_label_, lv_color_hex(0xFF6666), 0);
    lv_obj_set_style_text_font(state_label_, FontA.small, 0);

    // Beat counter
    beat_label_ = lv_label_create(display_container_);
    lv_label_set_text(beat_label_, "Beat: 0");
    lv_obj_set_style_text_color(beat_label_, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(beat_label_, FontA.small, 0);

    // Tick counter  
    tick_label_ = lv_label_create(display_container_);
    lv_label_set_text(tick_label_, "Tick: 0");
    lv_obj_set_style_text_color(tick_label_, lv_color_hex(0x999999), 0);
    lv_obj_set_style_text_font(tick_label_, FontA.small, 0);
}

void TransportControl::updateButtonStates() {
    if (!buttons_container_) return;

    // Update button visual states based on transport state
    switch (current_state_) {
        case MidiClockManager::TransportState::STOPPED:
            lv_obj_set_style_bg_color(play_btn_, lv_color_hex(0x00AA00), 0);  // Green
            lv_obj_set_style_bg_color(pause_btn_, lv_color_hex(0x666666), 0); // Dim
            lv_obj_set_style_bg_color(stop_btn_, lv_color_hex(0x666666), 0);  // Dim
            break;
            
        case MidiClockManager::TransportState::PLAYING:
            lv_obj_set_style_bg_color(play_btn_, lv_color_hex(0x00FF00), 0);  // Bright green
            lv_obj_set_style_bg_color(pause_btn_, lv_color_hex(0xFFAA00), 0); // Orange
            lv_obj_set_style_bg_color(stop_btn_, lv_color_hex(0xAA0000), 0);  // Red
            break;
            
        case MidiClockManager::TransportState::PAUSED:
            lv_obj_set_style_bg_color(play_btn_, lv_color_hex(0x00AA00), 0);  // Green
            lv_obj_set_style_bg_color(pause_btn_, lv_color_hex(0xFFFF00), 0); // Bright yellow
            lv_obj_set_style_bg_color(stop_btn_, lv_color_hex(0xAA0000), 0);  // Red
            break;
    }

    // Enable/disable buttons based on enabled state
    if (enabled_) {
        lv_obj_clear_state(play_btn_, LV_STATE_DISABLED);
        lv_obj_clear_state(pause_btn_, LV_STATE_DISABLED);
        lv_obj_clear_state(stop_btn_, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(play_btn_, LV_STATE_DISABLED);
        lv_obj_add_state(pause_btn_, LV_STATE_DISABLED);
        lv_obj_add_state(stop_btn_, LV_STATE_DISABLED);
    }
}

void TransportControl::updateTransportState(MidiClockManager::TransportState state) {
    current_state_ = state;
    updateButtonStates();

    if (state_label_) {
        const char* state_text = "UNKNOWN";
        uint32_t state_color = 0x999999;
        
        switch (state) {
            case MidiClockManager::TransportState::STOPPED:
                state_text = "STOPPED";
                state_color = 0xFF6666;
                break;
            case MidiClockManager::TransportState::PLAYING:
                state_text = "PLAYING";
                state_color = 0x66FF66;
                break;
            case MidiClockManager::TransportState::PAUSED:
                state_text = "PAUSED";
                state_color = 0xFFFF66;
                break;
        }
        
        lv_label_set_text(state_label_, state_text);
        lv_obj_set_style_text_color(state_label_, lv_color_hex(state_color), 0);
    }
}

void TransportControl::updateBPM(float bpm) {
    current_bpm_ = bpm;
    if (bpm_value_label_) {
        lv_label_set_text_fmt(bpm_value_label_, "%.1f", bpm);
    }
}

void TransportControl::updateTick(int tick_count) {
    current_tick_ = tick_count;
    
    if (tick_label_) {
        lv_label_set_text_fmt(tick_label_, "Tick: %d", tick_count);
    }
    
    if (beat_label_) {
        int beat = tick_count / 24; // Assuming 24 PPQN
        lv_label_set_text_fmt(beat_label_, "Beat: %d", beat);
    }
}

void TransportControl::setEnabled(bool enabled) {
    enabled_ = enabled;
    updateButtonStates();
}

// Button event handlers
void TransportControl::onPlayClicked() {
    if (!enabled_) return;
    std::cout << "TransportControl: Play clicked" << std::endl;
    
    auto& clock_manager = MidiClockManager::getInstance();
    clock_manager.play();
    
    if (transport_callback_) {
        transport_callback_(MidiClockManager::TransportState::PLAYING);
    }
}

void TransportControl::onPauseClicked() {
    if (!enabled_) return;
    std::cout << "TransportControl: Pause clicked" << std::endl;
    
    auto& clock_manager = MidiClockManager::getInstance();
    clock_manager.pause();
    
    if (transport_callback_) {
        transport_callback_(MidiClockManager::TransportState::PAUSED);
    }
}

void TransportControl::onStopClicked() {
    if (!enabled_) return;
    std::cout << "TransportControl: Stop clicked" << std::endl;
    
    auto& clock_manager = MidiClockManager::getInstance();
    clock_manager.stop();
    
    if (transport_callback_) {
        transport_callback_(MidiClockManager::TransportState::STOPPED);
    }
}

// Static event handlers
void TransportControl::onPlayButtonClicked(lv_event_t* e) {
    TransportControl* control = static_cast<TransportControl*>(lv_event_get_user_data(e));
    if (control) {
        control->onPlayClicked();
    }
}

void TransportControl::onPauseButtonClicked(lv_event_t* e) {
    TransportControl* control = static_cast<TransportControl*>(lv_event_get_user_data(e));
    if (control) {
        control->onPauseClicked();
    }
}

void TransportControl::onStopButtonClicked(lv_event_t* e) {
    TransportControl* control = static_cast<TransportControl*>(lv_event_get_user_data(e));
    if (control) {
        control->onStopClicked();
    }
}
