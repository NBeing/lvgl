#pragma once

#include "UnifiedMidiManager.h"

#ifdef ESP32_BUILD
#include <Control_Surface.h>  // Control Surface library has ESP32 USB MIDI support
#endif

/**
 * @brief ESP32 USB MIDI Backend using Control Surface library
 * 
 * Provides USB MIDI device functionality for ESP32.
 * Uses Control Surface library's USBMIDI_Interface for ESP32.
 */
class ESP32USBMidiBackend : public UnifiedMidiManager::MidiBackend {
public:
    ESP32USBMidiBackend();
    ~ESP32USBMidiBackend() override;
    
    // MidiBackend interface
    bool initialize() override;
    void cleanup() override;
    UnifiedMidiManager::ConnectionStatus getStatus() const override;
    bool supportsInput() const override { return true; }  // USB MIDI supports both directions
    bool supportsOutput() const override { return true; }
    void sendMessage(uint8_t status, uint8_t data1, uint8_t data2) override;
    void sendMessage(uint8_t status) override;
    void update() override;
    uint32_t getMessagesSent() const override { return messages_sent_; }
    uint32_t getMessagesReceived() const override { return messages_received_; }
    std::string getName() const override { return "ESP32 USB MIDI"; }
    UnifiedMidiManager::BackendType getType() const override { return UnifiedMidiManager::BackendType::USB_MIDI; }
    
private:
    void handleIncomingMessage(uint8_t status, uint8_t data1, uint8_t data2);
    
#ifdef ESP32_BUILD
    USBMIDI_Interface* usb_midi_ = nullptr;
#endif
    
    bool initialized_ = false;
    uint32_t messages_sent_ = 0;
    uint32_t messages_received_ = 0;
    UnifiedMidiManager::ConnectionStatus status_ = UnifiedMidiManager::ConnectionStatus::DISCONNECTED;
};
