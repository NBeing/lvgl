/**
 * Control-Surface USB MIDI test - uses Control-Surface's built-in TinyUSB implementation
 * This should work with Arduino 2.x since Control-Surface provides its own TinyUSB layer
 */
#include <Arduino.h>

#if defined(ESP32_BUILD)
#include <Control_Surface.h>

// Use Control-Surface's USB MIDI interface (this handles TinyUSB internally)
USBMIDI_Interface midi;

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("=== Control-Surface USB MIDI Test ===");
    
    // Initialize the USB MIDI interface first
    midi.begin();
    Serial.println("✅ USB MIDI interface initialized!");
    
    // Initialize Control-Surface (this will set up the rest)
    Control_Surface.begin();
    
    Serial.println("✅ Control-Surface USB MIDI initialized!");
    Serial.println("Device should appear as USB MIDI device");
    Serial.println("Check with: lsusb && aconnect -l");
}

void loop() {
    // Update Control-Surface (handles USB communication)
    Control_Surface.loop();
    
    // Send a test note every 2 seconds
    static unsigned long last_note = 0;
    static bool note_state = false;
    static int note = 60; // C4
    
    if (millis() - last_note > 2000) {
        last_note = millis();
        
        if (note_state) {
            // Send Note Off via Control-Surface
            Control_Surface.sendNoteOff({note, Channel_1}, 64);  // velocity = 64
            Serial.printf("🎵 USB MIDI Note OFF: %d\n", note);
            note_state = false;
        } else {
            // Send Note On via Control-Surface  
            Control_Surface.sendNoteOn({note, Channel_1}, 127);
            Serial.printf("🎵 USB MIDI Note ON: %d\n", note);
            note_state = true;
        }
    }
    
    // Small delay to prevent flooding
    delay(1);
}

#else
// Desktop version - just print that it's not supported
void setup() {
    Serial.begin(115200);
    Serial.println("USB MIDI test not supported on desktop - use Serial MIDI test instead");
}

void loop() {
    delay(1000);
}
#endif
