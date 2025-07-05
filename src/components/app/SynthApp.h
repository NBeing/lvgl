#pragma once

#include <lvgl.h>
#include <memory>
#include <functional>
#include "../MidiDial.h"
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
    
    // UI components
    std::unique_ptr<MidiDial> cutoff_dial_;
    std::unique_ptr<MidiDial> resonance_dial_;
    std::unique_ptr<MidiDial> volume_dial_;
    lv_obj_t* status_label_;
    
public:
    SynthApp();
    ~SynthApp();
    
    void setup();
    void loop();
    void createUI();
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