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

// MIDI handler instance
MidiHandler midi_handler;
#ifdef ESP32_BUILD
#define MIDI_IN_PIN 5
#define OPTO_INPUT_PIN 6

void setup() {
    Serial.begin(115200);
    delay(5000);  // Short delay for serial
    pinMode(MIDI_IN_PIN, INPUT);    
    app.setup();
}

void loop() {
    app.loop();
    midi_handler.update();
}
#else
int main() {
    std::cout << "=== Desktop SynthApp Starting ===" << std::endl;
    
    // Initialize MIDI test with debug info
    std::cout << "🔧 Initializing MIDI handler..." << std::endl;
    if (midi_handler.initialize()) {
        std::cout << "✅ MIDI Test initialized: " << midi_handler.getConnectionStatus() << std::endl;
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
                
        // Update MIDI handler
        midi_handler.update();
    }
    
    return 0;
}
#endif