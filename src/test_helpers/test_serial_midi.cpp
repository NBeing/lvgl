/**
 * Serial MIDI Test using Control-Surface (Arduino 2.x compatible)
 * This should work with your current platform setup
 */
#include <Arduino.h>

#if defined(ESP32_BUILD)
#include <Control_Surface.h>

// Create MIDI interface over Serial2 (pins 16=RX, 17=TX on ESP32-S3)
HardwareSerialMIDI_Interface midi = Serial2;

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("=== Serial MIDI Test (Control-Surface) ===");
    
    // Initialize Serial2 for MIDI communication
    Serial2.begin(31250, SERIAL_8N1, 16, 17); // Standard MIDI baud rate
    
    // Initialize Control Surface
    Control_Surface.begin();
    
    Serial.println("✅ Serial MIDI interface initialized!");
    Serial.println("Connect MIDI device to pins 16 (RX) and 17 (TX)");
    Serial.println("Or use USB-to-MIDI adapter with Serial2");
}

void loop() {
    // Update Control Surface (processes incoming MIDI)
    Control_Surface.loop();
    
    // Send a test note every 3 seconds
    static unsigned long last_note = 0;
    static bool note_state = false;
    static int note = 60; // C4
    
    if (millis() - last_note > 3000) {
        last_note = millis();
        
        if (note_state) {
            // Send note off
            midi.sendNoteOff({note, CHANNEL_1});
            Serial.printf("📤 MIDI Note OFF: %d\n", note);
            note_state = false;
        } else {
            // Send note on
            midi.sendNoteOn({note, CHANNEL_1}, 100);
            Serial.printf("📤 MIDI Note ON: %d\n", note);
            note_state = true;
            
            // Cycle through notes
            note++;
            if (note > 72) note = 60; // C4 to C5
        }
    }
    
    delay(10);
}

#else
// Desktop version - just print messages
void setup() {
    Serial.begin(115200);
    Serial.println("=== Desktop Version (No MIDI) ===");
}

void loop() {
    static unsigned long last_msg = 0;
    if (millis() - last_msg > 5000) {
        last_msg = millis();
        Serial.println("Desktop version running...");
    }
    delay(100);
}
#endif
