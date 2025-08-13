#include "HardwareMidiBackend.h"
#include <iostream>

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
#include "debug/RTEventTracer.h"
#endif

// Forward declaration to avoid circular dependency
class UnifiedMidiManager;

// External callback function declaration
extern "C" void logHardwareMidiInput(uint8_t status, uint8_t data1, uint8_t data2);

HardwareMidiBackend::HardwareMidiBackend() {
    // Constructor
}

HardwareMidiBackend::~HardwareMidiBackend() {
    cleanup();
}

bool HardwareMidiBackend::initialize() {
    if (initialized_) return true;
    
#if defined(ESP32_BUILD)
    std::cout << "[Hardware MIDI] Initializing serial MIDI backend..." << std::endl;
    std::cout << "[Hardware MIDI] TX Pin: " << tx_pin_ << ", RX Pin: " 
              << (rx_pin_ == -1 ? "disabled" : std::to_string(rx_pin_)) << std::endl;
    
    // Use UART1 for MIDI
    midi_serial_ = new HardwareSerial(1);
    
    if (!midi_serial_) {
        std::cout << "[Hardware MIDI] ERROR: Failed to create HardwareSerial" << std::endl;
        return false;
    }
    
    // Standard MIDI baud rate: 31250
    // 8 data bits, no parity, 1 stop bit
    midi_serial_->begin(31250, SERIAL_8N1, rx_pin_, tx_pin_);
    
    initialized_ = true;
    std::cout << "[Hardware MIDI] Serial MIDI backend initialized successfully" << std::endl;
    
    if (rx_pin_ != -1) {
        std::cout << "[Hardware MIDI] MIDI input enabled on pin " << rx_pin_ << std::endl;
    }
    
    // No test callback - monitoring only when tab is active
    
    return true;
#else
    std::cout << "[Hardware MIDI] Desktop simulation mode - initialized" << std::endl;
    initialized_ = false;
    return false;
#endif
}

void HardwareMidiBackend::cleanup() {
#if defined(ESP32_BUILD)
    if (midi_serial_) {
        midi_serial_->end();
        delete midi_serial_;
        midi_serial_ = nullptr;
    }
#endif
    
    initialized_ = false;
    std::cout << "[Hardware MIDI] Backend cleaned up" << std::endl;
}

UnifiedMidiManager::ConnectionStatus HardwareMidiBackend::getStatus() const {
#if defined(ESP32_BUILD)
    return (initialized_ && midi_serial_) ? 
        UnifiedMidiManager::ConnectionStatus::CONNECTED : 
        UnifiedMidiManager::ConnectionStatus::DISCONNECTED;
#else
    return UnifiedMidiManager::ConnectionStatus::DISCONNECTED;
#endif
}

void HardwareMidiBackend::sendMessage(uint8_t status, uint8_t data1, uint8_t data2) {
#if defined(ESP32_BUILD)
    if (midi_serial_ && initialized_) {
        std::cout << "[Hardware MIDI] Sending: 0x" << std::hex 
                  << (int)status << " 0x" << (int)data1 << " 0x" << (int)data2 << std::dec << std::endl;
        midi_serial_->write(status);
        midi_serial_->write(data1);
        midi_serial_->write(data2);
        messages_sent_++;
    } else {
        std::cout << "[Hardware MIDI] Cannot send - not initialized or no serial" << std::endl;
    }
#else
    std::cout << "[Hardware MIDI] Desktop sim: 0x" << std::hex 
              << (int)status << " 0x" << (int)data1 << " 0x" << (int)data2 << std::dec << std::endl;
#endif
}

void HardwareMidiBackend::sendMessage(uint8_t status) {
#if defined(ESP32_BUILD)
    if (midi_serial_ && initialized_) {
        std::cout << "[Hardware MIDI] Sending: 0x" << std::hex << (int)status << std::dec << std::endl;
        midi_serial_->write(status);
        messages_sent_++;
    } else {
        std::cout << "[Hardware MIDI] Cannot send - not initialized or no serial" << std::endl;
    }
#else
    std::cout << "[Hardware MIDI] Desktop sim: 0x" << std::hex << (int)status << std::dec << std::endl;
#endif
}

