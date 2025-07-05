// Temporary MIDI test using EspTinyUSB directly
#include <Arduino.h>
#include "EspTinyUSB.h"
#include "midiusb.h"

MIDIusb midi;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== EspTinyUSB MIDI Test ===");
    
    // For ESP32-S3, set base endpoint to 3 (as per your working Arduino project)
    midi.setBaseEP(3);
    
    // Initialize with device name that should appear in aconnect -l
    char device_name[] = "ESP32S3_DEV MIDI 1";
    if (midi.begin(device_name)) {
        Serial.println("✅ MIDI USB device initialized successfully!");
        Serial.println("💡 Check with: lsusb && aconnect -l");
        Serial.println("💡 Should appear as 'ESP32S3_DEV MIDI 1' in MIDI devices");
    } else {
        Serial.println("❌ MIDI USB device initialization failed!");
    }
    
    delay(2000);
}

void loop() {
    // Send a test note every 1 seconds
    static unsigned long last_note = 0;
    static bool note_state = false;
    static int note = 60; // C4
    
    if (millis() - last_note > 1000) {
        last_note = millis();
        
        if (note_state) {
            midi.noteON(note, 80, 0);  // Note, velocity 80, channel 0
            Serial.print("🎵 Note ON: ");
            Serial.println(note);
        } else {
            midi.noteOFF(note, 0, 0);   // Note, channel 0
            Serial.print("🎵 Note OFF: ");
            Serial.println(note);
            
            // Move to next note in C major scale
            note++;
            if (note > 72) note = 60; // Reset to C4 after C5
        }
        note_state = !note_state;
    }
    
    delay(10);
}
