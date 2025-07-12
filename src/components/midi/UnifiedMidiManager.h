#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <functional>
#include <string>

/**
 * @brief Unified MIDI Manager - handles both hardware and USB MIDI
 * 
 * This class abstracts MIDI I/O across different backends:
 * - Desktop: RtMidi (USB MIDI)
 * - ESP32: Hardware Serial MIDI
 * - Future: Bluetooth MIDI, Network MIDI, etc.
 */
class UnifiedMidiManager {
public:
    // MIDI message callback type
    using MidiMessageCallback = std::function<void(uint8_t status, uint8_t data1, uint8_t data2)>;
    
    // Connection status
    enum class ConnectionStatus {
        DISCONNECTED,
        CONNECTED,
        ERROR
    };
    
    // Backend types
    enum class BackendType {
        RTMIDI,      // Desktop USB MIDI
        HARDWARE,    // ESP32 Serial MIDI
        USB_MIDI,    // ESP32 USB MIDI device
        BLUETOOTH,   // Future: BLE MIDI
        NETWORK      // Future: Network MIDI
    };
    
    // Backend info
    struct BackendInfo {
        BackendType type;
        std::string name;
        ConnectionStatus status;
        bool supports_input;
        bool supports_output;
        uint32_t messages_sent;
        uint32_t messages_received;
    };
    
    static UnifiedMidiManager& getInstance();
    
    // Set shared MIDI handler (for desktop RtMidi backend)
    static void setSharedMidiHandler(std::shared_ptr<class MidiHandler> handler);
    
    // Initialization
    void initialize();
    void cleanup();
    
    // Backend management
    std::vector<BackendInfo> getAvailableBackends() const;
    bool enableBackend(BackendType type);
    bool disableBackend(BackendType type);
    bool isBackendEnabled(BackendType type) const;
    
    // MIDI Output (to all enabled backends)
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
    void sendControlChange(uint8_t channel, uint8_t cc, uint8_t value);
    void sendProgramChange(uint8_t channel, uint8_t program);
    void sendPitchBend(uint8_t channel, uint16_t value);
    
    // MIDI Clock & Transport (to all enabled backends)
    void sendClockPulse();
    void sendStart();
    void sendStop();
    void sendContinue();
    
    // System messages
    void sendSystemReset();
    void sendActiveSensing();
    
    // MIDI Input callbacks
    void setMidiMessageCallback(MidiMessageCallback callback);
    
    // Status and statistics
    ConnectionStatus getOverallStatus() const;
    uint32_t getTotalMessagesSent() const;
    uint32_t getTotalMessagesReceived() const;
    
    // Legacy compatibility
    bool isConnected() const { return getOverallStatus() == ConnectionStatus::CONNECTED; }
    
    // Update loop (call from main loop)
    void update();
    
    // Backend interface (public so backends can inherit from it)
    class MidiBackend {
    public:
        virtual ~MidiBackend() = default;
        virtual bool initialize() = 0;
        virtual void cleanup() = 0;
        virtual ConnectionStatus getStatus() const = 0;
        virtual bool supportsInput() const = 0;
        virtual bool supportsOutput() const = 0;
        virtual void sendMessage(uint8_t status, uint8_t data1, uint8_t data2) = 0;
        virtual void sendMessage(uint8_t status) = 0;
        virtual void update() = 0;
        virtual uint32_t getMessagesSent() const = 0;
        virtual uint32_t getMessagesReceived() const = 0;
        virtual std::string getName() const = 0;
        virtual BackendType getType() const = 0;
    };

private:
    UnifiedMidiManager() = default;
    ~UnifiedMidiManager() = default;
    UnifiedMidiManager(const UnifiedMidiManager&) = delete;
    UnifiedMidiManager& operator=(const UnifiedMidiManager&) = delete;
    
    // Backend implementations
    std::vector<std::unique_ptr<MidiBackend>> backends_;
    MidiMessageCallback message_callback_;
    bool initialized_ = false;
    
    // Helper methods
    void createBackends();
    MidiBackend* getBackend(BackendType type) const;
};