void HardwareMidiBackend::update() {
#if defined(ESP32_BUILD)
    if (!midi_serial_ || !initialized_) return;
    
    // Check for incoming MIDI data (if RX is enabled)
    while (midi_serial_->available()) {
        uint8_t byte = midi_serial_->read();
        processMidiByte(byte);
    }
#endif
}

void HardwareMidiBackend::processMidiByte(uint8_t byte) {
#if defined(ESP32_BUILD)
    if (isStatusByte(byte)) {
        // Handle status byte
        if (byte >= 0xF8) {
            // Real-time message - handle immediately
            handleCompleteMessage(byte, 0, 0);
            return;
        }
        
        // Channel/System message
        current_status_ = byte;
        running_status_ = byte; // Update running status
        parse_state_ = MidiParseState::WAITING_FOR_DATA1;
        
        // Some messages don't need data bytes
        int msg_length = getMessageLength(byte);
        if (msg_length == 1) {
            handleCompleteMessage(byte, 0, 0);
            parse_state_ = MidiParseState::WAITING_FOR_STATUS;
        }
    } else if (isDataByte(byte)) {
        // Handle data byte
        int msg_length; // Declare here to avoid cross-initialization issues
        
        switch (parse_state_) {
            case MidiParseState::WAITING_FOR_STATUS:
                // Running status - use previous status byte
                if (running_status_ != 0) {
                    current_status_ = running_status_;
                    current_data1_ = byte;
                    
                    msg_length = getMessageLength(current_status_);
                    if (msg_length == 2) {
                        handleCompleteMessage(current_status_, current_data1_, 0);
                        parse_state_ = MidiParseState::WAITING_FOR_STATUS;
                    } else {
                        parse_state_ = MidiParseState::WAITING_FOR_DATA2;
                    }
                }
                break;
                
            case MidiParseState::WAITING_FOR_DATA1:
                current_data1_ = byte;
                
                msg_length = getMessageLength(current_status_);
                if (msg_length == 2) {
                    handleCompleteMessage(current_status_, current_data1_, 0);
                    parse_state_ = MidiParseState::WAITING_FOR_STATUS;
                } else {
                    parse_state_ = MidiParseState::WAITING_FOR_DATA2;
                }
                break;
                
            case MidiParseState::WAITING_FOR_DATA2:
                handleCompleteMessage(current_status_, current_data1_, byte);
                parse_state_ = MidiParseState::WAITING_FOR_STATUS;
                break;
        }
    }
#endif
}

