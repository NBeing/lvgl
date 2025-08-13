# LVGL MIDI Device Framework

A cross-platform MIDI device development framework built with LVGL, supporting both desktop simulation and ESP32-S3 hardware for creating custom MIDI controllers.

## Features

🎹 **Cross-Platform MIDI**: Real MIDI output on both desktop and ESP32  
🖥️ **Desktop Simulation**: Fast development with SDL2 and RtMidi  
📱 **ESP32-S3 Hardware**: Touchscreen display with USB MIDI output  
🎛️ **MIDI Controller UI**: Knobs, sliders, and controls for MIDI device creation  
🔧 **Automated Builds**: PlatformIO with automated dependency management  

## Hardware Targets

### Desktop (Linux)
- **Display**: SDL2 window simulation
- **MIDI**: RtMidi with ALSA backend
- **Development**: Fast UI iteration and MIDI device testing

### ESP32-S3 DevKit
- **Display**: ST7796S TFT with LovyanGFX
- **MIDI**: USB MIDI via Control-Surface library
- **Hardware**: ESP32-S3 with PSRAM support

## Dependencies

### Desktop Dependencies
- **LVGL 9.3.0**: UI framework
- **RtMidi 6.0.0**: Cross-platform MIDI I/O (auto-installed)
- **SDL2**: Graphics and input
- **ALSA**: Linux MIDI system

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
cd lvgl-midi-framework
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
lvgl-midi-framework/
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
- **Virtual port creation**: Creates "LVGL MIDI Controller" if no hardware found
- **DAW integration**: Connect to Ableton Live, FL Studio, Reaper, etc.
- **Hardware support**: USB MIDI interfaces, MIDI keyboards, controllers

### ESP32-S3
- **USB MIDI device**: Appears as standard MIDI controller
- **Low latency**: Hardware USB implementation
- **Cross-platform**: Works with any MIDI-capable software

## Development Workflow

### Desktop-First Development
1. **Design UI** on desktop with fast build times
2. **Test MIDI output** with DAW or MIDI monitor software  
3. **Iterate quickly** without hardware dependency
4. **Port to ESP32** when MIDI device is finalized

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

## Test Framework

### Unified Test Architecture

The project includes a comprehensive **unified test framework** designed for professional MIDI device development with **real-time constraints** and **thread safety** validation.

```
test/
├── framework/                    # Unified test framework
│   ├── unified_test_framework.h # Core framework with TEST_UNIT/TEST_INTEGRATION macros
│   └── test_fixtures.h          # Common test fixtures and utilities
├── unit/                        # Unit tests (individual components)
│   ├── rt_safe_midi_test.cpp    # MIDI processing validation
│   ├── priority_queue_test.cpp  # Data structure testing
│   ├── rt_safe_events_test.cpp  # Event distribution testing
│   └── thread_safety_test.cpp   # Concurrency validation
├── integration/                 # Integration tests (component interaction)
│   ├── rt_safe_ui_control_test.cpp      # UI ↔ Parameter integration
│   ├── bidirectional_bridge_test.cpp    # MIDI ↔ UI bidirectional flow
│   ├── event_visualizer_test.cpp        # Event system visualization
│   └── parameter_lock_test.cpp          # Parameter locking system
└── system/                      # System tests (end-to-end workflows)
    └── complete_system_test.cpp # Complete RT-safe system validation
```

### Running Tests

**Simple Commands:**
```bash
# Run all tests
cd test && ./run_tests.sh

# Run specific test categories
cd test && ./run_tests.sh unit
cd test && ./run_tests.sh integration
cd test && ./run_tests.sh system

# Clean test executables
cd test && ./run_tests.sh clean
```

**Using Make (even simpler):**
```bash
# From test directory
make test              # Run all tests
make test-unit         # Run unit tests only
make test-integration  # Run integration tests only
make test-system       # Run system tests only

# Focused test groups
make test-midi         # Run MIDI-related tests
make test-ui           # Run UI integration tests
make test-threading    # Run threading/concurrency tests

# Maintenance
make clean             # Clean executables
make help              # Show all available commands
```

**Manual Build & Run (advanced):**
**Manual Build & Run (advanced):**
```bash
# Build and run specific tests manually
cd test && g++ -std=c++17 -I. -Iframework -pthread unit/priority_queue_test.cpp -o unit/priority_queue_test
./unit/priority_queue_test

# Build and run integration tests
cd test && g++ -std=c++17 -I. -Iframework -pthread integration/rt_safe_ui_control_test.cpp -o integration/rt_safe_ui_control_test
./integration/rt_safe_ui_control_test
```

