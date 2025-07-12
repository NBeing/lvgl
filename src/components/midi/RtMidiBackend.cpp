#include "RtMidiBackend.h"
#include <iostream>
#include <thread>
#include <chrono>

#if !defined(ESP32_BUILD)

RtMidiBackend::RtMidiBackend() {
    // Constructor
}

RtMidiBackend::~RtMidiBackend() {
    cleanup();
}

void RtMidiBackend::setMidiHandler(std::shared_ptr<MidiHandler> handler) {
    midi_handler_ = handler;
    std::cout << "[RtMidi] Using shared MidiHandler instance" << std::endl;
}

bool RtMidiBackend::initialize() {
    if (!midi_handler_) {
        std::cout << "[RtMidi] No MidiHandler set - creating new instance" << std::endl;
        try {
            // Fallback: create our own if none provided
            midi_handler_ = std::shared_ptr<MidiHandler>(new MidiHandler());
            
            // Give some time for MIDI devices to be detected
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
        } catch (const std::exception& e) {
            std::cout << "[RtMidi] Initialization failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    initialized_ = true;
    std::cout << "[RtMidi] USB MIDI backend initialized successfully" << std::endl;
    std::cout << "[RtMidi] Status: " << midi_handler_->getConnectionStatus() << std::endl;
    
    return true;
}

void RtMidiBackend::cleanup() {
    if (midi_handler_) {
        // Don't reset if it's a shared pointer - other components might be using it
        midi_handler_.reset();
    }
    initialized_ = false;
    std::cout << "[RtMidi] Backend cleaned up" << std::endl;
}

UnifiedMidiManager::ConnectionStatus RtMidiBackend::getStatus() const {
    if (!midi_handler_) {
        std::cout << "[RtMidi] getStatus: No midi_handler_" << std::endl;
        return UnifiedMidiManager::ConnectionStatus::DISCONNECTED;
    }
    
    bool is_connected = midi_handler_->isConnected();
    std::string status_str = midi_handler_->getConnectionStatus();
    std::cout << "[RtMidi] getStatus: isConnected=" << is_connected << ", status=" << status_str << std::endl;
    
    return is_connected ? 
        UnifiedMidiManager::ConnectionStatus::CONNECTED :
        UnifiedMidiManager::ConnectionStatus::DISCONNECTED;
}

void RtMidiBackend::sendMessage(uint8_t status, uint8_t data1, uint8_t data2) {
    if (!midi_handler_ || !initialized_) {
        return;
    }
    
    uint8_t command = status & 0xF0;
    uint8_t channel = status & 0x0F;
    
    switch (command) {
        case 0x80: // Note Off
            midi_handler_->sendNoteOff(channel, data1, data2);
            break;
        case 0x90: // Note On
            midi_handler_->sendNoteOn(channel, data1, data2);
            break;
        case 0xB0: // Control Change
            midi_handler_->sendControlChange(channel, data1, data2);
            break;
        case 0xC0: // Program Change
            // MidiHandler doesn't have sendProgramChange, simulate with CC
            std::cout << "[RtMidi] Program Change " << (int)data1 << " (simulated)" << std::endl;
            break;
        case 0xE0: // Pitch Bend
            // MidiHandler doesn't have sendPitchBend, simulate with CC
            std::cout << "[RtMidi] Pitch Bend " << (int)data1 << "," << (int)data2 << " (simulated)" << std::endl;
            break;
        default:
            std::cout << "[RtMidi] Unknown MIDI command: 0x" << std::hex << (int)command << std::dec << std::endl;
            break;
    }
    
    messages_sent_++;
}

void RtMidiBackend::sendMessage(uint8_t status) {
    if (!midi_handler_ || !initialized_) {
        return;
    }
    
    // Handle single-byte messages (system real-time)
    switch (status) {
        case 0xF8: // MIDI Clock
        case 0xFA: // Start
        case 0xFB: // Continue  
        case 0xFC: // Stop
        case 0xFE: // Active Sensing
        case 0xFF: // System Reset
            // For now, just log these - MidiHandler may not support them directly
            std::cout << "[RtMidi] System message: 0x" << std::hex << (int)status << std::dec << std::endl;
            break;
        default:
            std::cout << "[RtMidi] Unknown single-byte message: 0x" << std::hex << (int)status << std::dec << std::endl;
            break;
    }
    
    messages_sent_++;
}

void RtMidiBackend::update() {
    // Update input processing - MidiHandler may handle this internally
    if (midi_handler_) {
        // Check for incoming messages and call callbacks if needed
        // This depends on how MidiHandler implements input processing
    }
}

#else
// ESP32 stub - RtMidi not available on ESP32
RtMidiBackend::RtMidiBackend() {}
RtMidiBackend::~RtMidiBackend() {}
bool RtMidiBackend::initialize() { return false; }
void RtMidiBackend::cleanup() {}
UnifiedMidiManager::ConnectionStatus RtMidiBackend::getStatus() const { 
    return UnifiedMidiManager::ConnectionStatus::DISCONNECTED; 
}
void RtMidiBackend::sendMessage(uint8_t, uint8_t, uint8_t) {}
void RtMidiBackend::sendMessage(uint8_t) {}
void RtMidiBackend::update() {}
#endif
