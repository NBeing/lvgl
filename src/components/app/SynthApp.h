#pragma once

#include <lvgl.h>
#include <memory>
#include <functional>

// Forward declarations
class MidiDial;

class SynthApp {
public:
    SynthApp();
    ~SynthApp();
    
    // Main application lifecycle
    void setup();
    void loop();
    
    // Platform-specific initialization
    void initHardware();
    void initDesktop();
    
    // UI creation and management
    void createUI();
    void handleEvents();
    
    // MIDI handling
    void handleMidiCC(int cc_number, int value);
    void simulateMidiCC();
    void updateStatus(const char* control, int value);
    
private:
    // UI components
    std::unique_ptr<MidiDial> cutoff_dial_;
    std::unique_ptr<MidiDial> resonance_dial_;
    std::unique_ptr<MidiDial> volume_dial_;
    lv_obj_t* status_label_;
    
    // Application state
    bool initialized_;
    
    // Platform-specific helpers
    void platformDelay(int ms);
    unsigned long getPlatformTick();
};