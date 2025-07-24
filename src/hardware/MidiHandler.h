#pragma once

#if defined(ESP32_BUILD)
    #include <Arduino.h>
    #include <string>
    #include <Control_Surface.h>     // Control-Surface with Arduino Core 3.x support
    #include <iostream>

#else
    #include "RtMidi.h"
    #include <iostream>
    #include <vector>
    #include <cstdint>
    #include <memory>
    #include <string>
#endif

// Forward declaration of the MIDI logging function
extern "C" void logHardwareMidiInput(uint8_t status, uint8_t data1, uint8_t data2);

class MidiHandler {
private:
    bool initialized_;
    
    #if defined(ESP32_BUILD)
        // Move the MIDI interface inside the class as a member variable
        USBMIDI_Interface midi_interface_;
    #else
        std::unique_ptr<RtMidiOut> midi_out_;
        std::unique_ptr<RtMidiIn> midi_in_;
        unsigned int current_port_;
        std::vector<std::string> available_ports_;
    #endif
    
public:
    MidiHandler() : initialized_(false) {
        #if defined(ESP32_BUILD)
            std::cout << "MidiHandler constructor called! Instance at: " << this << std::endl;
        #else
            std::cout << "MidiHandler constructor called! Instance at: " << this << std::endl;
        #endif
        #if !defined(ESP32_BUILD)
            try {
                midi_out_ = std::make_unique<RtMidiOut>();
                midi_in_ = std::make_unique<RtMidiIn>();
                current_port_ = 0;
                std::cout << "MidiHandler " << this << " - RtMidiOut and RtMidiIn created" << std::endl;
            } catch (RtMidiError& error) {
                std::cerr << "RtMidi initialization error: " << error.getMessage() << std::endl;
            }
        #endif
    }
    
    #if !defined(ESP32_BUILD)
    void setupMidiInput() {
        if (!midi_in_) return;
        
        try {
            // Set up MIDI input callback
            midi_in_->setCallback([](double /*timeStamp*/, std::vector<unsigned char>* message, void* /*userData*/) {
                if (message->size() >= 1) {
                    uint8_t status = (*message)[0];
                    uint8_t data1 = message->size() > 1 ? (*message)[1] : 0;
                    uint8_t data2 = message->size() > 2 ? (*message)[2] : 0;
                    
                    std::cout << "[RtMidi Input] Received: " << std::hex 
                              << (int)status << " " << (int)data1 << " " << (int)data2 << std::dec << std::endl;
                    
                    // Forward to UnifiedMidiManager
                    logHardwareMidiInput(status, data1, data2);
                }
            });
            
            // Don't ignore sysex, timing, or active sensing messages
            midi_in_->ignoreTypes(false, false, false);
            
            // Scan for input ports and open the first hardware MIDI port
            unsigned int input_port_count = midi_in_->getPortCount();
            std::cout << "[RtMidi Input] Found " << input_port_count << " input ports" << std::endl;
            
            if (input_port_count == 0) {
                std::cout << "[RtMidi Input] ❌ No MIDI input ports available!" << std::endl;
                return;
            }
            
            for (unsigned int i = 0; i < input_port_count; i++) {
                try {
                    std::string port_name = midi_in_->getPortName(i);
                    std::cout << "  Input Port " << i << ": " << port_name << std::endl;
                    
                    // Look for hardware MIDI port (avoid software/virtual ports)
                    if (port_name.find("Midi Through") == std::string::npos &&
                        port_name.find("Virtual") == std::string::npos) {
                        midi_in_->openPort(i);
                        std::cout << "🎹 Desktop MIDI INPUT: Connected to port " << i << ": " << port_name << std::endl;
                        return; // Successfully opened input
                    }
                } catch (RtMidiError& error) {
                    std::cerr << "Error getting input port " << i << " name: " << error.getMessage() << std::endl;
                }
            }
            
            // If no hardware ports found, try opening the first available port
            if (input_port_count > 0) {
                midi_in_->openPort(0);
                std::cout << "🎹 Desktop MIDI INPUT: Connected to first available port" << std::endl;
            } else {
                std::cout << "⚠️  Desktop MIDI INPUT: No input ports available" << std::endl;
            }
            
        } catch (RtMidiError& error) {
            std::cerr << "Desktop MIDI input setup failed: " << error.getMessage() << std::endl;
        }
    }
    #endif
    
