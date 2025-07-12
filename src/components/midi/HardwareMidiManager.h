#pragma once

#ifdef ESP32_BUILD
#include <HardwareSerial.h>
#endif

#include <cstdint>
#include <functional>

/**
 * @brief Hardware MIDI Manager for ESP32
 * 
 * Provides direct hardware MIDI output via UART on configurable pins.
 * Integrates with the MIDI Clock Manager for real-time clock and transport.
 */
class HardwareMidiManager {
public:
    static HardwareMidiManager& getInstance();
    
    // Hardware setup
    void initialize(int tx_pin = 3, int rx_pin = -1); // Default: TX only on pin 3
    void cleanup();
    
    // MIDI Output - Channel Voice Messages
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
    void sendControlChange(uint8_t channel, uint8_t cc, uint8_t value);
    void sendProgramChange(uint8_t channel, uint8_t program);
    void sendPitchBend(uint8_t channel, uint16_t value);
    void sendAftertouch(uint8_t channel, uint8_t pressure);
    void sendPolyAftertouch(uint8_t channel, uint8_t note, uint8_t pressure);
    
    // MIDI Clock & Transport - System Real-Time Messages
    void sendClockPulse();
    void sendStart();
    void sendStop();
    void sendContinue();
    
    // System Messages
    void sendSystemReset();
    void sendActiveSensing();
    void sendAllNotesOff(uint8_t channel);
    void sendAllSoundOff(uint8_t channel);
    
    // Configuration
    void setActiveSensingEnabled(bool enabled) { active_sensing_enabled_ = enabled; }
    bool isActiveSensingEnabled() const { return active_sensing_enabled_; }
    
    // Status
    bool isInitialized() const { return initialized_; }
    int getTxPin() const { return tx_pin_; }
    int getRxPin() const { return rx_pin_; }
    
    // Statistics
    uint32_t getMessagesSent() const { return messages_sent_; }
    uint32_t getClockPulsesSent() const { return clock_pulses_sent_; }
    void resetStatistics();

private:
    HardwareMidiManager() = default;
    ~HardwareMidiManager() = default;
    HardwareMidiManager(const HardwareMidiManager&) = delete;
    HardwareMidiManager& operator=(const HardwareMidiManager&) = delete;
    
    // Low-level MIDI transmission
    void sendMidiMessage(uint8_t status, uint8_t data1, uint8_t data2);
    void sendMidiMessage(uint8_t status, uint8_t data1);
    void sendMidiMessage(uint8_t status); // For single-byte messages
    
    // Utility
    void logMidiMessage(const char* type, uint8_t status, uint8_t data1 = 0, uint8_t data2 = 0);

#ifdef ESP32_BUILD
    HardwareSerial* midi_serial_ = nullptr;
#endif
    
    // State
    bool initialized_ = false;
    int tx_pin_ = 3;
    int rx_pin_ = -1;
    bool active_sensing_enabled_ = false;
    
    // Statistics
    uint32_t messages_sent_ = 0;
    uint32_t clock_pulses_sent_ = 0;
};
