#include "ESP32USBMidiBackend.h"
#include <iostream>

ESP32USBMidiBackend::ESP32USBMidiBackend() {
    // Constructor
}

ESP32USBMidiBackend::~ESP32USBMidiBackend() {
    cleanup();
}

bool ESP32USBMidiBackend::initialize() {
#ifdef ESP32_BUILD
    try {
        std::cout << "[ESP32 USB MIDI] Initializing USB MIDI interface..." << std::endl;
        
        // Create USB MIDI interface using Control Surface
        usb_midi_ = new USBMIDI_Interface();
        
        // Begin the Control Surface library (this initializes USB MIDI)
        Control_Surface.begin();
        
        initialized_ = true;
        status_ = UnifiedMidiManager::ConnectionStatus::CONNECTED;
        
        std::cout << "[ESP32 USB MIDI] Control Surface initialized successfully" << std::endl;
        std::cout << "[ESP32 USB MIDI] Device should appear as USB MIDI device" << std::endl;
        std::cout << "[ESP32 USB MIDI] Check with 'aconnect -l' or similar MIDI monitoring tools" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cout << "[ESP32 USB MIDI] Initialization failed: " << e.what() << std::endl;
        status_ = UnifiedMidiManager::ConnectionStatus::ERROR;
        return false;
    }
#else
    // Desktop simulation mode
    std::cout << "[ESP32 USB MIDI] Simulation mode (desktop)" << std::endl;
    initialized_ = true;
    status_ = UnifiedMidiManager::ConnectionStatus::CONNECTED;
    return true;
#endif
}

void ESP32USBMidiBackend::cleanup() {
    if (!initialized_) return;
    
#ifdef ESP32_BUILD
    if (usb_midi_) {
        delete usb_midi_;
        usb_midi_ = nullptr;
    }
#endif
    
    initialized_ = false;
    status_ = UnifiedMidiManager::ConnectionStatus::DISCONNECTED;
    std::cout << "[ESP32 USB MIDI] Cleaned up" << std::endl;
}

UnifiedMidiManager::ConnectionStatus ESP32USBMidiBackend::getStatus() const {
    return status_;
}

void ESP32USBMidiBackend::sendMessage(uint8_t status, uint8_t data1, uint8_t data2) {
    if (!initialized_ || status_ != UnifiedMidiManager::ConnectionStatus::CONNECTED) {
        return;
    }
    
#ifdef ESP32_BUILD
    if (usb_midi_) {
        // Use Control Surface's ChannelMessage structure
        // Extract message type and channel from status byte
        uint8_t message_type = status & 0xF0;
        uint8_t channel = (status & 0x0F) + 1; // Control Surface uses 1-based channels
        
        switch (message_type) {
            case 0x90: // Note On
                usb_midi_->sendNoteOn({data1, cs::Channel(channel)}, data2);
                break;
            case 0x80: // Note Off
                usb_midi_->sendNoteOff({data1, cs::Channel(channel)}, data2);
                break;
            case 0xB0: // Control Change
                usb_midi_->sendControlChange({data1, cs::Channel(channel)}, data2);
                break;
            case 0xC0: // Program Change
                usb_midi_->sendProgramChange(cs::Channel(channel), data1);
                break;
            case 0xE0: // Pitch Bend
                usb_midi_->sendPitchBend(cs::Channel(channel), (data2 << 7) | data1);
                break;
            default:
                // Create a generic ChannelMessage for other types
                cs::ChannelMessage msg = {status, data1, data2};
                usb_midi_->send(msg);
                break;
        }
    }
#else
    // Desktop simulation
    std::cout << "[ESP32 USB MIDI] 0x" << std::hex 
              << (int)status << " 0x" << (int)data1 << " 0x" << (int)data2 << std::dec << std::endl;
#endif
    
    messages_sent_++;
}

void ESP32USBMidiBackend::sendMessage(uint8_t status) {
    if (!initialized_ || status_ != UnifiedMidiManager::ConnectionStatus::CONNECTED) {
        return;
    }
    
#ifdef ESP32_BUILD
    if (usb_midi_) {
        // Send single-byte MIDI message (usually real-time messages)
        // Control Surface expects RealTimeMessage enum values
        switch (status) {
            case 0xF8: // MIDI Clock
                usb_midi_->sendRealTime(0xF8);
                break;
            case 0xFA: // Start
                usb_midi_->sendRealTime(0xFA);
                break;
            case 0xFB: // Continue
                usb_midi_->sendRealTime(0xFB);
                break;
            case 0xFC: // Stop
                usb_midi_->sendRealTime(0xFC);
                break;
            default:
                // Send generic real-time message
                usb_midi_->sendRealTime(status);
                break;
        }
    }
#else
    // Desktop simulation
    std::cout << "[ESP32 USB MIDI] 0x" << std::hex << (int)status << std::dec << std::endl;
#endif
    
    messages_sent_++;
}

void ESP32USBMidiBackend::update() {
    if (!initialized_) return;
    
#ifdef ESP32_BUILD
    // Update Control Surface (this processes USB MIDI input/output)
    Control_Surface.loop();
    
    // Check for incoming messages
    if (usb_midi_) {
        // Note: Control Surface handles incoming messages via callbacks
        // We would need to set up a callback system to capture them
    }
#endif
}

void ESP32USBMidiBackend::handleIncomingMessage(uint8_t status, uint8_t data1, uint8_t data2) {
    // This would be called by a Control Surface callback
    // TODO: Forward to UnifiedMidiManager callback system
    messages_received_++;
}
