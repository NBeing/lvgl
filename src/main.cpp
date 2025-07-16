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
    app.setup();
    
    while (true) {
        app.loop();
                
        // Update MIDI handler
        midi_handler.update();
    }
    
    return 0;
}
#endif