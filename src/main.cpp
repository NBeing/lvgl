// src/main.cpp - Main application entry point
#include <lvgl.h>
#include <iostream>
#include "components/app/SynthApp.h"
#include "hardware/MidiHandler.h"

#if defined(ESP32_BUILD)
#include "hardware/LGFX_ST7796S.h"
LGFX_ST7796S tft;  // Global for SynthApp to access

// Custom tick function for ESP32
uint32_t millis_cb() {
    return millis();
}
#endif

// Global app instance
SynthApp app;

// MIDI test variables
MidiHandler midiTest;
unsigned long lastMidiTime = 0;
int currentNote = 60;  // Start with Middle C
bool noteOn = false;

void midiTestLoop() {
    unsigned long currentTime;
    
    #if defined(ESP32_BUILD)
        currentTime = millis();
    #else
        // For desktop, use a simple counter (LVGL tick)
        currentTime = lv_tick_get();
    #endif
    
    // Send MIDI note every 1000ms (1 second)
    if (currentTime - lastMidiTime >= 1000) {
        if (!noteOn) {
            // Send Note On
            midiTest.sendNoteOn(1, currentNote, 80);  // Channel 1, note, velocity 80
            std::cout << "🎵 MIDI Test: Note ON  - " << currentNote << " (velocity: 80)" << std::endl;
            noteOn = true;
        } else {
            // Send Note Off
            midiTest.sendNoteOff(1, currentNote, 0);   // Channel 1, note, velocity 0
            std::cout << "🎵 MIDI Test: Note OFF - " << currentNote << std::endl;
            noteOn = false;
            
            // Move to next note (C major scale)
            currentNote++;
            if (currentNote > 72) {  // C5
                currentNote = 60;    // Reset to C4
            }
        }
        
        lastMidiTime = currentTime;
    }
}

#ifdef ESP32_BUILD
void setup() {
    Serial.begin(115200);
    delay(1000);  // Short delay for serial
    
    std::cout << "=== ESP32 SynthApp Starting ===" << std::endl;
    
    // Initialize MIDI test
    if (midiTest.initialize()) {
        std::cout << "✅ MIDI Test initialized: " << midiTest.getConnectionStatus() << std::endl;
    } else {
        std::cout << "❌ MIDI Test initialization failed!" << std::endl;
    }
    
    app.setup();
}

void loop() {
    app.loop();
    
    // Run MIDI test
    midiTestLoop();
    
    // Update MIDI handler
    midiTest.update();
}
#else
int main() {
    std::cout << "=== Desktop SynthApp Starting ===" << std::endl;
    
    // Initialize MIDI test with debug info
    std::cout << "🔧 Initializing MIDI handler..." << std::endl;
    if (midiTest.initialize()) {
        std::cout << "✅ MIDI Test initialized: " << midiTest.getConnectionStatus() << std::endl;
        std::cout << "💡 Check 'aconnect -l' in another terminal to see the new MIDI port" << std::endl;
        std::cout << "💡 You can also try: pw-link --list-ports | grep -i midi" << std::endl;
    } else {
        std::cout << "❌ MIDI Test initialization failed!" << std::endl;
        std::cout << "💡 Make sure ALSA development libraries are installed:" << std::endl;
        std::cout << "   sudo apt-get install libasound2-dev" << std::endl;
    }
    
    app.setup();
    
    while (true) {
        app.loop();
        
        // Run MIDI test
        midiTestLoop();
        
        // Update MIDI handler
        midiTest.update();
    }
    
    return 0;
}
#endif