/**
 * Simple Serial MIDI test - should work with Arduino 2.x
 * This replicates your working approach but over Serial instead of USB
 */
#include <Arduino.h>

#if defined(ESP32_BUILD)
#include <MIDI.h>

// Create MIDI instance on Serial2 
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI);

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("=== Simple Serial MIDI Test ===");
    
    // Initialize Serial2 for MIDI (pins 16=RX, 17=TX)
    Serial2.begin(31250, SERIAL_8N1, 16, 17);
    
    // Initialize MIDI library
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.turnThruOff(); // Disable MIDI thru
    
    Serial.println("✅ Serial MIDI initialized!");
    Serial.println("Connect MIDI device to pins 16(RX)/17(TX)");
    Serial.println("Sending test notes...");
}

void loop() {
    // Send a test note every 2 seconds
    static unsigned long last_note = 0;
    static bool note_state = false;
    static int note = 60; // C4
    
    if (millis() - last_note > 2000) {
        last_note = millis();
        
        if (note_state) {
            // Send Note Off
            MIDI.sendNoteOff(note, 0, 1);
            Serial.printf("🎵 Note OFF: %d\n", note);
            note_state = false;
        } else {
            // Send Note On
            MIDI.sendNoteOn(note, 127, 1);
            Serial.printf("🎵 Note ON: %d\n", note);
            note_state = true;
        }
    }
    
    // Read any incoming MIDI messages
    if (MIDI.read()) {
        switch(MIDI.getType()) {
            case midi::NoteOn:
                Serial.printf("📥 Received Note ON: %d, velocity: %d, channel: %d\n", 
                             MIDI.getData1(), MIDI.getData2(), MIDI.getChannel());
                break;
            case midi::NoteOff:
                Serial.printf("📥 Received Note OFF: %d, channel: %d\n", 
                             MIDI.getData1(), MIDI.getChannel());
                break;
            case midi::ControlChange:
                Serial.printf("📥 Received CC: %d = %d, channel: %d\n", 
                             MIDI.getData1(), MIDI.getData2(), MIDI.getChannel());
                break;
            default:
                Serial.printf("📥 Received MIDI: Type %d\n", MIDI.getType());
                break;
        }
    }
    
    delay(10); // Small delay for stability
}

#else
// Desktop stub
void setup() {
    printf("Serial MIDI test - ESP32 only\n");
}

void loop() {
    // Nothing on desktop
}
#endif
