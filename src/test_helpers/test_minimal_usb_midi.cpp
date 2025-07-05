/**
 * Minimal USB MIDI test for ESP32-S3
 * Just initializes USB MIDI - no complex functionality
 * Check with: lsusb && aconnect -l
 */
#include <Arduino.h>

#if defined(ESP32_BUILD)
#include <Control_Surface.h>

// Create USB MIDI interface - this should make ESP32-S3 appear as USB MIDI device
USBMIDI_Interface midi;

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("=== Minimal USB MIDI Test ===");
    Serial.println("Initializing Control-Surface USB MIDI...");
    
    // Initialize Control-Surface with USB MIDI
    Control_Surface.begin();
    
    Serial.println("✅ USB MIDI initialized!");
    Serial.println("Check with: lsusb && aconnect -l");
    Serial.println("Device should appear as USB MIDI device");
}

void loop() {
    // Just update Control-Surface - no MIDI sending
    Control_Surface.loop();
    delay(100);
}

#else
// Desktop fallback - do nothing
void setup() {}
void loop() {}
#endif
