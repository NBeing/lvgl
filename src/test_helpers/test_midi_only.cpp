/**
 * Simple EspTinyUSB MIDI test - matches your working Arduino approach
 * This should work with Arduino 2.x + your platform config
 */
#include <Arduino.h>

#if defined(ESP32_BUILD)
#include "EspTinyUSB.h"
#include "midiusb.h"

MIDIusb midi;

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("=== EspTinyUSB MIDI Test ===");
    
    // For ESP32-S3, set base endpoint to 3 (as per your working project)
    midi.setBaseEP(3);
    
    // Initialize with device name
    char device_name[] = "LVGL ESP32S3 MIDI";
    if (midi.begin(device_name)) {
        Serial.println("✅ MIDI USB device initialized successfully!");
        Serial.println("Check with: lsusb && aconnect -l");
    } else {
        Serial.println("❌ MIDI USB device initialization failed!");
    }
}

void loop() {
    // Send a test note every 2 seconds
    static unsigned long last_note = 0;
    static bool note_state = false;
    static int note = 60; // C4
    
    if (millis() - last_note > 2000) {
        last_note = millis();
        
        if (note_state) {
            midi.noteON(note, 100, 0);  // Note, velocity 100, channel 0
            Serial.printf("🎵 Note ON: %d\n", note);
        } else {
            midi.noteOFF(note, 0, 0);   // Note, channel 0
            Serial.printf("🎵 Note OFF: %d\n", note);
            note = (note < 72) ? note + 1 : 60; // Cycle through C4-C5
        }
        note_state = !note_state;
    }
    
    delay(10);
}

#else
// Desktop fallback
#include <iostream>
int main() {
    std::cout << "This test is for ESP32 only" << std::endl;
    return 0;
}
#endif
