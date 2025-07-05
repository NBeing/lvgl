#!/bin/bash

# Parse command line arguments
AUTO_MODE=false
if [ "$1" = "--auto" ]; then
    AUTO_MODE=true
fi

echo "🎹 Connecting LVGL Synth to FluidSynth..."

# Show current connections first
echo "📋 Current MIDI connections:"
aconnect -l

# Find RtMidi output ports
echo "🔍 Searching for RtMidi ports..."
RTMIDI_PORTS=$(aconnect -l | grep "RtMidi Output Client" | cut -d: -f1 | grep -o '[0-9]*')
RTMIDI_COUNT=$(echo "$RTMIDI_PORTS" | wc -l)

if [ -z "$RTMIDI_PORTS" ]; then
    echo "❌ No RtMidi ports found!"
    echo "   Make sure the LVGL app is running"
    exit 1
fi

echo "📋 Found $RTMIDI_COUNT RtMidi port(s):"
echo "$RTMIDI_PORTS" | while read -r port; do
    echo "   Port $port: $(aconnect -l | grep "^client $port:" | cut -d"'" -f2)"
done

# Auto-select if only one port, otherwise ask user (unless in auto mode)
if [ "$RTMIDI_COUNT" -eq 1 ]; then
    RTMIDI_PORT=$(echo "$RTMIDI_PORTS" | head -1)
    echo "🔧 Auto-selected RtMidi port: $RTMIDI_PORT"
elif [ "$AUTO_MODE" = true ]; then
    # In auto mode, pick the last (newest) port
    RTMIDI_PORT=$(echo "$RTMIDI_PORTS" | tail -1)
    echo "🔧 Auto-selected RtMidi port: $RTMIDI_PORT (last created)"
else
    echo ""
    echo "🔧 Multiple RtMidi ports found. Which one should we use?"
    echo "   (Usually the last one created is the active output)"
    echo -n "Enter port number: "
    read RTMIDI_PORT
    
    # Validate the input
    if ! echo "$RTMIDI_PORTS" | grep -q "^$RTMIDI_PORT$"; then
        echo "❌ Invalid port number: $RTMIDI_PORT"
        exit 1
    fi
fi

# Find FluidSynth input port  
echo "🔍 Searching for FluidSynth port..."
FLUID_PORT=$(aconnect -l | grep "FLUID Synth" | head -1 | cut -d: -f1 | grep -o '[0-9]*')

echo ""
echo "🔍 Selected ports:"
echo "   RtMidi port: $RTMIDI_PORT"
echo "   FluidSynth port: $FLUID_PORT"

if [ -n "$RTMIDI_PORT" ] && [ -n "$FLUID_PORT" ]; then
    # Disconnect any existing connections first
    echo "🔌 Disconnecting any existing connections..."
    aconnect -d $RTMIDI_PORT:0 $FLUID_PORT:0 2>/dev/null || true
    
    # Make the connection
    echo "🔌 Connecting RtMidi ($RTMIDI_PORT:0) to FluidSynth ($FLUID_PORT:0)"
    aconnect $RTMIDI_PORT:0 $FLUID_PORT:0
    
    if [ $? -eq 0 ]; then
        echo "✅ Connected! You should now hear sound from the LVGL Synth!"
        echo ""
        echo "🎵 Test: In FluidSynth terminal, you can verify with:"
        echo "   > noteon 0 60 80"
        echo "   > noteoff 0 60"
    else
        echo "❌ Connection failed!"
    fi
    
    echo ""
    echo "📋 New connections:"
    aconnect -l | grep -A5 -B5 "RtMidi\|FLUID"
else
    echo "❌ Could not find MIDI ports"
    echo "   Make sure both FluidSynth and the LVGL app are running"
    echo ""
    echo "📋 Available ports:"
    aconnect -l
fi