void HardwareMidiBackend::handleCompleteMessage(uint8_t status, uint8_t data1, uint8_t data2) {
#if defined(ESP32_BUILD)
    messages_received_++;
    
    // Debug output
    uint8_t msg_type = status & 0xF0;
    uint8_t channel = (status & 0x0F) + 1; // Convert to 1-based
    
    // std::cout << "[Hardware MIDI] Received: ";  // DISABLED FOR DEBUG
    
    switch (msg_type) {
        case 0x90:
            // std::cout << "Note On Ch" << (int)channel << " Note:" << (int)data1 << " Vel:" << (int)data2;
            #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
            {
                std::string note_data = "Ch" + std::to_string(channel) + " Note:" + std::to_string(data1) + " Vel:" + std::to_string(data2);
                TRACE_MIDI_EVENT("ExternalController", "HardwareMidiBackend", "noteOn", note_data.c_str());
            }
            #endif
            break;
        case 0x80:
            // std::cout << "Note Off Ch" << (int)channel << " Note:" << (int)data1 << " Vel:" << (int)data2;
            #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
            {
                std::string note_data = "Ch" + std::to_string(channel) + " Note:" + std::to_string(data1) + " Vel:" + std::to_string(data2);
                TRACE_MIDI_EVENT("ExternalController", "HardwareMidiBackend", "noteOff", note_data.c_str());
            }
            #endif
            break;
        case 0xB0:
            // std::cout << "CC Ch" << (int)channel << " CC:" << (int)data1 << " Val:" << (int)data2;
            #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
            {
                std::string cc_data = "Ch" + std::to_string(channel) + " CC:" + std::to_string(data1) + " Val:" + std::to_string(data2);
                TRACE_MIDI_EVENT("ExternalController", "HardwareMidiBackend", "controlChange", cc_data.c_str());
            }
            #endif
            break;
        case 0xC0:
            // std::cout << "Program Change Ch" << (int)channel << " Program:" << (int)data1;
            #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
            {
                std::string pc_data = "Ch" + std::to_string(channel) + " Program:" + std::to_string(data1);
                TRACE_MIDI_EVENT("ExternalController", "HardwareMidiBackend", "programChange", pc_data.c_str());
            }
            #endif
            break;
        case 0xE0:
            // std::cout << "Pitch Bend Ch" << (int)channel << " Value:" << ((data2 << 7) | data1);
            #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
            {
                std::string pb_data = "Ch" + std::to_string(channel) + " Value:" + std::to_string((data2 << 7) | data1);
                TRACE_MIDI_EVENT("ExternalController", "HardwareMidiBackend", "pitchBend", pb_data.c_str());
            }
            #endif
            break;
        default:
            if (status >= 0xF8) {
                // std::cout << "Real-time: 0x" << std::hex << (int)status << std::dec;
                #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
                {
                    char rt_data[16];
                    snprintf(rt_data, sizeof(rt_data), "0x%02X", status);
                    TRACE_MIDI_EVENT("ExternalController", "HardwareMidiBackend", "realTime", rt_data);
                }
                #endif
            } else {
                // std::cout << "0x" << std::hex << (int)status << " 0x" << (int)data1 << " 0x" << (int)data2 << std::dec;
                #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
                {
                    char sys_data[32];
                    snprintf(sys_data, sizeof(sys_data), "0x%02X 0x%02X 0x%02X", status, data1, data2);
                    TRACE_MIDI_EVENT("ExternalController", "HardwareMidiBackend", "sysex", sys_data);
                }
                #endif
            }
            break;
    }
    
    // std::cout << std::endl;  // DISABLED FOR DEBUG
    
    // Forward to MIDI monitor via callback to avoid circular dependency
    logHardwareMidiInput(status, data1, data2);
    
    // TODO: Forward to callback system for parameter updates
    // For now, just log the received message
#endif
}

int HardwareMidiBackend::getMessageLength(uint8_t status) {
    uint8_t msg_type = status & 0xF0;
    
    switch (msg_type) {
        case 0x80: // Note Off
        case 0x90: // Note On  
        case 0xB0: // Control Change
        case 0xE0: // Pitch Bend
            return 3;
            
        case 0xC0: // Program Change
        case 0xD0: // Aftertouch
            return 2;
            
        default:
            if (status >= 0xF8) {
                return 1; // Real-time messages
            }
            return 3; // Default to 3-byte message
    }
}

void HardwareMidiBackend::setPins(int tx_pin, int rx_pin) {
    if (initialized_) {
        std::cout << "[Hardware MIDI] Cannot change pins while initialized" << std::endl;
        return;
    }
    
    tx_pin_ = tx_pin;
    rx_pin_ = rx_pin;
}

std::string HardwareMidiBackend::getName() const {
#if defined(ESP32_BUILD)
    return "Hardware Serial (Pin " + std::to_string(tx_pin_) + 
           (rx_pin_ != -1 ? ", RX " + std::to_string(rx_pin_) : "") + ")";
#else
    return "Hardware Serial (Simulated)";
#endif
}
