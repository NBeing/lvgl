# MIDI Test System

Automated MIDI testing tools for the LVGL Synthesizer.

## Overview

The MIDI test system provides automated testing of the synthesizer's MIDI functionality by:
- **Auto-connecting** to exposed MIDI ports
- **Sending comprehensive test sequences** 
- **Monitoring MIDI feedback**
- **Stress testing** parameter changes
- **Validating musical functionality**

## Files

- `scripts/midi_test_client.py` - Python MIDI test client
- `scripts/test_midi.sh` - Shell wrapper script
- `scripts/monitor_midi.sh` - MIDI monitoring utility (existing)

## Usage

### Quick Test
```bash
# Start the synthesizer first
.pio/build/desktop/program &

# Run all tests
./scripts/test_midi.sh
```

### Specific Tests
```bash
# Parameter sweep test only
./scripts/test_midi.sh --test sweep

# Stress test (10 seconds)
./scripts/test_midi.sh --test stress --duration 10

# Musical note sequences
./scripts/test_midi.sh --test musical

# Verbose output
./scripts/test_midi.sh --verbose
```

### Direct Python Usage
```bash
# Install dependencies
pip3 install python-rtmidi

# Run comprehensive tests
python3 scripts/midi_test_client.py

# Run specific test with custom duration
python3 scripts/midi_test_client.py --test stress --duration 30 --verbose
```

## Test Scenarios

### 1. Parameter Sweep Test
- Tests all synthesizer parameters (Filter, Envelope, Volume, etc.)
- Sweeps each parameter from 0-127 and back
- Validates parameter responsiveness

### 2. Stress Test  
- Sends rapid parameter changes (10ms intervals)
- Tests system stability under high MIDI load
- Configurable duration

### 3. Musical Test
- Plays musical scales and chord progressions
- Tests note on/off functionality
- Validates musical timing

### 4. Feedback Monitoring
- Listens for MIDI feedback from synthesizer
- Validates bidirectional MIDI communication
- Reports message statistics

## Requirements

- **Python 3.6+**
- **python-rtmidi** package
- **Running LVGL synthesizer** with exposed MIDI ports

## Expected MIDI Ports

The test client looks for these ports created by the synthesizer:
- **Output**: `LVGL Synth Output` (where we send commands)
- **Input**: `RtMidi Input` (where we receive feedback)

## Test Parameters

The system tests these synthesizer parameters:

| Parameter | CC# | Description |
|-----------|-----|-------------|
| Filter Cutoff | 74 | Low-pass filter frequency |
| Filter Resonance | 71 | Filter resonance/Q |
| Envelope Attack | 73 | Amplitude envelope attack time |
| Master Volume | 7 | Overall output level |
| Clock BPM | 3 | Tempo/clock speed |

## Integration with CI/CD

The test system can be integrated into automated testing:

```bash
# Start synthesizer in background
.pio/build/desktop/program &
SYNTH_PID=$!

# Wait for startup
sleep 2

# Run tests
if ./scripts/test_midi.sh --test all; then
    echo "✅ MIDI tests passed"
    EXIT_CODE=0
else
    echo "❌ MIDI tests failed" 
    EXIT_CODE=1
fi

# Cleanup
kill $SYNTH_PID
exit $EXIT_CODE
```

## Troubleshooting

### "LVGL Synthesizer not detected"
- Make sure the synthesizer is running: `.pio/build/desktop/program`
- Check MIDI ports with: `python3 -c "import rtmidi; print(rtmidi.MidiOut().get_ports())"`

### "python-rtmidi not found"
- Install with: `pip3 install python-rtmidi`
- On Ubuntu: `sudo apt install python3-rtmidi`

### No MIDI feedback received
- This is normal if the synthesizer doesn't send feedback
- Tests will still validate parameter sending

## Contributing

To add new test scenarios:

1. Add test methods to `MidiTestClient` class
2. Add command-line options in `main()`
3. Update this documentation

Example:
```python
def run_my_custom_test(self):
    """My custom test description"""
    self.log("🧪 Starting custom test...")
    # Your test logic here
```
