#pragma once

#include <lvgl.h>
#include <memory>

// Forward declarations
class MidiDial;

#if defined(ESP32_BUILD)
class ESP32Display;
#endif

class SynthApp {
public:
    SynthApp();
    ~SynthApp();
    
    void setup();
    void loop();
    
    // Event handlers
    void simulateMidiCC();
    void handleMidiCC(int cc_number, int value);
    
private:
    void initHardware();
    void initDesktop();
    void createUI();
    void handleEvents();
    void updateStatus(const char* control, int value);
    
    // Platform abstraction
    void platformDelay(int ms);
    unsigned long getPlatformTick();
    
    // UI components
    std::unique_ptr<MidiDial> cutoff_dial_;
    std::unique_ptr<MidiDial> resonance_dial_;
    std::unique_ptr<MidiDial> volume_dial_;
    lv_obj_t* status_label_;
    
    // Hardware driver (ESP32 only)
#if defined(ESP32_BUILD)
    std::unique_ptr<ESP32Display> display_driver_;
#endif
    
    bool initialized_;
};