    bool initialize() {
        #if defined(ESP32_BUILD)
            std::cout << "Initializing Control-Surface USB MIDI..." << std::endl;

            // Initialize the MIDI interface - this is the key!
            midi_interface_.begin();
            
            std::cout << "✅ Control-Surface USB MIDI initialized successfully!" << std::endl;
            std::cout << "Device should now appear as USB MIDI device" << std::endl;
            std::cout << "Check with: lsusb -v -d 303a:1001 | grep -A 5 bInterfaceClass" << std::endl;
            initialized_ = true;
            return true;
        #else
            // Initialize RtMidi on desktop
            if (!midi_out_ || !midi_in_) return false;
            
            try {
                // Setup MIDI Input first
                setupMidiInput();
                
                // Scan for available MIDI ports
                unsigned int port_count = midi_out_->getPortCount();
                available_ports_.clear();
                
                std::cout << "Desktop MIDI: Found " << port_count << " output ports:" << std::endl;
                
                for (unsigned int i = 0; i < port_count; i++) {
                    try {
                        std::string port_name = midi_out_->getPortName(i);
                        available_ports_.push_back(port_name);
                        std::cout << "  Port " << i << ": " << port_name << std::endl;
                    } catch (RtMidiError& error) {
                        std::cerr << "Error getting port " << i << " name: " << error.getMessage() << std::endl;
                    }
                }
                
                // Try to find a suitable output port (avoid Midi Through to prevent loops)
                bool found_output_port = false;
                for (unsigned int i = 0; i < available_ports_.size(); i++) {
                    // Skip Midi Through and our own input clients to prevent MIDI loops
                    if (available_ports_[i].find("Midi Through") == std::string::npos &&
                        available_ports_[i].find("RtMidi Input") == std::string::npos) {
                        midi_out_->openPort(i);
                        current_port_ = i;
                        std::cout << "Desktop MIDI: Connected to port " << i << ": " << available_ports_[i] << std::endl;
                        found_output_port = true;
                        break;
                    }
                }
                
                if (!found_output_port) {
                    // Create virtual port to avoid loops
                    midi_out_->openVirtualPort("LVGL Synth Output");
                    std::cout << "Desktop MIDI: Created virtual port 'LVGL Synth Output'" << std::endl;
                }
                
                initialized_ = true;
                return true;
                
            } catch (RtMidiError& error) {
                std::cerr << "Desktop MIDI initialization failed: " << error.getMessage() << std::endl;
                return false;
            }
        #endif
    }
    
    bool isConnected() {
        return initialized_;
    }
    
    std::string getConnectionStatus() {
        #if defined(ESP32_BUILD)
            return initialized_ ? "Control-Surface USB MIDI Connected" : "Control-Surface USB MIDI Disconnected";
        #else
            return initialized_ ? "Desktop MIDI Connected" : "Desktop MIDI Disconnected";
        #endif
    }
    
    void sendControlChange(uint8_t channel, uint8_t cc_number, uint8_t value) {
        if (!initialized_) return;
        
        #if defined(ESP32_BUILD)
            // Use Control-Surface's MIDI sending through the interface
            midi_interface_.sendControlChange({cc_number, Channel(channel)}, value);
            // Serial.printf("Control-Surface CC: Ch%d CC%d Val%d\n", channel, cc_number, value);
        #else
            // Desktop - Use RtMidi
            if (!midi_out_) return;
            
            try {
                std::vector<unsigned char> message;
                message.push_back(0xB0 | (channel & 0x0F));  // Control Change + channel
                message.push_back(cc_number & 0x7F);         // CC number
                message.push_back(value & 0x7F);             // CC value
                
                midi_out_->sendMessage(&message);
                std::cout << "Desktop MIDI CC: Ch" << (int)channel << " CC" << (int)cc_number << " Val" << (int)value << std::endl;
                
            } catch (RtMidiError& error) {
                std::cerr << "MIDI send error: " << error.getMessage() << std::endl;
            }
        #endif
    }
    
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
        if (!initialized_) return;
        
        #if defined(ESP32_BUILD)
            midi_interface_.sendNoteOn({note, Channel(channel)}, velocity);
            // Serial.printf("Control-Surface Note On: Ch%d Note%d Vel%d\n", channel, note, velocity);
        #else
            if (!midi_out_) return;
            
