# LVGL MIDI Sequencer - Design Document

## Project Overview

A general-purpose MIDI transformation device built on ESP32-S3 with touch display, featuring sequencing, mapping, macros, and hardware controls.

## Core Features

### MIDI Transformation Engine
- **Input Filtering**: Channel, note range, CC range filtering
- **Transform Rules**: Note → CC, CC → note, channel mapping
- **Velocity Curves**: Custom velocity scaling and response curves
- **CC Scaling**: Flexible CC value transformation
- **Chord Triggers**: Single note input → multiple note output

### Sequencer Engine
- **Multi-track Sequencing**: Notes + CCs on separate tracks
- **Pattern-based Architecture**: Similar to Elektron workflow
- **Real-time Recording**: Live input capture with quantization
- **Step Editing**: Grid-based pattern editing
- **CC Automation**: Touch-drawable CC curves and LFOs
- **Sync Options**: Internal clock, MIDI clock, external sync
- **Quantization**: Configurable timing correction
- **Swing/Humanization**: Groove and timing variations

### Macro System
- **Chain Sequencing**: Multiple MIDI messages per trigger
- **Conditional Logic**: Context-sensitive macro execution
- **Parameter Automation**: Multi-parameter macro control
- **Trigger Assignment**: Hardware button → macro mapping

### Synthesizer Parameter Database
- **Preset Definitions**: Pre-loaded CC mappings for popular synthesizers
- **Human-readable Names**: "Filter Cutoff" instead of "CC 74"
- **Parameter Categories**: Organized by function (Filter, Envelope, LFO, etc.)
- **Value Ranges**: Min/max values and default settings
- **Custom Definitions**: User-created synthesizer mappings
- **Quick Assignment**: One-touch parameter assignment to sequencer tracks

### Hardware Integration
- **30+ Button Matrix**: 6×5 or 8×4 scanning pattern
- **LED Feedback**: Visual status for all controls
- **Velocity-sensitive Pads**: Analog input for dynamic control
- **Encoder Matrix**: Parameter control and navigation
- **Dedicated Transport**: Play/Stop/Record buttons

## Hardware Architecture

### ESP32-S3 Configuration
```
ESP32-S3 DevKit with dual USB-C:
├── USB-C #1 (Serial): Programming/Debug connection
├── USB-C #2 (Native): USB-OTG for MIDI + optional keyboard input
├── Touch Display: 480×320 with SD card slot
├── Button Matrix: 30+ buttons with LED feedback
├── Encoder Matrix: Rotary encoders for parameter control
├── Velocity Pads: Pressure-sensitive triggers
└── DIN MIDI I/O: Hardware UART + optocouplers
```

### Multi-Mode USB Strategy
- **Device Mode**: ESP32 → Computer (MIDI output)
- **Host Mode**: USB Keyboard → ESP32 (development input)
- **Mode Switching**: UI button to toggle between modes
- **Dual Port Usage**: Programming via USB #1, MIDI/input via USB #2

## Software Architecture

### Core System Layout
```
┌─────────────────────────────────────┐
│ Application Layer (Sequencer Logic) │
├─────────────────────────────────────┤
│ Menu System Framework               │
├─────────────────────────────────────┤
│ Enhanced UI Components              │
├─────────────────────────────────────┤
│ Hardware Abstraction Layer          │
├─────────────────────────────────────┤
│ LVGL Core                          │
└─────────────────────────────────────┘
```

### MIDI Processing Engine
```cpp
MIDIProcessor
├── InputRouter (USB + DIN inputs)
├── TransformEngine (mapping/filtering)
├── SequencerEngine (multi-track)
├── ClockSync (master/slave modes)
├── MacroProcessor (complex sequences)
├── SynthParameterDB (CC mappings)
└── OutputRouter (USB + DIN outputs)
```

### Synthesizer Parameter System
```cpp
class SynthParameterManager {
    std::vector<SynthDefinition> synth_definitions_;
    SynthDefinition* current_synth_ = nullptr;
    
public:
    bool loadSynthDefinition(const std::string& synth_name);
    std::vector<std::string> getAvailableSynths();
    std::vector<Parameter> getParametersByCategory(Category cat);
    Parameter* findParameterByName(const std::string& name);
    std::vector<Parameter> searchParameters(const std::string& query);
};

struct Parameter {
    std::string name;           // "Filter Cutoff"
    std::string short_name;     // "Cutoff"
    uint8_t cc_number;         // 74
    Category category;         // FILTER
    uint8_t min_value;         // 0
    uint8_t max_value;         // 127
    uint8_t default_value;     // 64
    std::string description;   // "Controls low-pass filter frequency"
    bool is_bipolar;          // true for -64 to +63 style parameters
};

enum class Category {
    FILTER, ENVELOPE, LFO, OSCILLATOR, 
    EFFECTS, MODULATION, ARPEGGIATOR, 
    PERFORMANCE, GLOBAL
};
```

