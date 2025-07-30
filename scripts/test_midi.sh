#!/bin/bash
#
# MIDI Test Runner for LVGL Synthesizer
# =====================================
# 
# Automatically runs MIDI tests against the running synthesizer.
# Make sure the synthesizer is running before executing this script.
#

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." &> /dev/null && pwd )"

echo "🎹 LVGL Synthesizer MIDI Test Runner"
echo "===================================="

# Check if python3 is available
if ! command -v python3 &> /dev/null; then
    echo "❌ python3 is required but not installed"
    exit 1
fi

# Check if python-rtmidi is installed
if ! python3 -c "import rtmidi" 2>/dev/null; then
    echo "📦 Installing python-rtmidi..."
    pip3 install python-rtmidi
fi

# Check if synthesizer is running by looking for MIDI ports
echo "🔍 Checking for LVGL Synthesizer MIDI ports..."
if ! python3 -c "
import rtmidi
midi_in = rtmidi.MidiIn()
ports = midi_in.get_ports()
found = any('LVGL Synth Output' in port for port in ports)
midi_in.close_port()
if not found:
    print('❌ LVGL Synthesizer not detected!')
    print('Available input ports:', ports)
    print('Please start the synthesizer first: .pio/build/desktop/program')
    exit(1)
else:
    print('✅ LVGL Synthesizer detected!')
"; then
    exit 1
fi

# Run the test client
echo "🚀 Starting MIDI tests..."
cd "$PROJECT_ROOT"

# If no arguments provided, show usage
if [ $# -eq 0 ]; then
    python3 scripts/midi_test_client.py
else
    python3 scripts/midi_test_client.py "$@"
fi

echo "✅ MIDI testing completed!"
