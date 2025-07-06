#pragma once

#include <lvgl.h>
#include <memory>
#include <functional>
#include "../MidiDial.h"
#include "../ParameterControl.h"
#include "../ParameterBinder.h"
#include "../CommandManager.h"
#include "../ButtonControl.h"
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
    std::unique_ptr<CommandManager> command_manager_;
    
    // UI components - NEW: Parameter-aware controls
    std::shared_ptr<DialControl> cutoff_dial_;
    std::shared_ptr<DialControl> resonance_dial_;
    std::shared_ptr<DialControl> volume_dial_;
    std::shared_ptr<DialControl> attack_dial_;
    std::shared_ptr<DialControl> release_dial_;
    std::shared_ptr<DialControl> lfo_rate_dial_;
    
    // Button controls for testing
    std::shared_ptr<ButtonControl> filter_enable_btn_;
    std::shared_ptr<ButtonControl> lfo_sync_btn_;
    std::shared_ptr<ButtonControl> trigger_btn_;
    
    // OLD: Keep original dials for comparison (can be removed later)
    std::unique_ptr<MidiDial> old_cutoff_dial_;
    std::unique_ptr<MidiDial> old_resonance_dial_;
    std::unique_ptr<MidiDial> old_volume_dial_;
    
    lv_obj_t* status_label_;
    lv_obj_t* undo_btn_;
    lv_obj_t* redo_btn_;
    lv_obj_t* undo_label_;
    lv_obj_t* redo_label_;
    
public:
    SynthApp();
    ~SynthApp();
    
    void setup();
    void loop();
    void createUI();
    void createParameterDials();    // NEW: Create parameter-aware dials
    void createButtonControls();    // NEW: Create button controls
    void createLegacyDials();       // OLD: Original dial creation
    void createUndoRedoControls();  // NEW: Create undo/redo UI
    void updateUndoRedoButtons();   // NEW: Update undo/redo button states
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