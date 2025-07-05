# LVGL Synthesizer Controller

A cross-platform MIDI synthesizer controller built with LVGL, supporting both desktop simulation and ESP32-S3 hardware.

## Features

🎹 **Cross-Platform MIDI**: Real MIDI output on both desktop and ESP32  
🖥️ **Desktop Simulation**: Fast development with SDL2 and RtMidi  
📱 **ESP32-S3 Hardware**: Touchscreen display with USB MIDI output  
🎛️ **Synthesizer UI**: Knobs, sliders, and controls for real-time parameter control  
🔧 **Automated Builds**: PlatformIO with automated dependency management  

## Hardware Targets

### Desktop (Linux)
- **Display**: SDL2 window simulation
- **MIDI**: RtMidi with ALSA backend
- **Development**: Fast UI iteration and testing

### ESP32-S3 DevKit
- **Display**: ST7796S TFT with LovyanGFX
- **MIDI**: USB MIDI via Control-Surface library
- **Hardware**: ESP32-S3 with PSRAM support

## Dependencies

### Desktop Dependencies
- **LVGL 9.3.0**: UI framework
- **RtMidi 6.0.0**: Cross-platform MIDI I/O (auto-installed)
- **SDL2**: Graphics and input
- **ALSA**: Linux audio system

### ESP32 Dependencies  
- **LVGL 9.3.0**: UI framework
- **LovyanGFX 1.2.7**: High-performance display driver
- **Control-Surface 1.2.0**: ESP32 MIDI library
- **TFT_eSPI 2.5.43**: Display driver support

## Quick Start

### Prerequisites

```bash
# Ubuntu/Debian - Install system dependencies
sudo apt-get update
sudo apt-get install build-essential python3-pip
sudo apt-get install libsdl2-dev libasound2-dev  # For desktop build

# Install PlatformIO
python3 -m pip install platformio

# Clone the repository
git clone <your-repo-url>
cd lvgl-synth-controller
```

### Desktop Development

```bash
# RtMidi will be automatically downloaded and installed
pio run -e desktop

# Run the desktop simulator
./.pio/build/desktop/program
```

**Note**: The first build will automatically download and configure RtMidi for Linux. The build script ensures only the necessary files are included (no Android/iOS dependencies).

### ESP32-S3 Hardware

```bash
# Build for ESP32-S3
pio run -e rymcu-esp32-s3-devkitc-1

# Upload to device
pio run -e rymcu-esp32-s3-devkitc-1 -t upload

# Monitor serial output
pio device monitor
```

## Project Structure

```
lvgl-synth-controller/
├── src/                          # Main source code
│   ├── main.cpp                 # Application entry point
│   ├── ui/                      # LVGL UI components
│   ├── hardware/                # Hardware abstraction
│   └── hal/                     # Hardware abstraction layer
│       ├── desktop/             # Desktop-specific code
│       └── esp32/               # ESP32-specific code
├── include/                     # Header files
│   └── lv_conf.h               # LVGL configuration
├── lib/                        # Local libraries
│   └── rtmidi_linux/           # Auto-generated RtMidi (desktop only)
├── scripts/                    # Build automation
│   └── install_rtmidi.py       # RtMidi installer script
└── platformio.ini              # PlatformIO configuration
```

## Build Configuration

### Pinned Dependencies

All dependencies are pinned to specific versions for reproducible builds:

- **LVGL**: `9.3.0`
- **RtMidi**: `6.0.0` (Linux ALSA only)
- **LovyanGFX**: Commit `5438181`
- **Control-Surface**: Commit `f85f2e3`
- **TFT_eSPI**: `2.5.43`

### Environment-Specific Settings

#### Desktop (`[env:desktop]`)
- **Platform**: `native` (Linux x86_64)
- **Build flags**: SDL2, ALSA, RtMidi configuration
- **MIDI Backend**: RtMidi with ALSA
- **Display**: SDL2 window

#### ESP32-S3 (`[env:rymcu-esp32-s3-devkitc-1]`)
- **Platform**: `espressif32@^6.4.0`
- **Board**: ESP32-S3 DevKit with 16MB Flash, 8MB PSRAM
- **MIDI Backend**: Control-Surface USB MIDI
- **Display**: ST7796S via LovyanGFX

