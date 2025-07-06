#pragma once

#include <lvgl.h>
#include <memory>
#include <functional>
#include "../MidiDial.h"
#include "../ParameterControl.h"
#include "../ParameterBinder.h"
#include "../../hardware/MidiHandler.h"

#if defined(ESP32_BUILD)
    #include "../../hardware/ESP32Display.h"
#endif

class SynthApp {
private:
    bool initialized_;
    
    // Hardware interfaces
    #if defined(ESP32_BUILD)
        std::unique_ptr<ESP32Display> display_driver_;
    #endif
    
    std::unique_ptr<MidiHandler> midi_handler_;
    
    // Parameter system
    std::unique_ptr<ParameterBinder> parameter_binder_;
    
    // UI components - NEW: Parameter-aware controls
    std::shared_ptr<DialControl> cutoff_dial_;
    std::shared_ptr<DialControl> resonance_dial_;
    std::shared_ptr<DialControl> volume_dial_;
    std::shared_ptr<DialControl> attack_dial_;
    std::shared_ptr<DialControl> release_dial_;
    std::shared_ptr<DialControl> lfo_rate_dial_;
    
    // OLD: Keep original dials for comparison (can be removed later)
    std::unique_ptr<MidiDial> old_cutoff_dial_;
    std::unique_ptr<MidiDial> old_resonance_dial_;
    std::unique_ptr<MidiDial> old_volume_dial_;
    
    lv_obj_t* status_label_;
    
public:
    SynthApp();
    ~SynthApp();
    
    void setup();
    void loop();
    void createUI();
    void createParameterDials();    // NEW: Create parameter-aware dials
    void createLegacyDials();       // OLD: Original dial creation
    void simulateMidiCC();
    void updateStatus(const char* control, int value);
    
private:
    void initHardware();
    void initDesktop();
    void handleEvents();
    void handleMidiCC(int cc_number, int value);
    void platformDelay(int ms);
    unsigned long getPlatformTick();
};