**Test Runner Features:**
- 🎨 **Colored output** for easy reading
- 📊 **Detailed summaries** for each test category
- 🔧 **Automatic building** and execution
- 🧹 **Easy cleanup** of build artifacts
- ⚡ **Fast execution** with parallel building
- 🎛️ **VS Code integration** with keyboard shortcuts

**VS Code Integration:**
```
Ctrl+Shift+T  →  Run all tests
Ctrl+Shift+U  →  Unit tests only
Ctrl+Shift+M  →  MIDI components  
Ctrl+Shift+H  →  Thread safety tests
```
Use VS Code's Command Palette: `Tasks: Run Task` to access all test commands.

### Test Framework Features

**🧵 Thread Safety Testing:**
- Concurrent producer/consumer validation
- Race condition detection
- Atomic operation verification
- Multi-threaded stress testing

**⚡ Real-Time Validation:**
- Sub-millisecond timing constraints
- MIDI-thread compatible operations
- RT-safe event distribution
- Performance benchmarking

**🎛️ Professional MIDI Patterns:**
- MIDI priority queue validation
- Parameter lock systems
- UI ↔ MIDI bidirectional flow
- Smooth value interpolation testing

**📊 Comprehensive Coverage:**
- **Unit Tests**: Individual component validation
- **Integration Tests**: Component interaction validation  
- **System Tests**: Complete end-to-end workflow validation

### Writing Tests

**Test Structure Example:**
```cpp
#include "../framework/unified_test_framework.h"

TEST_UNIT(ComponentName, TestName) {
    // Setup
    auto component = createTestComponent();
    
    // Execute
    bool result = component.performOperation();
    
    // Verify
    ASSERT_TRUE(result);
    ASSERT_EQ(42, component.getValue());
}

int main() {
    auto& runner = TestFramework::TestRunner::getInstance();
    auto results = runner.runCategory("unit/ComponentName");
    return results.failed_tests == 0 ? 0 : 1;
}
```

**Comment Style Guidelines:**
Follow the story-driven comment style from `test/RTSafeUIControlIntegrationTests.cpp`:
- **SCENARIO**: Describe the real-world situation being tested
- **VALIDATES**: List what the test proves/verifies
- **Story Structure**: Each test tells part of the system's story

### Test Results

All tests maintain **100% pass rate** with comprehensive validation of:
- **85+ individual tests** across the complete system
- **Thread safety** under concurrent load
- **Real-time timing** constraints for MIDI applications  
- **Professional MIDI device** workflow patterns

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
2. **Test MIDI output**: Verify with DAW or MIDI monitor software
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

## ESP32-S3 Arduino Core 3.x + USB MIDI Setup

### Overview

This project successfully combines **LovyanGFX**, **LVGL**, and **USB MIDI** on ESP32-S3 using Arduino Core 3.x. This was a significant challenge due to compatibility issues between these libraries, but we've found a working solution.

### The Problem

- **Arduino Core 2.x**: Stable but lacks proper USB MIDI support
- **Arduino Core 3.x**: Has USB MIDI but breaks LovyanGFX compatibility  
- **Official espressif32 platform**: Has incomplete Arduino Core 3.x support
- **TinyUSB configuration**: Complex and often misconfigured

### The Solution: pioarduino Community Fork

