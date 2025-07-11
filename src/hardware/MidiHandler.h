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

class MidiHandler {
private:
    bool initialized_;
    
    #if defined(ESP32_BUILD)
        // Move the MIDI interface inside the class as a member variable
        USBMIDI_Interface midi_interface_;
    #else
        std::unique_ptr<RtMidiOut> midi_out_;
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
                current_port_ = 0;
                std::cout << "MidiHandler " << this << " - RtMidiOut created" << std::endl;
            } catch (RtMidiError& error) {
                std::cerr << "RtMidi initialization error: " << error.getMessage() << std::endl;
            }
        #endif
    }
    
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
            if (!midi_out_) return false;
            
            try {
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
                
                // Try to open the first available port, or create a virtual port
                if (available_ports_.size() > 0) {
                    // Open existing port
                    midi_out_->openPort(0);
                    current_port_ = 0;
                    std::cout << "Desktop MIDI: Connected to port 0: " << available_ports_[0] << std::endl;
                } else {
                    // Create virtual port
                    midi_out_->openVirtualPort("LVGL Synth Controller");
                    std::cout << "Desktop MIDI: Created virtual port 'LVGL Synth Controller'" << std::endl;
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
};