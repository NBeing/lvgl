#include "ClockTab.h"
#include "Constants.h"
#include "FontConfig.h"
#include "components/settings/SettingsManager.h"
#include "components/midi/HardwareMidiManager.h"
#include <iostream>

ClockTab::ClockTab()
    : Tab("Clock")
    , controls_container_(nullptr)
    , tempo_container_(nullptr)
    , status_container_(nullptr)
    , settings_container_(nullptr)
    , tempo_up_btn_(nullptr)
    , tempo_down_btn_(nullptr)
    , tempo_display_(nullptr)
    , ppqn_dropdown_(nullptr)
    , clock_mode_dropdown_(nullptr)
    , midi_test_btn_(nullptr)
    , clock_status_label_(nullptr)
    , tick_rate_label_(nullptr)
    , beat_indicator_(nullptr)
    , sync_status_label_(nullptr)
    , beat_led_(nullptr)
    , beat_led_state_(false)
    , last_beat_(0)
    , settings_observer_registered_(false)
{
}

void ClockTab::create(lv_obj_t* parent) {
    if (container_) return; // Already created

    // Create main container for this tab
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(98), LV_PCT(98));
    lv_obj_set_pos(container_, 0, 0);
    lv_obj_set_style_bg_color(container_, lv_color_hex(SynthConstants::Color::BG), 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 8, 0);

    // Set up main layout
    lv_obj_set_layout(container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(container_, 10, 0);

    setContainer(container_);

    // Create title
    lv_obj_t* title = lv_label_create(container_);
    lv_label_set_text(title, "MIDI Clock & Transport");
    lv_obj_set_style_text_color(title, lv_color_hex(SynthConstants::Color::TITLE), 0);
    lv_obj_set_style_text_font(title, FontA.med, 0);

    // Create subtitle
    lv_obj_t* subtitle = lv_label_create(container_);
    lv_label_set_text(subtitle, "Test MIDI clock generation, transport controls, and timing synchronization");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(SynthConstants::Color::HELP), 0);
    lv_obj_set_style_text_font(subtitle, FontA.small, 0);

    createClockControls();
    createTempoControls();
    createStatusDisplay();
    createSettingsPanel();

    // Register with clock manager for callbacks
    auto& clock_manager = MidiClockManager::getInstance();
    clock_manager.setTransportChangedCallback([this](auto old_state, auto new_state) {
        onTransportChanged(new_state);
    });
    clock_manager.setClockTickCallback([this](int tick) {
        onClockTick(tick);
    });
    clock_manager.setBPMChangedCallback([this](float bpm) {
        onBPMChanged(bpm);
    });

    // Register with settings manager
    SettingsManager::getInstance().addObserver("ClockTab",
        [this](const std::string& key, const std::any& old_val, const std::any& new_val) {
            onSettingChanged(key);
        });
    settings_observer_registered_ = true;

    // Sync initial settings
    syncSettingsToClockManager();

    std::cout << "ClockTab created with MIDI clock testing interface" << std::endl;
}

