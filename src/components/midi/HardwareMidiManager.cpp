#include "HardwareMidiManager.h"
#include <iostream>

HardwareMidiManager& HardwareMidiManager::getInstance() {
    static HardwareMidiManager instance;
    return instance;
}

void HardwareMidiManager::initialize(int tx_pin, int rx_pin) {
    tx_pin_ = tx_pin;
    rx_pin_ = rx_pin;
    
#ifdef ESP32_BUILD
    // Use UART1 for MIDI (UART0 is typically used for serial console)
    midi_serial_ = new HardwareSerial(1);
    
    // Standard MIDI baud rate: 31250 bps
    // 8 data bits, no parity, 1 stop bit
    // RX pin: -1 (disabled for TX-only), TX pin: user-specified
    midi_serial_->begin(31250, SERIAL_8N1, rx_pin_, tx_pin_);
    
    initialized_ = true;
    std::cout << "[ESP32] Hardware MIDI initialized - TX Pin: " << tx_pin_ 
              << ", RX Pin: " << (rx_pin_ == -1 ? "disabled" : std::to_string(rx_pin_)) 
              << ", Baud: 31250" << std::endl;
#else
    initialized_ = false;
    std::cout << "[Desktop] Hardware MIDI manager created - running in simulation mode" << std::endl;
    std::cout << "[Desktop] MIDI TX Pin: " << tx_pin_ << " (simulated)" << std::endl;
#endif

    // Reset statistics
    resetStatistics();
}

void HardwareMidiManager::cleanup() {
#ifdef ESP32_BUILD
    if (midi_serial_) {
        midi_serial_->end();
        delete midi_serial_;
        midi_serial_ = nullptr;
    }
    std::cout << "[ESP32] Hardware MIDI cleaned up" << std::endl;
#endif
    initialized_ = false;
}

// Channel Voice Messages
void HardwareMidiManager::sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    sendMidiMessage(0x90 | (channel & 0x0F), note & 0x7F, velocity & 0x7F);
    logMidiMessage("Note On", 0x90 | (channel & 0x0F), note, velocity);
}

void HardwareMidiManager::sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    sendMidiMessage(0x80 | (channel & 0x0F), note & 0x7F, velocity & 0x7F);
    logMidiMessage("Note Off", 0x80 | (channel & 0x0F), note, velocity);
}

void HardwareMidiManager::sendControlChange(uint8_t channel, uint8_t cc, uint8_t value) {
    sendMidiMessage(0xB0 | (channel & 0x0F), cc & 0x7F, value & 0x7F);
    logMidiMessage("CC", 0xB0 | (channel & 0x0F), cc, value);
}

void HardwareMidiManager::sendProgramChange(uint8_t channel, uint8_t program) {
    sendMidiMessage(0xC0 | (channel & 0x0F), program & 0x7F);
    logMidiMessage("Program Change", 0xC0 | (channel & 0x0F), program);
}

void HardwareMidiManager::sendPitchBend(uint8_t channel, uint16_t value) {
    uint8_t lsb = value & 0x7F;
    uint8_t msb = (value >> 7) & 0x7F;
    sendMidiMessage(0xE0 | (channel & 0x0F), lsb, msb);
    logMidiMessage("Pitch Bend", 0xE0 | (channel & 0x0F), lsb, msb);
}

void HardwareMidiManager::sendAftertouch(uint8_t channel, uint8_t pressure) {
    sendMidiMessage(0xD0 | (channel & 0x0F), pressure & 0x7F);
    logMidiMessage("Aftertouch", 0xD0 | (channel & 0x0F), pressure);
}

void HardwareMidiManager::sendPolyAftertouch(uint8_t channel, uint8_t note, uint8_t pressure) {
    sendMidiMessage(0xA0 | (channel & 0x0F), note & 0x7F, pressure & 0x7F);
    logMidiMessage("Poly Aftertouch", 0xA0 | (channel & 0x0F), note, pressure);
}

// MIDI Clock & Transport
void HardwareMidiManager::sendClockPulse() {
    sendMidiMessage(0xF8); // MIDI Clock
    clock_pulses_sent_++;
    // Don't log every clock pulse to avoid spam
}

void HardwareMidiManager::sendStart() {
    sendMidiMessage(0xFA); // MIDI Start
    logMidiMessage("Start", 0xFA);
}

void HardwareMidiManager::sendStop() {
    sendMidiMessage(0xFC); // MIDI Stop
    logMidiMessage("Stop", 0xFC);
}

void HardwareMidiManager::sendContinue() {
    sendMidiMessage(0xFB); // MIDI Continue
    logMidiMessage("Continue", 0xFB);
}

// System Messages
void HardwareMidiManager::sendSystemReset() {
    sendMidiMessage(0xFF); // System Reset
    logMidiMessage("System Reset", 0xFF);
}

void HardwareMidiManager::sendActiveSensing() {
    sendMidiMessage(0xFE); // Active Sensing
    // Don't log active sensing to avoid spam
}

void HardwareMidiManager::sendAllNotesOff(uint8_t channel) {
    sendControlChange(channel, 123, 0); // CC 123 = All Notes Off
}

void HardwareMidiManager::sendAllSoundOff(uint8_t channel) {
    sendControlChange(channel, 120, 0); // CC 120 = All Sound Off
}

void HardwareMidiManager::resetStatistics() {
    messages_sent_ = 0;
    clock_pulses_sent_ = 0;
}

// Low-level MIDI transmission
void HardwareMidiManager::sendMidiMessage(uint8_t status, uint8_t data1, uint8_t data2) {
#ifdef ESP32_BUILD
    if (midi_serial_ && initialized_) {
        midi_serial_->write(status);
        midi_serial_->write(data1);
        midi_serial_->write(data2);
    }
#endif
    messages_sent_++;
}

void HardwareMidiManager::sendMidiMessage(uint8_t status, uint8_t data1) {
#ifdef ESP32_BUILD
    if (midi_serial_ && initialized_) {
        midi_serial_->write(status);
        midi_serial_->write(data1);
    }
#endif
    messages_sent_++;
}

void HardwareMidiManager::sendMidiMessage(uint8_t status) {
#ifdef ESP32_BUILD
    if (midi_serial_ && initialized_) {
        midi_serial_->write(status);
    }
#endif
    messages_sent_++;
}

void HardwareMidiManager::logMidiMessage(const char* type, uint8_t status, uint8_t data1, uint8_t data2) {
#ifdef ESP32_BUILD
    // Only log on ESP32 for real hardware debugging
    std::cout << "[HW MIDI] " << type << " - Status: 0x" << std::hex << (int)status;
    if (data1 != 0 || data2 != 0) {
        std::cout << " Data: 0x" << (int)data1;
        if (data2 != 0) {
            std::cout << " 0x" << (int)data2;
        }
    }
    std::cout << std::dec << std::endl;
#else
    // Desktop simulation with more readable format
    std::cout << "[MIDI Sim] " << type;
    if (data1 != 0 || data2 != 0) {
        std::cout << " - Ch:" << ((status & 0x0F) + 1) << " Data:" << (int)data1;
        if (data2 != 0) {
            std::cout << "," << (int)data2;
        }
    }
    std::cout << std::endl;
#endif
}
