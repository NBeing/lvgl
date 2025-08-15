# Sequencer MIDI Control Documentation

## Overview

The **SequencerMidiController** provides comprehensive MIDI control for all aspects of the step sequencer, enabling external MIDI controllers, DAWs, and hardware to control every function of the sequencer. This extends beyond basic parameter control to include transport, step programming, track management, and parameter locks.

## 🎛️ **MIDI CC Mapping Reference**

### **Transport Control (CC 80-89)**

| CC  | Function | Value Range | Description |
|-----|----------|-------------|-------------|
| 80  | TRANSPORT_PLAY | 0-127 | Play/Stop toggle (≥64 = trigger) |
| 81  | TRANSPORT_STOP | 0-127 | Stop & Reset (≥64 = trigger) |
| 82  | TRANSPORT_PAUSE | 0-127 | Pause/Continue toggle (≥64 = trigger) |
| 83  | TRANSPORT_BPM | 0-127 | Tempo control (mapped to BPM range) |
| 84  | PATTERN_SELECT | 0-127 | Pattern selection (0-15) |
| 85  | PATTERN_LENGTH | 0-127 | Pattern length (1-16 steps) |
| 86  | SWING_AMOUNT | 0-127 | Swing timing (0-100%) |

### **Track Control (CC 120-127)**

| CC  | Function | Value Range | Description |
|-----|----------|-------------|-------------|
| 120 | TRACK_SELECT | 0-127 | Current track selection (0-7) |
| 121 | TRACK_MUTE | 0-127 | Mute current track (≥64 = trigger) |
| 122 | TRACK_SOLO | 0-127 | Solo current track (≥64 = trigger) |
| 123 | TRACK_TRANSPOSE | 0-127 | Transpose current track (-12 to +12 semitones) |

### **Step Programming (CC 90-111)**

| CC  | Function | Value Range | Description |
|-----|----------|-------------|-------------|
| 90  | STEP_SELECT | 0-127 | Current step selection (0-15) |
| 91-106 | STEP_TRIGGER_01-16 | 0-127 | Individual step triggers (≥64 = toggle) |
| 107 | STEP_VELOCITY | 0-127 | Current step velocity (0-127) |
| 108 | STEP_PROBABILITY | 0-127 | Current step probability (0-100%) |
| 109 | STEP_MICRO_TIMING | 0-127 | Current step micro-timing offset |
| 110 | STEP_NOTE | 0-127 | Current step MIDI note (0-127) |
| 111 | STEP_LENGTH | 0-127 | Current step length (1-24 ticks) |

### **Parameter Lock Control (CC 70-79)**

| CC  | Function | Value Range | Description |
|-----|----------|-------------|-------------|
| 70  | PARAM_LOCK_MODE | 0-127 | Enter/exit parameter lock mode (≥64 = toggle) |
| 71  | PARAM_LOCK_PARAMETER | 0-127 | Select parameter to lock (0-255) |
| 72  | PARAM_LOCK_VALUE | 0-127 | Set parameter lock value (0.0-1.0) |
| 73  | PARAM_LOCK_CLEAR | 0-127 | Clear current parameter lock (≥64 = trigger) |
| 74  | PARAM_LOCK_CLEAR_ALL | 0-127 | Clear all locks for current step (≥64 = trigger) |
| 75  | PARAM_LOCK_COPY | 0-127 | Copy locks from current step (≥64 = trigger) |
| 76  | PARAM_LOCK_PASTE | 0-127 | Paste locks to current step (≥64 = trigger) |

### **Performance Controls (CC 60-69)**

| CC  | Function | Value Range | Description |
|-----|----------|-------------|-------------|
| 60  | FILL_MODE | 0-127 | Activate fill mode (≥64 = trigger) |
| 61  | MUTE_ALL | 0-127 | Global mute toggle (≥64 = trigger) |
| 62  | SOLO_CLEAR | 0-127 | Clear all solo states (≥64 = trigger) |
| 63  | PATTERN_CHAIN | 0-127 | Chain to next pattern (≥64 = trigger) |
| 64  | RANDOMIZE_TRACK | 0-127 | Randomize current track (≥64 = trigger) |
| 65  | SHIFT_TRACK_LEFT | 0-127 | Shift track pattern left (≥64 = trigger) |
| 66  | SHIFT_TRACK_RIGHT | 0-127 | Shift track pattern right (≥64 = trigger) |
| 67  | CLEAR_TRACK | 0-127 | Clear current track (≥64 = trigger) |
| 68  | COPY_TRACK | 0-127 | Copy current track (≥64 = trigger) |
| 69  | PASTE_TRACK | 0-127 | Paste to current track (≥64 = trigger) |