## MIDI Output

### Desktop
- **Automatic port detection**: Scans for available MIDI ports
- **Virtual port creation**: Creates "LVGL Synth Controller" if no hardware found
- **DAW integration**: Connect to Ableton Live, FL Studio, Reaper, etc.
- **Hardware support**: USB MIDI interfaces, synthesizers

### ESP32-S3
- **USB MIDI device**: Appears as standard MIDI controller
- **Low latency**: Hardware USB implementation
- **Cross-platform**: Works with any MIDI-capable software

## Development Workflow

### Desktop-First Development
1. **Design UI** on desktop with fast build times
2. **Test MIDI output** with DAW or virtual synthesizer  
3. **Iterate quickly** without hardware dependency
4. **Port to ESP32** when UI is finalized

### Build Targets

```bash
# Clean builds
pio run -t clean

# Desktop simulation
pio run -e desktop

# ESP32-S3 hardware  
pio run -e rymcu-esp32-s3-devkitc-1

# Verbose build (debugging)
pio run -e desktop -v

# Debug build with JTAG
pio run -e rymcu-esp32-s3-devkitc-1-debug
```

## RtMidi Installation

The project includes an automated RtMidi installer (`scripts/install_rtmidi.py`) that:

✅ **Downloads RtMidi 6.0.0** from GitHub  
✅ **Extracts only required files** (`RtMidi.h`, `RtMidi.cpp`)  
✅ **Creates proper library.json** for PlatformIO  
✅ **Configures Linux ALSA backend** only  
✅ **Avoids problematic dependencies** (no Android/iOS/contrib files)  
✅ **Caches installation** (skips if already installed)  

### Manual RtMidi Installation

If automatic installation fails:

```bash
# Run the installer manually
python scripts/install_rtmidi.py

# Verify installation
ls -la lib/rtmidi_linux/
# Should contain: RtMidi.h, RtMidi.cpp, library.json
```

### Troubleshooting RtMidi

```bash
# Clean RtMidi installation
rm -rf lib/rtmidi_linux/ .pio/
python scripts/install_rtmidi.py

# Check ALSA development headers
pkg-config --cflags --libs alsa

# Install missing dependencies
sudo apt-get install libasound2-dev
```

## MIDI Testing

### Desktop Testing
```bash
# List available MIDI ports
aconnect -l

# Connect to software synthesizer
# The app will show available ports and create virtual port if needed
```

### ESP32 Testing
```bash
# Monitor USB MIDI on Linux
aseqdump -p <port>

# Test with amidi
amidi -l
amidi -p hw:X,0 -d  # Replace X with device number
```

## Contributing

1. **Desktop development**: Use `pio run -e desktop` for fast iteration
2. **Test MIDI output**: Verify with DAW or `aseqdump`
3. **Cross-platform testing**: Ensure code works on both targets
4. **Follow pinned versions**: Don't update dependencies without testing

## Troubleshooting

### Common Issues

**SDL2 not found**:
```bash
sudo apt-get install libsdl2-dev
```

**ALSA not found**:
```bash
sudo apt-get install libasound2-dev
```

**RtMidi compilation errors**:
```bash
rm -rf lib/rtmidi_linux/ .pio/
python scripts/install_rtmidi.py
pio run -e desktop
```

**ESP32 upload fails**:
```bash
# Check USB connection and permissions
ls -la /dev/ttyUSB* /dev/ttyACM*
sudo usermod -a -G dialout $USER  # Then logout/login
```

### Build Flags

The project uses specific build flags for each platform:

**Desktop**: `-D__LINUX_ALSA__`, `-D__LITTLE_ENDIAN__`, `-DHAVE_GETTIMEOFDAY`  
**ESP32**: `-DESP32_BUILD`, `-DBOARD_HAS_PSRAM`, `-DLV_TICK_CUSTOM`

## License

[Your license here]

---

🎹 **Happy synthesizing!** Connect your desktop simulator to a DAW or run on ESP32-S3 hardware for a complete MIDI controller experience.