#pragma once

#include "UnifiedMidiManager.h"

#if defined(ESP32_BUILD)
#include <HardwareSerial.h>
#endif

/**
 * @brief Hardware Serial MIDI Backend for ESP32
 */
class HardwareMidiBackend : public UnifiedMidiManager::MidiBackend {
public:
    HardwareMidiBackend();
    ~HardwareMidiBackend() override;
    
    bool initialize() override;
    void cleanup() override;
    UnifiedMidiManager::ConnectionStatus getStatus() const override;
    bool supportsInput() const override { return rx_pin_ != -1; }
    bool supportsOutput() const override { return tx_pin_ != -1; }
    void sendMessage(uint8_t status, uint8_t data1, uint8_t data2) override;
    void sendMessage(uint8_t status) override;
    void update() override;
    uint32_t getMessagesSent() const override { return messages_sent_; }
    uint32_t getMessagesReceived() const override { return messages_received_; }
    std::string getName() const override;
    UnifiedMidiManager::BackendType getType() const override { return UnifiedMidiManager::BackendType::HARDWARE; }
    
    // Configuration
    void setPins(int tx_pin, int rx_pin = -1);
    
private:
#if defined(ESP32_BUILD)
    HardwareSerial* midi_serial_ = nullptr;
#endif
    
    int tx_pin_ = 3;
    int rx_pin_ = -1;
    uint32_t messages_sent_ = 0;
    uint32_t messages_received_ = 0;
    bool initialized_ = false;
    
    // MIDI input parsing state
    enum class MidiParseState {
        WAITING_FOR_STATUS,
        WAITING_FOR_DATA1,
        WAITING_FOR_DATA2
    };
    
    MidiParseState parse_state_ = MidiParseState::WAITING_FOR_STATUS;
    uint8_t current_status_ = 0;
    uint8_t current_data1_ = 0;
    uint8_t running_status_ = 0; // For MIDI running status
    
    void processMidiByte(uint8_t byte);
    void handleCompleteMessage(uint8_t status, uint8_t data1, uint8_t data2);
    bool isStatusByte(uint8_t byte) { return (byte & 0x80) != 0; }
    bool isDataByte(uint8_t byte) { return (byte & 0x80) == 0; }
    int getMessageLength(uint8_t status);
};
