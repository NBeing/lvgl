#pragma once

#include <functional>
#include <chrono>
#include <memory>

/**
 * @brief MIDI Clock and Transport Manager
 * 
 * Handles MIDI clock generation, transport controls (play/pause/stop),
 * and synchronization with external devices.
 */
class MidiClockManager {
public:
    enum class TransportState {
        STOPPED,
        PLAYING,
        PAUSED
    };

    enum class ClockMode {
        INTERNAL,    // Generate clock internally
        EXTERNAL,    // Sync to external clock
        OFF          // No clock sync (renamed from DISABLED to avoid ESP32 macro conflict)
    };

    enum class SyncSource {
        INTERNAL,           // Internal software clock
        USB_MIDI,          // USB MIDI input clock
        HARDWARE_MIDI,     // Hardware MIDI input clock  
        SOFTWARE,          // Software/DAW clock (virtual ports)
        AUTO_DETECT        // Automatically detect active source
    };

    struct ClockSettings {
        int ppqn = 24;              // Pulses Per Quarter Note (standard = 24)
        float bpm = 120.0f;         // Beats Per Minute
        ClockMode mode = ClockMode::INTERNAL;
        SyncSource sync_source = SyncSource::INTERNAL;
        bool send_clock = true;     // Send MIDI clock messages
        bool send_transport = true; // Send start/stop/continue messages
        bool receive_clock = false; // Respond to incoming clock
        bool receive_transport = false; // Respond to transport messages
        bool auto_detect_source = true; // Auto-switch to active clock source
    };

    using TransportChangedCallback = std::function<void(TransportState old_state, TransportState new_state)>;
    using ClockTickCallback = std::function<void(int tick_count)>;
    using BPMChangedCallback = std::function<void(float new_bpm)>;

    // Singleton pattern for global access
    static MidiClockManager& getInstance();

    // Transport control
    void play();
    void pause();
    void stop();
    void continue_playback();
    TransportState getTransportState() const { return transport_state_; }

    // Clock settings
    void setClockSettings(const ClockSettings& settings);
    const ClockSettings& getClockSettings() const { return settings_; }
    
    // BPM control
    void setBPM(float bpm);
    float getBPM() const { return settings_.bpm; }
    
    // PPQN control
    void setPPQN(int ppqn);
    int getPPQN() const { return settings_.ppqn; }

    // Clock mode
    void setClockMode(ClockMode mode);
    ClockMode getClockMode() const { return settings_.mode; }

    // Sync source control
    void setSyncSource(SyncSource source);
    SyncSource getSyncSource() const { return settings_.sync_source; }
    SyncSource getActiveSyncSource() const { return active_sync_source_; }
    const char* getSyncSourceName(SyncSource source) const;
    
    // Auto-detection
    void setAutoDetectSource(bool enable) { settings_.auto_detect_source = enable; }
    bool isAutoDetectEnabled() const { return settings_.auto_detect_source; }

    // Update loop (call from main loop)
    void update();

    // Event callbacks
    void setTransportChangedCallback(TransportChangedCallback callback) { transport_callback_ = callback; }
    void setClockTickCallback(ClockTickCallback callback) { clock_callback_ = callback; }
    void setBPMChangedCallback(BPMChangedCallback callback) { bpm_callback_ = callback; }

    // MIDI message handling (called by MidiHandler)
    void handleMidiClockMessage(SyncSource source = SyncSource::AUTO_DETECT);
    void handleMidiStartMessage(SyncSource source = SyncSource::AUTO_DETECT);
    void handleMidiStopMessage(SyncSource source = SyncSource::AUTO_DETECT);
    void handleMidiContinueMessage(SyncSource source = SyncSource::AUTO_DETECT);

    // Status
    bool isRunning() const { return transport_state_ == TransportState::PLAYING; }
    int getCurrentTick() const { return current_tick_; }
    int getCurrentBeat() const { return current_tick_ / settings_.ppqn; }
    
    // Calculate timing
    double getTickIntervalMs() const;
    double getBeatIntervalMs() const { return getTickIntervalMs() * settings_.ppqn; }

private:
    MidiClockManager() = default;
    ~MidiClockManager() = default;
    MidiClockManager(const MidiClockManager&) = delete;
    MidiClockManager& operator=(const MidiClockManager&) = delete;

    void sendMidiClock();
    void sendMidiStart();
    void sendMidiStop();
    void sendMidiContinue();
    
    void notifyTransportChanged(TransportState old_state, TransportState new_state);
    void notifyClockTick();
    void notifyBPMChanged();
    
    // Sync source detection and management
    void detectActiveSyncSource();
    void updateSyncSourceActivity(SyncSource source);
    bool isSyncSourceActive(SyncSource source) const;
    void resetSyncSourceActivity();

    // State
    ClockSettings settings_;
    TransportState transport_state_ = TransportState::STOPPED;
    SyncSource active_sync_source_ = SyncSource::INTERNAL;
    
    // Timing
    std::chrono::steady_clock::time_point last_tick_time_;
    int current_tick_ = 0;
    bool first_tick_ = true;
    
    // External sync tracking
    std::chrono::steady_clock::time_point last_external_clock_;
    std::chrono::steady_clock::time_point last_usb_clock_;
    std::chrono::steady_clock::time_point last_hardware_clock_;
    std::chrono::steady_clock::time_point last_software_clock_;
    float detected_bpm_ = 120.0f;
    int external_tick_count_ = 0;
    
    // Source activity tracking
    bool usb_midi_active_ = false;
    bool hardware_midi_active_ = false;
    bool software_midi_active_ = false;

    // Callbacks
    TransportChangedCallback transport_callback_;
    ClockTickCallback clock_callback_;
    BPMChangedCallback bpm_callback_;
};
