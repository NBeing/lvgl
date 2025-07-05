#!/bin/bash

# Test script for ESP32 MIDI over USB
# Usage: ./scripts/test_esp32.sh

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🔌 ESP32 USB MIDI Test Suite${NC}"
echo -e "${BLUE}============================${NC}"
echo ""

cd "$PROJECT_DIR"

# Step 1: Check if ESP32 is connected
echo -e "${YELLOW}🔍 Checking for connected ESP32...${NC}"
if ! pio device list | grep -q "ESP32\|CP21\|CH34\|FT232"; then
    echo -e "${RED}❌ No ESP32 device found!${NC}"
    echo "   Make sure your ESP32 is connected via USB"
    echo "   Available devices:"
    pio device list
    exit 1
fi

echo -e "${GREEN}✅ ESP32 device detected${NC}"
pio device list | grep -E "ESP32|CP21|CH34|FT232" | head -3
echo ""

# Step 2: Build the ESP32 firmware
echo -e "${YELLOW}🔨 Building ESP32 firmware...${NC}"
pio run -e rymcu-esp32-s3-devkitc-1

if [ $? -ne 0 ]; then
    echo -e "${RED}❌ ESP32 build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}✅ ESP32 build successful${NC}"
echo ""

# Step 3: Upload firmware
echo -e "${YELLOW}📤 Uploading firmware to ESP32...${NC}"
pio run -e rymcu-esp32-s3-devkitc-1 -t upload

if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Upload failed!${NC}"
    echo "   Try pressing the BOOT button on your ESP32 during upload"
    exit 1
fi

echo -e "${GREEN}✅ Upload successful${NC}"
echo ""

# Step 4: Wait for ESP32 to boot and enumerate USB MIDI
echo -e "${YELLOW}⏳ Waiting for ESP32 to boot and enumerate USB MIDI...${NC}"
sleep 5

# Step 5: Check for USB MIDI device
echo -e "${YELLOW}🎵 Checking for USB MIDI device...${NC}"

# Check with aconnect (ALSA MIDI)
echo "📋 ALSA MIDI ports:"
aconnect -l

# Look for ESP32 MIDI device
ESP32_MIDI_PORT=""
if aconnect -l | grep -i "esp32\|synth\|midi" | grep -v "FLUID"; then
    ESP32_MIDI_PORT=$(aconnect -l | grep -i "esp32\|synth" | grep -v "FLUID" | head -1 | cut -d: -f1 | grep -o '[0-9]*' || true)
    if [ -n "$ESP32_MIDI_PORT" ]; then
        echo -e "${GREEN}✅ Found ESP32 MIDI device at port $ESP32_MIDI_PORT${NC}"
    fi
else
    echo -e "${YELLOW}⚠️  No ESP32 MIDI device found via ALSA${NC}"
fi

echo ""

# Also check USB devices
echo -e "${YELLOW}🔍 Checking USB devices...${NC}"
echo "📋 USB devices:"
lsusb | grep -E "Espressif|ESP32" || echo "   No ESP32 USB devices found"

# Check if there are any new MIDI devices
echo ""
echo "📋 All current MIDI devices:"
if command -v amidi &> /dev/null; then
    amidi -l || echo "   No raw MIDI devices found"
else
    echo "   (amidi not available - install alsa-utils for more detailed USB MIDI info)"
fi

echo ""

# Step 6: Monitor serial output
echo -e "${YELLOW}📺 Starting serial monitor...${NC}"
echo -e "${BLUE}💡 You should see MIDI test messages in the serial output${NC}"
echo -e "${BLUE}💡 Press Ctrl+C to stop monitoring${NC}"
echo ""

# Start monitoring with timeout
timeout 30s pio device monitor -e rymcu-esp32-s3-devkitc-1 || true

echo ""
echo -e "${GREEN}🎉 ESP32 test complete!${NC}"
echo ""
echo -e "${BLUE}📋 Summary:${NC}"
echo -e "   • ESP32 firmware built and uploaded ✅"
echo -e "   • Serial output monitored for 30 seconds ✅"
if [ -n "$ESP32_MIDI_PORT" ]; then
    echo -e "   • USB MIDI device detected at port $ESP32_MIDI_PORT ✅"
    echo ""
    echo -e "${GREEN}🎵 To test MIDI output from ESP32:${NC}"
    echo -e "   aseqdump -p $ESP32_MIDI_PORT:0"
else
    echo -e "   • USB MIDI device status: Not detected ⚠️"
    echo ""
    echo -e "${YELLOW}🔧 Troubleshooting USB MIDI:${NC}"
    echo -e "   1. Check if ESP32 supports USB MIDI (ESP32-S2/S3/C3 do)"
    echo -e "   2. Verify Control-Surface library is properly configured"
    echo -e "   3. Try unplugging and reconnecting the ESP32"
    echo -e "   4. Check serial output for initialization messages"
    echo -e "   5. Some ESP32 boards need native USB port (not UART)"
fi

echo ""
echo -e "${BLUE}💡 Next steps:${NC}"
echo -e "   • Run './scripts/test_synth.sh' to test desktop version"
echo -e "   • Connect ESP32 MIDI to FluidSynth with './scripts/connect_synth.sh'"
echo -e "   • Monitor both platforms: 'aseqdump -p <port>'"
