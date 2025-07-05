#pragma once

#if defined(ESP32_BUILD)
    #include <Control_Surface.h>
#else
    #include "RtMidi.h"
    #include <iostream>
    #include <vector>
    #include <cstdint>
    #include <memory>
#endif

class MidiHandler {
private:
    #if defined(ESP32_BUILD)
        // Move the MIDI interface inside the class as a member variable
        USBMIDI_Interface midi_interface_;
    #else
        std::unique_ptr<RtMidiOut> midi_out_;
        unsigned int current_port_;
        std::vector<std::string> available_ports_;
    #endif
    
    bool initialized_;
    
public:
    MidiHandler() : initialized_(false) {
        #if defined(ESP32_BUILD)
            // Constructor initializes the MIDI interface
        #else
            // Initialize RtMidi
            try {
                midi_out_ = std::make_unique<RtMidiOut>();
                current_port_ = 0;
            } catch (RtMidiError& error) {
                std::cerr << "RtMidi initialization error: " << error.getMessage() << std::endl;
            }
        #endif
    }
    
    bool initialize() {
        #if defined(ESP32_BUILD)
            // Initialize Control-Surface with ESP32 USB MIDI
            std::cout << "Initializing ESP32-S3 USB MIDI with Control-Surface..." << std::endl;
            Control_Surface.begin();
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
    
    void sendControlChange(uint8_t channel, uint8_t cc_number, uint8_t value) {
        if (!initialized_) return;
        
        #if defined(ESP32_BUILD)
            // Use the correct API for Control-Surface
            Control_Surface.sendControlChange({cc_number, Channel(channel)}, value);
            std::cout << "ESP32 MIDI CC: Ch" << (int)channel << " CC" << (int)cc_number << " Val" << (int)value << std::endl;
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
            Control_Surface.sendNoteOn({note, Channel(channel)}, velocity);
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
    
    void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
        if (!initialized_) return;
        
        #if defined(ESP32_BUILD)
            Control_Surface.sendNoteOff({note, Channel(channel)}, velocity);
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
    
    bool isConnected() const {
        return initialized_;
    }
    
    const char* getConnectionStatus() const {
        if (!initialized_) return "Not initialized";
        
        #if defined(ESP32_BUILD)
            return "ESP32-S3 USB MIDI (Control_Surface)";
        #else
            static std::string status;
            if (available_ports_.size() > 0) {
                status = "Desktop RtMidi: " + available_ports_[current_port_];
            } else {
                status = "Desktop RtMidi: Virtual Port";
            }
            return status.c_str();
        #endif
    }
    
    void update() {
        if (!initialized_) return;
        
        #if defined(ESP32_BUILD)
            Control_Surface.loop();  // Process MIDI events
        #else
            // Desktop - nothing needed for console output
        #endif
    }
};