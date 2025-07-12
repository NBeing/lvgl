#pragma once

#include "UnifiedMidiManager.h"

#if !defined(ESP32_BUILD)
#include "hardware/MidiHandler.h"
#include <memory>

/**
 * @brief RtMidi Backend for desktop USB MIDI
 */
class RtMidiBackend : public UnifiedMidiManager::MidiBackend {
public:
    RtMidiBackend();
    ~RtMidiBackend() override;
    
    // Set external MIDI handler (to avoid creating duplicate instances)
    void setMidiHandler(std::shared_ptr<MidiHandler> handler);
    
    // MidiBackend interface
    bool initialize() override;
    void cleanup() override;
    UnifiedMidiManager::ConnectionStatus getStatus() const override;
    bool supportsInput() const override { return true; }
    bool supportsOutput() const override { return true; }
    void sendMessage(uint8_t status, uint8_t data1, uint8_t data2) override;
    void sendMessage(uint8_t status) override;
    void update() override;
    uint32_t getMessagesSent() const override { return messages_sent_; }
    uint32_t getMessagesReceived() const override { return messages_received_; }
    std::string getName() const override { return "RtMidi (USB)"; }
    UnifiedMidiManager::BackendType getType() const override { return UnifiedMidiManager::BackendType::RTMIDI; }
    
private:
    std::shared_ptr<MidiHandler> midi_handler_;  // Shared MIDI handler
    uint32_t messages_sent_ = 0;
    uint32_t messages_received_ = 0;
    bool initialized_ = false;
};

#endif // !defined(ESP32_BUILD)