### Real-time Task Architecture
```cpp
Core 0 (Real-time):
├── MIDI Processing (<1ms latency)
├── Sequencer Engine
├── Clock Synchronization
└── Hardware Input Scanning

Core 1 (Non-real-time):
├── LVGL GUI Updates (~16ms)
├── Touch Processing
├── Menu Navigation
└── Display Rendering
```

## User Interface Design

### Screen Hierarchy
```
Main Menu
├── Sequencer
│   ├── Pattern Edit
│   │   ├── Step Edit (touch grid)
│   │   ├── Real-time Record
│   │   └── CC Lane Edit (touch draw)
│   ├── Song Mode
│   └── Sync Settings
├── MIDI Mapping
│   ├── Input Filters
│   ├── Transform Rules
│   └── Output Routing
├── Synthesizers
│   ├── Select Synth Model
│   ├── Parameter Browser
│   ├── Quick Assign
│   └── Custom Definitions
├── Macros
│   ├── Macro List
│   ├── Macro Editor
│   └── Trigger Assignment
├── Hardware
│   ├── Control Assignment
│   ├── Encoder Mapping
│   └── Button Functions
└── Settings
    ├── MIDI Channels
    ├── Clock Source
    └── Display Options
```

### Synthesizer Parameter UI
```
Parameter Browser Screen:
├── Synth Selection Dropdown
├── Category Tabs (Filter, Envelope, etc.)
├── Parameter List with Search
├── Quick Assign Buttons
└── Parameter Details Panel

Quick Assign Workflow:
1. Select synthesizer model
2. Browse/search for parameter
3. Assign to sequencer track
4. Parameter appears with human name
```

### Hardware Abstraction Pattern

#### Input Device Interface (Strategy Pattern)
```cpp
class InputDevice {
public:
    virtual void update() = 0;
    virtual bool isButtonPressed(int button_id) = 0;
    virtual bool isButtonJustPressed(int button_id) = 0;
    virtual int getEncoderDelta(int encoder_id) = 0;
    virtual int getVelocityPadValue(int pad_id) = 0;
    
    using ButtonCallback = std::function<void(int, bool)>;
    using EncoderCallback = std::function<void(int, int)>;
    
    virtual void setButtonCallback(ButtonCallback callback) = 0;
    virtual void setEncoderCallback(EncoderCallback callback) = 0;
};
```

#### Implementation Types
- **ESP32InputDevice**: Hardware button/encoder matrix
- **USBKeyboardHost**: USB keyboard input (development)
- **DesktopInputDevice**: SDL keyboard mapping (cross-platform development)
- **MockInputDevice**: Testing and simulation

#### Factory Pattern for Device Selection
```cpp
enum class InputDeviceType {
    ESP32_HARDWARE,
    USB_KEYBOARD,
    DESKTOP_KEYBOARD,
    MOCK_DEVICE
};

class InputDeviceFactory {
public:
    static std::unique_ptr<InputDevice> create(InputDeviceType type);
};
```

## Undo/Savepoint System

### Implementation Strategy
**Hybrid Approach**: Command Pattern + State Snapshots

#### Command Pattern (Fine-grained changes)
```cpp
class Command {
public:
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string getDescription() const = 0;
};

// Examples: SetStepCommand, SetParameterCommand, etc.
```

#### State Snapshots (Major changes)
```cpp
struct SequencerState {
    std::vector<Pattern> patterns;
    std::vector<Track> tracks;
    TransportState transport;
    MidiMappings mappings;
    
    std::vector<uint8_t> serialize() const;
    void deserialize(const std::vector<uint8_t>& data);
};
```

#### Memory Management
- **RAM Buffer**: 50-100 recent operations (1-2MB)
- **SD Card Storage**: Named savepoints (unlimited)
- **Circular Buffer**: Automatic old operation removal
- **Delta Compression**: Merge operations periodically

## Storage Architecture

### SD Card Organization
```
/savepoints/
├── user_savepoint_001.dat
├── user_savepoint_002.dat
└── auto_backup_YYYYMMDD.dat

/patterns/
├── pattern_bank_A.dat
├── pattern_bank_B.dat
└── user_patterns/

/macros/
├── default_macros.dat
└── user_macros.dat

/synthesizers/
├── hydrasynth.json
├── prophet_rev2.json
├── moog_subsequent.json
├── custom_synth_001.json
└── user_definitions/

/settings/
├── midi_config.json
├── hardware_config.json
└── display_settings.json
```

### Synthesizer Definition Format
Each synthesizer has a JSON file with complete CC mapping:
```json
{
  "name": "ASM Hydrasynth",
  "manufacturer": "Ashun Sound Machines",
  "version": "1.0",
  "categories": {
    "filter": [ /* parameters */ ],
    "envelope": [ /* parameters */ ],
    "lfo": [ /* parameters */ ]
  }
}
```

### Performance Considerations
- **Pattern Loading**: Lazy loading from SD to PSRAM
- **Auto-save**: Periodic background saves
- **Wear Leveling**: Rotate save locations
- **Synth Definition Caching**: Keep current synth in RAM

## Development Phases