void ClockTab::createClockControls() {
    // Create transport control section
    controls_container_ = lv_obj_create(container_);
    lv_obj_set_size(controls_container_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(controls_container_, lv_color_hex(0x1E1E2E), 0);
    lv_obj_set_style_border_color(controls_container_, lv_color_hex(0x6C7086), 0);
    lv_obj_set_style_border_width(controls_container_, 1, 0);
    lv_obj_set_style_radius(controls_container_, 8, 0);
    lv_obj_set_style_pad_all(controls_container_, 10, 0);

    // Section title
    lv_obj_t* controls_title = lv_label_create(controls_container_);
    lv_label_set_text(controls_title, "Transport Controls");
    lv_obj_set_style_text_color(controls_title, lv_color_hex(0xCDD6F4), 0);
    lv_obj_set_style_text_font(controls_title, FontA.small, 0);

    // Create transport control component
    transport_control_ = std::make_unique<TransportControl>();
    transport_control_->create(controls_container_);
    transport_control_->setTransportChangedCallback([this](auto state) {
        onTransportChanged(state);
    });
}

void ClockTab::createTempoControls() {
    // Create tempo control section
    tempo_container_ = lv_obj_create(container_);
    lv_obj_set_size(tempo_container_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(tempo_container_, lv_color_hex(0x1E1E2E), 0);
    lv_obj_set_style_border_color(tempo_container_, lv_color_hex(0x6C7086), 0);
    lv_obj_set_style_border_width(tempo_container_, 1, 0);
    lv_obj_set_style_radius(tempo_container_, 8, 0);
    lv_obj_set_style_pad_all(tempo_container_, 10, 0);

    // Set up flex layout
    lv_obj_set_layout(tempo_container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tempo_container_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tempo_container_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Section title
    lv_obj_t* tempo_title = lv_label_create(tempo_container_);
    lv_label_set_text(tempo_title, "Tempo Control");
    lv_obj_set_style_text_color(tempo_title, lv_color_hex(0xCDD6F4), 0);
    lv_obj_set_style_text_font(tempo_title, FontA.small, 0);

    // Tempo adjustment buttons
    lv_obj_t* tempo_controls = lv_obj_create(tempo_container_);
    lv_obj_set_size(tempo_controls, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(tempo_controls, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tempo_controls, 0, 0);
    lv_obj_set_style_pad_all(tempo_controls, 5, 0);

    lv_obj_set_layout(tempo_controls, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tempo_controls, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tempo_controls, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(tempo_controls, 5, 0);

    // Tempo down button
    tempo_down_btn_ = lv_btn_create(tempo_controls);
    lv_obj_set_size(tempo_down_btn_, 40, 40);
    lv_obj_set_style_bg_color(tempo_down_btn_, lv_color_hex(0x0066CC), 0);
    lv_obj_add_event_cb(tempo_down_btn_, onTempoDownButtonClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* down_label = lv_label_create(tempo_down_btn_);
    lv_label_set_text(down_label, "-");
    lv_obj_set_style_text_font(down_label, FontA.med, 0);
    lv_obj_center(down_label);

    // Tempo display
    tempo_display_ = lv_label_create(tempo_controls);
    lv_label_set_text(tempo_display_, "120.0 BPM");
    lv_obj_set_style_text_color(tempo_display_, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(tempo_display_, FontA.med, 0);

    // Tempo up button
    tempo_up_btn_ = lv_btn_create(tempo_controls);
    lv_obj_set_size(tempo_up_btn_, 40, 40);
    lv_obj_set_style_bg_color(tempo_up_btn_, lv_color_hex(0x0066CC), 0);
    lv_obj_add_event_cb(tempo_up_btn_, onTempoUpButtonClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* up_label = lv_label_create(tempo_up_btn_);
    lv_label_set_text(up_label, "+");
    lv_obj_set_style_text_font(up_label, FontA.med, 0);
    lv_obj_center(up_label);
}

void ClockTab::createStatusDisplay() {
    // Create status display section
    status_container_ = lv_obj_create(container_);
    lv_obj_set_size(status_container_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(status_container_, lv_color_hex(0x1E1E2E), 0);
    lv_obj_set_style_border_color(status_container_, lv_color_hex(0x6C7086), 0);
    lv_obj_set_style_border_width(status_container_, 1, 0);
    lv_obj_set_style_radius(status_container_, 8, 0);
    lv_obj_set_style_pad_all(status_container_, 10, 0);

    // Set up flex layout
    lv_obj_set_layout(status_container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_container_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_container_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Section title
    lv_obj_t* status_title = lv_label_create(status_container_);
    lv_label_set_text(status_title, "Clock Status");
    lv_obj_set_style_text_color(status_title, lv_color_hex(0xCDD6F4), 0);
    lv_obj_set_style_text_font(status_title, FontA.small, 0);

    // Beat indicator (LED-like)
    beat_led_ = lv_obj_create(status_container_);
    lv_obj_set_size(beat_led_, 20, 20);
    lv_obj_set_style_bg_color(beat_led_, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_color(beat_led_, lv_color_hex(0x666666), 0);
    lv_obj_set_style_border_width(beat_led_, 2, 0);
    lv_obj_set_style_radius(beat_led_, 10, 0); // Round LED

    // Status labels
    lv_obj_t* status_labels = lv_obj_create(status_container_);
    lv_obj_set_size(status_labels, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(status_labels, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status_labels, 0, 0);
    lv_obj_set_style_pad_all(status_labels, 5, 0);

    lv_obj_set_layout(status_labels, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_labels, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(status_labels, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    clock_status_label_ = lv_label_create(status_labels);
    lv_label_set_text(clock_status_label_, "Mode: Internal");
    lv_obj_set_style_text_color(clock_status_label_, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(clock_status_label_, FontA.small, 0);

    tick_rate_label_ = lv_label_create(status_labels);
    lv_label_set_text(tick_rate_label_, "PPQN: 24");
    lv_obj_set_style_text_color(tick_rate_label_, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(tick_rate_label_, FontA.small, 0);

    sync_status_label_ = lv_label_create(status_labels);
    lv_label_set_text(sync_status_label_, "Sync: Ready");
    lv_obj_set_style_text_color(sync_status_label_, lv_color_hex(0x66FF66), 0);
    lv_obj_set_style_text_font(sync_status_label_, FontA.small, 0);
}

void ClockTab::createSettingsPanel() {
    // Create settings quick access section
    settings_container_ = lv_obj_create(container_);
    lv_obj_set_size(settings_container_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(settings_container_, lv_color_hex(0x1E1E2E), 0);
    lv_obj_set_style_border_color(settings_container_, lv_color_hex(0x6C7086), 0);
    lv_obj_set_style_border_width(settings_container_, 1, 0);
    lv_obj_set_style_radius(settings_container_, 8, 0);
    lv_obj_set_style_pad_all(settings_container_, 10, 0);

    // Section title
    lv_obj_t* settings_title = lv_label_create(settings_container_);
    lv_label_set_text(settings_title, "Quick Settings");
    lv_obj_set_style_text_color(settings_title, lv_color_hex(0xCDD6F4), 0);
    lv_obj_set_style_text_font(settings_title, FontA.small, 0);

    // Note: For now, just display current settings
    // TODO: Add quick setting controls if needed
    lv_obj_t* settings_note = lv_label_create(settings_container_);
    lv_label_set_text(settings_note, "Use the Settings tab to configure MIDI clock options");
    lv_obj_set_style_text_color(settings_note, lv_color_hex(0x9399B2), 0);
    lv_obj_set_style_text_font(settings_note, FontA.small, 0);

    // Add MIDI test button
    midi_test_btn_ = lv_btn_create(settings_container_);
    lv_obj_set_size(midi_test_btn_, 120, 40);
    lv_obj_set_style_bg_color(midi_test_btn_, lv_color_hex(0xFF6B35), 0);
    lv_obj_set_style_border_color(midi_test_btn_, lv_color_hex(0xFF8A50), 0);
    lv_obj_set_style_border_width(midi_test_btn_, 2, 0);
    lv_obj_set_style_radius(midi_test_btn_, 6, 0);
    lv_obj_add_event_cb(midi_test_btn_, onMidiTestButtonClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* test_label = lv_label_create(midi_test_btn_);
    lv_label_set_text(test_label, "Test MIDI");
    lv_obj_set_style_text_color(test_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(test_label, FontA.small, 0);
    lv_obj_center(test_label);
}

void ClockTab::syncSettingsToClockManager() {
    auto& settings = SettingsManager::getInstance();
    auto& clock_manager = MidiClockManager::getInstance();

    MidiClockManager::ClockSettings clock_settings;
    
    // Get BPM from clock manager (might have been changed directly)
    clock_settings.bpm = clock_manager.getBPM();
    
    // PPQN from settings
    int ppqn_index = settings.getListSelection("midi.ppqn");
    switch (ppqn_index) {
        case 0: clock_settings.ppqn = 12; break;
        case 1: clock_settings.ppqn = 24; break;
        case 2: clock_settings.ppqn = 48; break;
        case 3: clock_settings.ppqn = 96; break;
        default: clock_settings.ppqn = 24; break;
    }
    
    // Clock mode from settings
    int mode_index = settings.getListSelection("midi.clock_mode");
    switch (mode_index) {
        case 0: clock_settings.mode = MidiClockManager::ClockMode::INTERNAL; break;
        case 1: clock_settings.mode = MidiClockManager::ClockMode::EXTERNAL; break;
        case 2: clock_settings.mode = MidiClockManager::ClockMode::OFF; break;
        default: clock_settings.mode = MidiClockManager::ClockMode::INTERNAL; break;
    }
    
    // Transport settings
    clock_settings.send_clock = settings.getBool("midi.send_clock");
    clock_settings.send_transport = settings.getBool("midi.send_transport");
    clock_settings.receive_clock = settings.getBool("midi.receive_clock");
    clock_settings.receive_transport = settings.getBool("midi.receive_transport");
    
    clock_manager.setClockSettings(clock_settings);
    
    updateClockDisplay();
    updateTempoDisplay();
    updateStatusDisplay();
}

void ClockTab::onTransportChanged(MidiClockManager::TransportState state) {
    if (transport_control_) {
        transport_control_->updateTransportState(state);
    }
    updateStatusDisplay();
}

void ClockTab::onClockTick(int tick_count) {
    if (transport_control_) {
        transport_control_->updateTick(tick_count);
    }
    
    // Update beat indicator
    int current_beat = tick_count / 24; // Assuming 24 PPQN
    if (current_beat != last_beat_) {
        last_beat_ = current_beat;
        beat_led_state_ = !beat_led_state_;
        
        if (beat_led_) {
            uint32_t color = beat_led_state_ ? 0x00FF00 : 0x333333;
            lv_obj_set_style_bg_color(beat_led_, lv_color_hex(color), 0);
        }
    }
}

void ClockTab::onBPMChanged(float new_bpm) {
    if (transport_control_) {
        transport_control_->updateBPM(new_bpm);
    }
    updateTempoDisplay();
}

void ClockTab::onTempoUpClicked() {
    auto& clock_manager = MidiClockManager::getInstance();
    float current_bpm = clock_manager.getBPM();
    clock_manager.setBPM(current_bpm + 1.0f);
}

void ClockTab::onTempoDownClicked() {
    auto& clock_manager = MidiClockManager::getInstance();
    float current_bpm = clock_manager.getBPM();
    clock_manager.setBPM(current_bpm - 1.0f);
}

void ClockTab::onSettingChanged(const std::string& key) {
    // Sync relevant MIDI settings changes to clock manager
    if (key.find("midi.") == 0) {
        syncSettingsToClockManager();
    }
}

void ClockTab::updateClockDisplay() {
    // Called when clock manager settings change
}

void ClockTab::updateTempoDisplay() {
    if (tempo_display_) {
        auto& clock_manager = MidiClockManager::getInstance();
        lv_label_set_text_fmt(tempo_display_, "%.1f BPM", clock_manager.getBPM());
    }
}

void ClockTab::updateStatusDisplay() {
    auto& clock_manager = MidiClockManager::getInstance();
    
    if (clock_status_label_) {
        const char* mode_text = "Unknown";
        switch (clock_manager.getClockMode()) {
            case MidiClockManager::ClockMode::INTERNAL: mode_text = "Internal"; break;
            case MidiClockManager::ClockMode::EXTERNAL: mode_text = "External"; break;
            case MidiClockManager::ClockMode::OFF: mode_text = "Off"; break;
        }
        lv_label_set_text_fmt(clock_status_label_, "Mode: %s", mode_text);
    }
    
    if (tick_rate_label_) {
        lv_label_set_text_fmt(tick_rate_label_, "PPQN: %d", clock_manager.getPPQN());
    }
    
    if (sync_status_label_) {
        const char* status_text = "Ready";
        uint32_t status_color = 0x66FF66;
        
        if (clock_manager.isRunning()) {
            status_text = "Running";
            status_color = 0x00FF00;
        } else if (clock_manager.getTransportState() == MidiClockManager::TransportState::PAUSED) {
            status_text = "Paused";
            status_color = 0xFFFF00;
        }
        
        lv_label_set_text_fmt(sync_status_label_, "Sync: %s", status_text);
        lv_obj_set_style_text_color(sync_status_label_, lv_color_hex(status_color), 0);
    }
}

void ClockTab::onMidiTestClicked() {
    std::cout << "ClockTab: MIDI Test button clicked!" << std::endl;
    
    auto& hw_midi = HardwareMidiManager::getInstance();
    
    if (!hw_midi.isInitialized()) {
        std::cout << "Hardware MIDI not initialized" << std::endl;
        return;
    }
    
    std::cout << "=== Hardware MIDI Test Sequence ===" << std::endl;
    
    // Test note sequence - play a little melody
    hw_midi.sendNoteOn(1, 60, 100);   // C4
    hw_midi.sendNoteOn(1, 64, 100);   // E4  
    hw_midi.sendNoteOn(1, 67, 100);   // G4
    
    // Test control changes
    hw_midi.sendControlChange(1, 7, 127);   // Volume full
    hw_midi.sendControlChange(1, 10, 64);   // Pan center
    hw_midi.sendControlChange(1, 74, 100);  // Filter cutoff
    
    // Test program change
    hw_midi.sendProgramChange(1, 1);  // Piano sound
    
    // Turn notes off
    hw_midi.sendNoteOff(1, 60, 0);
    hw_midi.sendNoteOff(1, 64, 0);
    hw_midi.sendNoteOff(1, 67, 0);
    
    std::cout << "MIDI test complete - Messages: " << hw_midi.getMessagesSent() 
              << ", Clock pulses: " << hw_midi.getClockPulsesSent() << std::endl;
}

void ClockTab::onActivated() {
    std::cout << "ClockTab activated - MIDI clock testing ready" << std::endl;
    syncSettingsToClockManager();
}

void ClockTab::onDeactivated() {
    std::cout << "ClockTab deactivated" << std::endl;
}

// Static event handlers
void ClockTab::onTempoUpButtonClicked(lv_event_t* e) {
    ClockTab* tab = static_cast<ClockTab*>(lv_event_get_user_data(e));
    if (tab) {
        tab->onTempoUpClicked();
    }
}

void ClockTab::onTempoDownButtonClicked(lv_event_t* e) {
    ClockTab* tab = static_cast<ClockTab*>(lv_event_get_user_data(e));
    if (tab) {
        tab->onTempoDownClicked();
    }
}

void ClockTab::onMidiTestButtonClicked(lv_event_t* e) {
    ClockTab* tab = static_cast<ClockTab*>(lv_event_get_user_data(e));
    if (tab) {
        tab->onMidiTestClicked();
    }
}
