#pragma once

#include <lvgl.h>
#include <functional>
#include "components/midi/MidiClockManager.h"

/**
 * @brief Transport Control UI Component
 * 
 * Provides play/pause/stop buttons and tempo display for MIDI transport control
 */
class TransportControl {
public:
    using TransportChangedCallback = std::function<void(MidiClockManager::TransportState state)>;

    TransportControl();
    ~TransportControl() = default;

    // UI Management
    void create(lv_obj_t* parent);
    void destroy();
    bool isCreated() const { return container_ != nullptr; }

    // Event callbacks
    void setTransportChangedCallback(TransportChangedCallback callback) { transport_callback_ = callback; }

    // Update from clock manager
    void updateTransportState(MidiClockManager::TransportState state);
    void updateBPM(float bpm);
    void updateTick(int tick_count);

    // Manual control
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

private:
    void createButtons();
    void createTempoDisplay();
    void createTickDisplay();
    
    void updateButtonStates();
    void onPlayClicked();
    void onPauseClicked();
    void onStopClicked();

    // Static event handlers
    static void onPlayButtonClicked(lv_event_t* e);
    static void onPauseButtonClicked(lv_event_t* e);
    static void onStopButtonClicked(lv_event_t* e);

    // UI Elements
    lv_obj_t* container_;
    lv_obj_t* buttons_container_;
    lv_obj_t* display_container_;
    
    // Transport buttons
    lv_obj_t* play_btn_;
    lv_obj_t* pause_btn_;
    lv_obj_t* stop_btn_;
    
    // Status displays
    lv_obj_t* bpm_label_;
    lv_obj_t* bpm_value_label_;
    lv_obj_t* tick_label_;
    lv_obj_t* beat_label_;
    lv_obj_t* state_label_;

    // State
    MidiClockManager::TransportState current_state_;
    float current_bpm_;
    int current_tick_;
    bool enabled_;
    
    // Callback
    TransportChangedCallback transport_callback_;
};
