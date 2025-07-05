#!/bin/bash

# Universal MIDI monitor and router
# Usage: ./scripts/monitor_midi.sh [port_number]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🎵 Universal MIDI Monitor${NC}"
echo -e "${BLUE}=========================${NC}"
echo ""

# Show all available MIDI ports
echo -e "${YELLOW}📋 Available MIDI ports:${NC}"
aconnect -l

echo ""

# If a port is specified, monitor it directly
if [ -n "$1" ]; then
    MONITOR_PORT="$1"
    echo -e "${GREEN}🔍 Monitoring specified port: $MONITOR_PORT${NC}"
else
    # Find interesting MIDI sources
    echo -e "${YELLOW}🔍 Looking for MIDI sources...${NC}"
    
    # Look for ESP32, RtMidi, or other interesting sources
    SOURCES=$(aconnect -l | grep -E "client [0-9]+:" | grep -E "ESP32|RtMidi|Synth" | grep -v "FLUID")
    
    if [ -z "$SOURCES" ]; then
        echo -e "${RED}❌ No interesting MIDI sources found${NC}"
        echo "   Available sources:"
        aconnect -l | grep -E "client [0-9]+:" | grep -v "System\|PipeWire\|FLUID"
        exit 1
    fi
    
    echo "🎯 Found MIDI sources:"
    echo "$SOURCES"
    
    # Auto-select the first interesting source
    MONITOR_PORT=$(echo "$SOURCES" | head -1 | cut -d: -f1 | grep -o '[0-9]*')
    echo ""
    echo -e "${GREEN}🔧 Auto-selected port: $MONITOR_PORT${NC}"
fi

# Verify the port exists
if ! aconnect -l | grep -q "^client $MONITOR_PORT:"; then
    echo -e "${RED}❌ Port $MONITOR_PORT not found!${NC}"
    exit 1
fi

# Show port details
PORT_NAME=$(aconnect -l | grep "^client $MONITOR_PORT:" | cut -d"'" -f2)
echo -e "${BLUE}📡 Monitoring: Port $MONITOR_PORT ($PORT_NAME)${NC}"

# Check if FluidSynth is running
FLUID_PORT=$(aconnect -l | grep "FLUID Synth" | head -1 | cut -d: -f1 | grep -o '[0-9]*' || true)

if [ -n "$FLUID_PORT" ]; then
    echo -e "${GREEN}🎵 FluidSynth detected at port: $FLUID_PORT${NC}"
    echo -e "${YELLOW}🔗 Auto-connecting to FluidSynth...${NC}"
    
    # Connect to FluidSynth
    aconnect -d $MONITOR_PORT:0 $FLUID_PORT:0 2>/dev/null || true
    aconnect $MONITOR_PORT:0 $FLUID_PORT:0
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✅ Connected to FluidSynth for audio output${NC}"
    else
        echo -e "${RED}❌ Failed to connect to FluidSynth${NC}"
    fi
else
    echo -e "${YELLOW}⚠️  FluidSynth not running - no audio output${NC}"
    echo "   Start FluidSynth: fluidsynth -a alsa /usr/share/sounds/sf2/FluidR3_GM.sf2"
fi

echo ""
echo -e "${BLUE}👂 Live MIDI monitoring (Press Ctrl+C to stop):${NC}"
echo -e "${BLUE}================================================${NC}"

# Monitor MIDI events
aseqdump -p $MONITOR_PORT:0