            try {
                std::vector<unsigned char> message;
                message.push_back(0x90 | (channel & 0x0F));  // Note On + channel
                message.push_back(note & 0x7F);              // Note number
                message.push_back(velocity & 0x7F);          // Velocity
                
                midi_out_->sendMessage(&message);
                
            } catch (RtMidiError& error) {
                std::cerr << "MIDI send error: " << error.getMessage() << std::endl;
            }
        #endif
    }
    
    void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity = 0) {
        if (!initialized_) return;
        
        #if defined(ESP32_BUILD)
            midi_interface_.sendNoteOff({note, Channel(channel)}, velocity);
            // Serial.printf("Control-Surface Note Off: Ch%d Note%d\n", channel, note);
        #else
            if (!midi_out_) return;
            
            try {
                std::vector<unsigned char> message;
                message.push_back(0x80 | (channel & 0x0F));  // Note Off + channel
                message.push_back(note & 0x7F);              // Note number
                message.push_back(velocity & 0x7F);          // Velocity
                
                midi_out_->sendMessage(&message);
                
            } catch (RtMidiError& error) {
                std::cerr << "MIDI send error: " << error.getMessage() << std::endl;
            }
        #endif
    }
    
    void update() {
        if (!initialized_) return;
        
        #if defined(ESP32_BUILD)
            // Update the MIDI interface - this is important for proper operation
            midi_interface_.update();
        #endif
    }
    
    // Advanced Control-Surface features
    #if defined(ESP32_BUILD)
    void sendPitchBend(uint8_t channel, int16_t bend) {
        if (!initialized_) return;
        midi_interface_.sendPitchBend(Channel(channel), bend);
        // Serial.printf("Control-Surface Pitch Bend: Ch%d Bend%d\n", channel, bend);
    }
    
    void sendProgramChange(uint8_t channel, uint8_t program) {
        if (!initialized_) return;
        midi_interface_.sendProgramChange(Channel(channel), program);
        // Serial.printf("Control-Surface Program Change: Ch%d Prog%d\n", channel, program);
    }
    
    void sendAftertouch(uint8_t channel, uint8_t note, uint8_t pressure) {
        if (!initialized_) return;
        midi_interface_.sendKeyPressure({note, Channel(channel)}, pressure);
        // Serial.printf("Control-Surface Aftertouch: Ch%d Note%d Press%d\n", channel, note, pressure);
    }
    #endif
    
    // System/Real-time MIDI Messages
    void sendClockPulse() {
        if (!initialized_) return;
        
        #if defined(ESP32_BUILD)
            midi_interface_.sendRealTime(0xF8);
        #else
            if (!midi_out_) return;
            
            try {
                std::vector<unsigned char> message;
                message.push_back(0xF8);  // MIDI Clock
                midi_out_->sendMessage(&message);
                std::cout << "Desktop MIDI Clock sent" << std::endl;
            } catch (RtMidiError& error) {
                std::cerr << "MIDI clock send error: " << error.getMessage() << std::endl;
            }
        #endif
    }
    
    void sendStart() {
        if (!initialized_) return;
        
        #if defined(ESP32_BUILD)
            midi_interface_.sendRealTime(0xFA);
        #else
            if (!midi_out_) return;
            
            try {
                std::vector<unsigned char> message;
                message.push_back(0xFA);  // MIDI Start
                midi_out_->sendMessage(&message);
                std::cout << "Desktop MIDI Start sent" << std::endl;
            } catch (RtMidiError& error) {
                std::cerr << "MIDI start send error: " << error.getMessage() << std::endl;
            }
        #endif
    }
    
    void sendStop() {
        if (!initialized_) return;
        
        #if defined(ESP32_BUILD)
            midi_interface_.sendRealTime(0xFC);
        #else
            if (!midi_out_) return;
            
            try {
                std::vector<unsigned char> message;
                message.push_back(0xFC);  // MIDI Stop
                midi_out_->sendMessage(&message);
                std::cout << "Desktop MIDI Stop sent" << std::endl;
            } catch (RtMidiError& error) {
                std::cerr << "MIDI stop send error: " << error.getMessage() << std::endl;
            }
        #endif
    }
    
    void sendContinue() {
        if (!initialized_) return;
        
        #if defined(ESP32_BUILD)
            midi_interface_.sendRealTime(0xFB);
        #else
            if (!midi_out_) return;
            
            try {
                std::vector<unsigned char> message;
                message.push_back(0xFB);  // MIDI Continue
                midi_out_->sendMessage(&message);
                std::cout << "Desktop MIDI Continue sent" << std::endl;
            } catch (RtMidiError& error) {
                std::cerr << "MIDI continue send error: " << error.getMessage() << std::endl;
            }
        #endif
    }
    
    void sendSystemMessage(uint8_t status) {
        if (!initialized_) return;
        
        #if defined(ESP32_BUILD)
            midi_interface_.sendRealTime(status);
        #else
            if (!midi_out_) return;
            
            try {
                std::vector<unsigned char> message;
                message.push_back(status);
                midi_out_->sendMessage(&message);
                std::cout << "Desktop MIDI System Message: 0x" << std::hex << (int)status << std::dec << " sent" << std::endl;
            } catch (RtMidiError& error) {
                std::cerr << "MIDI system message send error: " << error.getMessage() << std::endl;
            }
        #endif
    }
};