### Phase 1: Foundation Enhancement
- [ ] Enhanced menu system framework
- [ ] Hardware abstraction layer implementation
- [ ] Basic input device switching (USB modes)
- [ ] SD card storage integration
- [ ] Core undo/redo system
- [ ] Synthesizer parameter database system

### Phase 2: Hardware Integration
- [ ] Button/encoder matrix scanning
- [ ] LED feedback system
- [ ] Velocity pad input processing
- [ ] Hardware-driven navigation
- [ ] USB keyboard host mode

### Phase 3: MIDI Engine Core
- [ ] MIDI routing and mapping engine
- [ ] Basic sequencer (notes + timing)
- [ ] Clock synchronization (internal/external)
- [ ] DIN MIDI I/O integration
- [ ] Real-time performance optimization
- [ ] Synthesizer parameter integration

### Phase 4: Advanced Sequencer Features
- [ ] Multi-track sequencing
- [ ] CC automation and LFO drawing
- [ ] Pattern chaining and song mode
- [ ] Advanced quantization options
- [ ] Swing and humanization
- [ ] Parameter browser and quick assign

### Phase 5: Professional Features
- [ ] Complex macro system
- [ ] Advanced MIDI transformation
- [ ] Performance mode optimization
- [ ] Backup/restore functionality
- [ ] Firmware update system
- [ ] Custom synthesizer definition editor

## Performance Requirements

### Real-time Timing Budgets
```
Critical (Core 0):
├── MIDI Timing: <1ms (Note accuracy)
├── Button Response: <10ms (User experience)
└── Clock Sync: <0.1ms (Tight timing)

Non-critical (Core 1):
├── Display Update: <16ms (60fps target)
├── Menu Navigation: <50ms (Responsive feel)
└── File Operations: <500ms (Background tasks)
```

### Memory Allocation
```
ESP32-S3 (8MB PSRAM):
├── Pattern Storage: 2-4MB (hundreds of patterns)
├── Undo Buffer: 1-2MB (50-100 operations)
├── Graphics Buffer: 1MB (LVGL framebuffer)
├── Synth Definitions: 200KB (cached current synth)
├── System/Code: 1MB (firmware + stack)
└── Available: 1-3MB (future expansion)
```

## Scalability Considerations

### Single ESP32-S3 Capabilities
- Basic sequencer (8-16 tracks)
- MIDI I/O + USB functionality
- Touch UI with moderate complexity
- ~30 buttons/encoders with LED feedback
- Pattern storage and basic macro system
- Multiple synthesizer definitions

### Multi-ESP32 Architecture (If Needed)
```
ESP32-S3 Master (MIDI Engine):
├── Sequencer logic and timing
├── MIDI routing and transformation
├── USB MIDI device functionality
├── Synthesizer parameter database
└── System coordination

ESP32 Slave 1 (UI/Display):
├── LVGL graphics rendering
├── Touch input processing
├── Menu system navigation
├── Parameter browser UI
└── Display management

ESP32 Slave 2 (Hardware I/O):
├── Button/encoder matrix scanning
├── LED matrix control
├── Velocity pad processing
└── DIN MIDI I/O
```

### Inter-processor Communication
- **SPI**: High-speed deterministic communication (40MHz)
- **I2C**: Simpler implementation for command/control
- **UART**: Reliable for event streaming
- **Message Protocol**: Structured command/response system

## Risk Mitigation

### Performance Monitoring
```cpp
class PerformanceMonitor {
    void measureMIDILatency();
    void trackMemoryUsage();
    void monitorTaskTiming();
    void reportBottlenecks();
};
```

### Fallback Strategies
- **Reduced Feature Mode**: Disable non-essential features if performance degrades
- **Emergency Stops**: Halt non-critical tasks during high MIDI load
- **Graceful Degradation**: Reduce UI refresh rate during intensive operations

### Development Safety
- **Cross-platform Testing**: Desktop simulation for rapid iteration
- **Incremental Features**: Each phase builds on proven foundation
- **Performance Gates**: Don't proceed to next phase until timing requirements met
- **Backup Architecture**: Always maintain working fallback configuration

## Technology Stack

### Core Libraries
- **LVGL**: GUI framework (proven working)
- **LovyanGFX**: Display driver (pinned to compatible version)
- **Control-Surface**: MIDI library (Arduino Core 3.x compatible)
- **Arduino Core 3.x**: pioarduino fork for ESP32-S3
- **ArduinoJson**: JSON parsing for synthesizer definitions

### Development Tools
- **PlatformIO**: Build system and library management
- **SDL2**: Desktop simulation environment
- **Git**: Version control with incremental milestones
- **Performance Profiling**: Built-in timing measurement

### Hardware Components
- **ESP32-S3 DevKit**: Dual USB-C, 8MB PSRAM
- **Touch Display**: 480×320 with SD slot
- **Button Matrix**: Custom PCB with LED integration
- **Encoder Array**: Rotary encoders for parameter control
- **DIN MIDI**: Standard 5-pin DIN I/O circuits
- **Velocity Pads**: Analog pressure sensors

This design provides a roadmap for building a professional-grade MIDI sequencer and transformation device while maintaining real-time performance and allowing for future expansion.