The key breakthrough is using the **[pioarduino community platform](https://github.com/pioarduino/platform-espressif32)** which provides proper Arduino Core 3.x support with LovyanGFX compatibility.

### Working Configuration

#### PlatformIO Configuration (`platformio.ini`)

```ini
[env:esp32-s3-usb-midi]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
board = esp32-s3-devkitc-1-n16r8v
framework = arduino
monitor_speed = 115200
monitor_filters = esp32_exception_decoder
upload_speed = 921600

build_unflags = 
    -DARDUINO_USB_MODE=1             ; Remove default CDC/JTAG mode

build_flags = 
    -D ESP32_BUILD=1
    -D BOARD_HAS_PSRAM
    -D ARDUINO_USB_MODE=0            ; Enable native USB-OTG mode
    -D ARDUINO_USB_CDC_ON_BOOT=1     ; Enable CDC interface at boot
    -D USE_TINYUSB                   ; Enable TinyUSB stack (NO =1!)
    -D CFG_TUD_MIDI=1               ; Enable USB MIDI device class
    -I include/

lib_deps = 
    lvgl/lvgl@^8.3.0
    bodmer/TFT_eSPI@^2.5.43
    lovyan03/LovyanGFX@^1.1.16       ; Version 1.1.16+ has Arduino Core 3.x fixes
```

#### Critical USB MIDI Configuration

The most important discoveries:

1. **`USE_TINYUSB` without `=1`** - This was the critical flag that enables proper USB enumeration
2. **`ARDUINO_USB_MODE=0`** - Enables USB-OTG mode instead of CDC/JTAG
3. **`CFG_TUD_MIDI=1`** - Explicitly enables USB MIDI device class
4. **pioarduino platform** - Essential for Arduino Core 3.x + LovyanGFX compatibility

#### MIDI Handler Implementation

```cpp
// MidiHandler.h
#pragma once

#if defined(ESP32_BUILD)
    #include <Arduino.h>
    #include <string>
    #include "USB.h"
    #include "USBMIDI.h"
    
    extern USBMIDI midi_usb;
#endif

class MidiHandler {
private:
    bool initialized_;
    
public:
    MidiHandler() : initialized_(false) {}
    
    bool initialize() {
        #if defined(ESP32_BUILD)
            Serial.println("Initializing USB MIDI...");
            USB.begin();
            midi_usb.begin();
            Serial.println("USB MIDI initialized successfully!");
            initialized_ = true;
            return true;
        #endif
        return false;
    }
    
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
        if (!initialized_) return;
        #if defined(ESP32_BUILD)
            midi_usb.noteOn(note, velocity, channel);
            Serial.printf("MIDI Note On: Ch%d Note%d Vel%d\n", channel, note, velocity);
        #endif
    }
    
    void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity = 0) {
        if (!initialized_) return;
        #if defined(ESP32_BUILD)
            midi_usb.noteOff(note, velocity, channel);
            Serial.printf("MIDI Note Off: Ch%d Note%d\n", channel, note);
        #endif
    }
    
    void sendControlChange(uint8_t channel, uint8_t cc_number, uint8_t value) {
        if (!initialized_) return;
        #if defined(ESP32_BUILD)
            midi_usb.controlChange(cc_number, value, channel);
            Serial.printf("MIDI CC: Ch%d CC%d Val%d\n", channel, cc_number, value);
        #endif
    }
};
```

```cpp
// MidiHandler.cpp
#include "MidiHandler.h"

#if defined(ESP32_BUILD)
// Define the global USBMIDI instance
USBMIDI midi_usb;
#endif
```

### Verification

After successful upload, verify USB MIDI is working:

```bash
# Check USB device enumeration
lsusb | grep 303a
# Should show: ID 303a:1001 Espressif Systems Espressif ESP32-S3-DevKitC-1-N16R8V

# Check ALSA MIDI devices
aconnect -l
# Should show: client X: 'Espressif ESP32-S3-DevKitC-1-N1' [type=kernel,card=1]

# Monitor MIDI messages
aseqdump -p "Espressif ESP32-S3-DevKitC-1-N1"
```

### Troubleshooting

#### Common Issues

1. **Device appears as JTAG/Serial only**
   - Check `ARDUINO_USB_MODE=0` is set
   - Ensure `USE_TINYUSB` flag is present (without `=1`)
   - Verify you're using the pioarduino platform

2. **LovyanGFX compilation errors**
   - Must use LovyanGFX version 1.1.16 or later
   - Must use pioarduino platform, not official espressif32

3. **Upload failures in USB-OTG mode**
   - Use the UART/JTAG USB port for uploading
   - Or manually enter download mode: Hold BOOT, press RESET, release BOOT

4. **No MIDI device in `aconnect -l`**
   - Check USB device classes: `lsusb -v -d 303a:1001 | grep -A 10 "bInterfaceClass"`
   - Should show Audio interface (class 1) for MIDI

### Key Resources

- **[pioarduino platform](https://github.com/pioarduino/platform-espressif32)** - Community-maintained Arduino Core 3.x support
- **[LovyanGFX compatibility guide](https://lovyan03.github.io/LovyanGFX/)** - Arduino Core 3.x compatibility notes
- **[Arduino ESP32-S3 USB documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/usb.html)**

## Usage
Something about the screen size here

### Credits

This breakthrough was achieved through extensive testing and debugging of Arduino Core 3.x compatibility issues. Special thanks to the pioarduino community for maintaining Arduino Core 3.x support.

// ...rest of existing README...
## License

[Your license here]

---

🎹 **Happy MIDI device building!** Connect your desktop simulator to a DAW or run on ESP32-S3 hardware for a complete custom MIDI controller experience.