## 🎹 **Note Input Programming**

Beyond CC control, you can also program steps using MIDI Note On messages:

- **Current Track/Step Context:** Notes are programmed into the currently selected track and step
- **Automatic Step Advancement:** After programming a note, the current step advances automatically
- **Note & Velocity Capture:** Both MIDI note number and velocity are captured
- **Step Activation:** Programming a note automatically activates the step

**Example Workflow:**
1. Select Track 1 (CC 120 = 0)
2. Select Step 1 (CC 90 = 0)
3. Play MIDI note C4 (note 60, velocity 100)
4. Step 1 is programmed with C4, velocity 100, and activated
5. Current step advances to Step 2 automatically

## 💻 **Programming Integration**

### **Basic Setup**

```cpp
#include "components/midi/SequencerMidiController.h"
#include "components/midi/StepSequencer.h"
#include "components/parameter/ParameterManager.h"

// Initialize sequencer components
auto sequencer = std::make_shared<StepSequencer>();
auto param_manager = std::make_shared<Parameters::ParameterManager>();

// Initialize MIDI controller
auto& midi_controller = SequencerMidiController::getInstance();
midi_controller.initialize(sequencer, param_manager);

// The controller will automatically process incoming MIDI CC messages
```

### **Custom MIDI Mappings**

```cpp
// Assign custom MIDI mappings
midi_controller.assignSequencerMidiCC(
    channel: 2,                                    // MIDI channel 2
    cc: 50,                                       // CC 50
    param_id: SequencerParameterID::TRANSPORT_PLAY // Play/Stop function
);

// Remove mappings
midi_controller.removeSequencerMidiCC(channel: 1, cc: 80);

// Query mappings
auto param_id = midi_controller.getSequencerMidiMapping(channel: 1, cc: 80);
```

### **Current Context Management**

```cpp
// Set programming context
midi_controller.setCurrentTrack(2);    // Track 3 (0-based)
midi_controller.setCurrentStep(7);     // Step 8 (0-based)

// Query current context
int track = midi_controller.getCurrentTrack();
int step = midi_controller.getCurrentStep();
bool lock_mode = midi_controller.isParameterLockModeActive();
```

### **Statistics and Debugging**

```cpp
// Get processing statistics
auto stats = midi_controller.getStatistics();
std::cout << "Transport commands: " << stats.transport_commands.load() << std::endl;
std::cout << "Step commands: " << stats.step_commands.load() << std::endl;

// Print detailed statistics
midi_controller.printSequencerMidiStatistics();

// Export current mappings
std::string mappings = midi_controller.exportMidiMappings();
std::cout << mappings << std::endl;
```

## 🎛️ **Hardware Controller Examples**

### **Elektron Digitakt Style Control**

```cpp
// Map CCs to match Digitakt workflow
midi_controller.assignSequencerMidiCC(1, 12, SequencerParameterID::TRACK_SELECT);     // Track knob
midi_controller.assignSequencerMidiCC(1, 13, SequencerParameterID::PATTERN_SELECT);   // Pattern knob
midi_controller.assignSequencerMidiCC(1, 14, SequencerParameterID::STEP_VELOCITY);    // Level knob

// Step buttons (using note input)
// Notes C1-D#2 (36-51) map to steps 1-16
```

### **Akai MPC Style Control**

```cpp
// Transport controls
midi_controller.assignSequencerMidiCC(1, 115, SequencerParameterID::TRANSPORT_PLAY);  // Play button
midi_controller.assignSequencerMidiCC(1, 116, SequencerParameterID::TRANSPORT_STOP);  // Stop button

// 16 pads for step triggers (using default CC 91-106)
// Velocity-sensitive step programming via Note On
```

### **Ableton Push Style Control**

```cpp
// Performance controls
midi_controller.assignSequencerMidiCC(1, 44, SequencerParameterID::MUTE_ALL);         // Master mute
midi_controller.assignSequencerMidiCC(1, 45, SequencerParameterID::SOLO_CLEAR);       // Solo clear

// Touch strip for swing
midi_controller.assignSequencerMidiCC(1, 1, SequencerParameterID::SWING_AMOUNT);      // Mod wheel

// 64 pads for step + track selection via Note On
```

