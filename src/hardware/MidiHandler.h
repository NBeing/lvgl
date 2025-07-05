#pragma once

#if defined(ESP32_BUILD)
    #include <Control_Surface.h>
#else
    #include <iostream>
    #include <vector>
    #include <cstdint>
#endif

class MidiHandler {
private:
    #if defined(ESP32_BUILD)
        // Move the MIDI interface inside the class as a member variable
        USBMIDI_Interface midi_interface_;
    #endif
    
    bool initialized_;
    
public:
    MidiHandler() : initialized_(false) {
        #if defined(ESP32_BUILD)
            // Constructor initializes the MIDI interface
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
            std::cout << "Initializing Desktop MIDI simulation..." << std::endl;
            initialized_ = true;
            return true;
        #endif
    }
    
    void sendControlChange(uint8_t channel, uint8_t cc_number, uint8_t value) {
        if (!initialized_) return;
        
        #if defined(ESP32_BUILD)
            // Use the correct API for Control-Surface
            Control_Surface.sendControlChange({cc_number, Channel(channel)}, value);
            std::cout << "ESP32 MIDI CC: Ch" << (int)channel << " CC" << (int)cc_number << " Val" << (int)value << std::endl;
        #else
            std::cout << "MIDI CC: Channel " << (int)channel 
                     << " CC" << (int)cc_number 
                     << " Value " << (int)value << std::endl;
        #endif
    }
    
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
        if (!initialized_) return;
        
        #if defined(ESP32_BUILD)
            Control_Surface.sendNoteOn({note, Channel(channel)}, velocity);
        #else
            std::cout << "MIDI Note On: Channel " << (int)channel 
                     << " Note " << (int)note 
                     << " Velocity " << (int)velocity << std::endl;
        #endif
    }
    
    void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
        if (!initialized_) return;
        
        #if defined(ESP32_BUILD)
            Control_Surface.sendNoteOff({note, Channel(channel)}, velocity);
        #else
            std::cout << "MIDI Note Off: Channel " << (int)channel 
                     << " Note " << (int)note 
                     << " Velocity " << (int)velocity << std::endl;
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
            return "Desktop MIDI Simulation";
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