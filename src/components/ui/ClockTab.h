#pragma once

#include "components/ui/Window.h"
#include "components/ui/TransportControl.h"
#include "components/midi/MidiClockManager.h"
#include <memory>

/**
 * @brief Clock test tab for MIDI clock and transport testing
 * 
 * This tab provides a comprehensive interface for testing MIDI clock
 * functionality, transport controls, and timing synchronization.
 */
class ClockTab : public Tab {
public:
    ClockTab();
    ~ClockTab() = default;

    // Tab interface
    void create(lv_obj_t* parent) override;
    void onActivated() override;
    void onDeactivated() override;

private:
    void createClockControls();
    void createTempoControls();
    void createStatusDisplay();
    void createSettingsPanel();
    
    void updateClockDisplay();
    void updateTempoDisplay();
    void updateStatusDisplay();

    // Event handlers
    void onTransportChanged(MidiClockManager::TransportState state);
    void onClockTick(int tick_count);
    void onBPMChanged(float new_bpm);
    void onTempoUpClicked();
    void onTempoDownClicked();
    void onPPQNChanged(int ppqn_index);
    void onClockModeChanged(int mode_index);
    void onMidiTestClicked();

    // Settings sync
    void syncSettingsToClockManager();
    void onSettingChanged(const std::string& key);

    // Static event callbacks
    static void onTempoUpButtonClicked(lv_event_t* e);
    static void onTempoDownButtonClicked(lv_event_t* e);
    static void onMidiTestButtonClicked(lv_event_t* e);

    // UI Elements
    lv_obj_t* controls_container_;
    lv_obj_t* tempo_container_;
    lv_obj_t* status_container_;
    lv_obj_t* settings_container_;

    // Transport control
    std::unique_ptr<TransportControl> transport_control_;

    // Tempo controls
    lv_obj_t* tempo_up_btn_;
    lv_obj_t* tempo_down_btn_;
    lv_obj_t* tempo_display_;
    lv_obj_t* ppqn_dropdown_;
    lv_obj_t* clock_mode_dropdown_;
    lv_obj_t* midi_test_btn_;

    // Status displays
    lv_obj_t* clock_status_label_;
    lv_obj_t* tick_rate_label_;
    lv_obj_t* beat_indicator_;
    lv_obj_t* sync_status_label_;

    // Visual beat indicator
    lv_obj_t* beat_led_;
    bool beat_led_state_;
    int last_beat_;

    // Settings observer
    bool settings_observer_registered_;
};