## 🎵 **Usage Workflows**

### **Live Performance Workflow**

1. **Track Selection:** Use CC 120 to switch between tracks
2. **Step Programming:** Use CC 91-106 to trigger/untrigger steps during playback
3. **Performance Controls:** Use CC 61-69 for mute, solo, randomize, shift operations
4. **Transport:** Use CC 80-82 for play/stop/pause control
5. **Real-time Parameter Locks:** Use CC 70-76 to modify parameter locks during performance

### **Pattern Programming Workflow**

1. **Setup:** Select track (CC 120) and pattern (CC 84)
2. **Step Programming:** Use Note On messages to program step notes and velocities
3. **Step Editing:** Use CC 107-111 to adjust velocity, probability, timing, length
4. **Parameter Locks:** Enter lock mode (CC 70), select parameter (CC 71), set values (CC 72)
5. **Pattern Editing:** Use CC 64-69 for randomize, shift, copy/paste operations

### **Parameter Lock Workflow**

1. **Enter Lock Mode:** Send CC 70 with value ≥64
2. **Select Parameter:** Send CC 71 with parameter ID (0-255)
3. **Navigate:** Use CC 120 (track) and CC 90 (step) to select target step
4. **Set Lock Value:** Send CC 72 with desired value (0-127)
5. **Copy/Paste Locks:** Use CC 75 (copy) and CC 76 (paste) for bulk operations
6. **Exit Lock Mode:** Send CC 70 with value <64

## 🔧 **Technical Details**

### **Thread Safety**
- All MIDI processing happens on RT thread
- Sequencer operations are queued to appropriate threads
- Lock-free data structures used for RT-safe operation
- Statistics tracking uses atomic operations

### **Value Conversion**
- **MIDI CC Values (0-127)** → **Normalized (0.0-1.0)** → **Parameter Range**
- **Trigger Values:** ≥64 = ON/Trigger, <64 = OFF/Release
- **Continuous Values:** Linear mapping from 0-127 to parameter range

### **Memory Efficiency**
- Minimal memory allocation during RT processing
- Efficient hash maps for MIDI mapping lookup
- Compact event structures for debugging/tracing

### **Integration Points**
- **EnhancedRTClockManager:** Routes MIDI CC through sequencer controller first
- **MidiParameterBridge:** Handles general parameter control for unmapped CCs
- **StepSequencer:** Receives sequencer commands through thread-safe interface
- **ParameterManager:** Manages parameter locks and automation

## 📊 **Monitoring and Debugging**

The system provides comprehensive monitoring capabilities:

```cpp
// Real-time statistics
auto stats = midi_controller.getStatistics();
// - transport_commands: Play/stop/tempo commands
// - step_commands: Step programming commands  
// - track_commands: Track control commands
// - param_lock_commands: Parameter lock operations
// - performance_commands: Live performance commands
// - unmapped_ccs: CCs not handled by sequencer
// - midi_learn_assignments: Dynamic MIDI learn operations

// Debug output (enabled with DEBUG_SEQUENCER_MIDI)
// Console logging of all MIDI events with parameter mapping
// RT-safe logging for performance analysis
```

## 🎯 **Best Practices**

1. **Initialization Order:** Initialize ParameterManager → StepSequencer → SequencerMidiController
2. **Thread Safety:** Only call non-const methods from main thread except for MIDI processing
3. **MIDI Channel Usage:** Use different channels for different controller types to avoid conflicts
4. **Performance:** Batch MIDI operations when possible to reduce RT thread load
5. **Debugging:** Enable DEBUG_SEQUENCER_MIDI only during development to avoid console spam

## 🎛️ **Summary**

The **SequencerMidiController** provides **complete MIDI control** over every aspect of your step sequencer:

✅ **Transport Control** - Play, stop, tempo, pattern switching  
✅ **Step Programming** - Individual step triggers, velocity, probability, timing  
✅ **Track Management** - Selection, mute, solo, transpose  
✅ **Parameter Locks** - Full Digitakt-style parameter lock system  
✅ **Performance Features** - Randomize, shift, copy/paste, fill mode  
✅ **Note Input** - Chromatic step programming via MIDI notes  
✅ **Custom Mappings** - Fully customizable MIDI CC assignments  
✅ **Thread Safety** - RT-safe operation with comprehensive monitoring

Your sequencer is now **fully MIDI controllable** and ready for professional live performance and studio integration! 🎵✨
