#!/bin/bash

# MIDI Test Script for ESP32 Input Testing
# Usage: ./test_midi_input.sh

MIDI_PORT="hw:1,0"  # Midihub MH-30EM451 port

echo "🎵 MIDI Input Test for ESP32 Pin 46"
echo "Sending to port: $MIDI_PORT"
echo "Watch ESP32 monitor for received messages..."
echo

# Test Control Change Messages (matching your synthesizer parameters)
echo "📡 Sending Control Change messages..."

echo "  CC74 (Filter Cutoff) = 64"
amidi -p $MIDI_PORT -S "B0 4A 40"
sleep 1

echo "  CC71 (Resonance) = 100"  
amidi -p $MIDI_PORT -S "B0 47 64"
sleep 1

echo "  CC07 (Master Volume) = 127"
amidi -p $MIDI_PORT -S "B0 07 7F"
sleep 1

echo "  CC73 (Attack) = 50"
amidi -p $MIDI_PORT -S "B0 49 32"
sleep 1

echo "  CC72 (Release) = 80"
amidi -p $MIDI_PORT -S "B0 48 50"
sleep 1

echo "  CC76 (LFO Rate) = 90"
amidi -p $MIDI_PORT -S "B0 4C 5A"
sleep 1

# Test Note Messages
echo
echo "📡 Sending Note messages..."

echo "  Note On: C4 (60) velocity 100"
amidi -p $MIDI_PORT -S "90 3C 64"
sleep 0.5

echo "  Note Off: C4 (60)"
amidi -p $MIDI_PORT -S "80 3C 00"
sleep 1

# Test Program Change
echo
echo "📡 Sending Program Change..."
echo "  Program Change: 10"
amidi -p $MIDI_PORT -S "C0 0A"
sleep 1

# Test Pitch Bend
echo
echo "📡 Sending Pitch Bend..."
echo "  Pitch Bend: Center (8192)"
amidi -p $MIDI_PORT -S "E0 00 40"
sleep 1

echo
echo "✅ MIDI test complete!"
echo "Check ESP32 monitor for '[Hardware MIDI] Received:' messages"
