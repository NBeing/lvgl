#!/bin/bash

# Test script for LVGL Synth - starts everything and connects automatically
# Usage: ./scripts/test_synth.sh

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# PIDs to track for cleanup
DESKTOP_PID=""
FLUIDSYNTH_PID=""

# Cleanup function
cleanup() {
    echo ""
    echo -e "${YELLOW}🧹 Cleaning up...${NC}"
    
    if [ -n "$DESKTOP_PID" ] && kill -0 "$DESKTOP_PID" 2>/dev/null; then
        echo -e "${BLUE}   Stopping desktop app (PID: $DESKTOP_PID)${NC}"
        kill "$DESKTOP_PID" 2>/dev/null || true
        wait "$DESKTOP_PID" 2>/dev/null || true
    fi
    
    if [ -n "$FLUIDSYNTH_PID" ] && kill -0 "$FLUIDSYNTH_PID" 2>/dev/null; then
        echo -e "${BLUE}   Stopping FluidSynth (PID: $FLUIDSYNTH_PID)${NC}"
        kill "$FLUIDSYNTH_PID" 2>/dev/null || true
        wait "$FLUIDSYNTH_PID" 2>/dev/null || true
    fi
    
    echo -e "${GREEN}✅ Cleanup complete${NC}"
}

# Set up signal handlers
trap cleanup EXIT INT TERM

echo -e "${BLUE}🎹 LVGL Synth Test Suite${NC}"
echo -e "${BLUE}=========================${NC}"
echo ""

# Check if FluidSynth is available
if ! command -v fluidsynth &> /dev/null; then
    echo -e "${RED}❌ FluidSynth not found!${NC}"
    echo "   Install with: sudo apt install fluidsynth fluid-soundfont-gm"
    exit 1
fi

# Check if we have a soundfont
SOUNDFONT=""
SOUNDFONT_PATHS=(
    "/usr/share/sounds/sf2/FluidR3_GM.sf2"
    "/usr/share/sounds/sf2/default.sf2"
    "/usr/share/soundfonts/FluidR3_GM.sf2"
    "/usr/share/soundfonts/default.sf2"
)

for sf in "${SOUNDFONT_PATHS[@]}"; do
    if [ -f "$sf" ]; then
        SOUNDFONT="$sf"
        break
    fi
done

if [ -z "$SOUNDFONT" ]; then
    echo -e "${RED}❌ No soundfont found!${NC}"
    echo "   Install with: sudo apt install fluid-soundfont-gm"
    exit 1
fi

echo -e "${GREEN}🔊 Using soundfont: $SOUNDFONT${NC}"
echo ""

# Step 1: Build the desktop app
echo -e "${YELLOW}🔨 Building desktop app...${NC}"
cd "$PROJECT_DIR"
pio run -e desktop

if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Build successful${NC}"
echo ""

# Step 2: Start FluidSynth in background
echo -e "${YELLOW}🎵 Starting FluidSynth...${NC}"
fluidsynth -a alsa -g 0.5 -o synth.cpu-cores=2 --server --no-shell "$SOUNDFONT" &
FLUIDSYNTH_PID=$!

# Give FluidSynth time to start
sleep 2

if ! kill -0 "$FLUIDSYNTH_PID" 2>/dev/null; then
    echo -e "${RED}❌ FluidSynth failed to start!${NC}"
    exit 1
fi

echo -e "${GREEN}✅ FluidSynth started (PID: $FLUIDSYNTH_PID)${NC}"
echo ""

# Step 3: Start the desktop app in background
echo -e "${YELLOW}🖥️  Starting desktop app...${NC}"
./.pio/build/desktop/program &
DESKTOP_PID=$!

# Give the app time to start and create MIDI ports
sleep 3

if ! kill -0 "$DESKTOP_PID" 2>/dev/null; then
    echo -e "${RED}❌ Desktop app failed to start!${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Desktop app started (PID: $DESKTOP_PID)${NC}"
echo ""

# Step 4: Connect MIDI ports
echo -e "${YELLOW}🔌 Connecting MIDI ports...${NC}"
"$SCRIPT_DIR/connect_synth.sh" --auto

if [ $? -ne 0 ]; then
    echo -e "${RED}❌ MIDI connection failed!${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}🎉 Test setup complete!${NC}"
echo -e "${BLUE}================================================${NC}"
echo -e "${GREEN}✅ Desktop app is running and sending MIDI notes${NC}"
echo -e "${GREEN}✅ FluidSynth is running and should be playing sound${NC}"
echo -e "${GREEN}✅ MIDI ports are connected automatically${NC}"
echo ""
echo -e "${YELLOW}🎵 You should hear notes playing every second!${NC}"
echo ""
echo -e "${BLUE}💡 Tips:${NC}"
echo -e "   • Press Ctrl+C to stop everything and cleanup"
echo -e "   • Check volume if you don't hear sound"
echo -e "   • Monitor MIDI with: aseqdump -p <port>"
echo ""
echo -e "${YELLOW}⏳ Running... (Press Ctrl+C to stop)${NC}"

# Keep the script running until interrupted
while true; do
    sleep 1
    # Check if processes are still running
    if ! kill -0 "$DESKTOP_PID" 2>/dev/null; then
        echo -e "${RED}❌ Desktop app stopped unexpectedly!${NC}"
        break
    fi
    if ! kill -0 "$FLUIDSYNTH_PID" 2>/dev/null; then
        echo -e "${RED}❌ FluidSynth stopped unexpectedly!${NC}"
        break
    fi
done
