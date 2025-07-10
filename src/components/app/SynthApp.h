#pragma once

#include <lvgl.h>
#include <memory>
#include <functional>

#include "components/controls/ParameterControl.h"
#include "components/controls/DialControl.h"
#include "components/parameter/ParameterBinder.h"
#include "components/parameter/CommandManager.h"
#include "components/controls/ButtonControl.h"
#include "components/ui/WindowManager.h"
#include "components/ui/MainControlTab.h"
#include "components/ui/HelloTab.h"
#include "components/ui/WorldTab.h"
#include "hardware/MidiHandler.h"

#if defined(ESP32_BUILD)
#include "hardware/ESP32Display.h"
#endif

class SynthApp : public WindowObserver {
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
    
    // NEW: Window management system
    std::unique_ptr<WindowManager> window_manager_;
    lv_obj_t* root_container_;
    
    // Tab instances
    std::unique_ptr<MainControlTab> main_tab_;
    std::unique_ptr<HelloTab> hello_tab_;
    std::unique_ptr<WorldTab> world_tab_;

    // LEGACY: Individual controls (will be moved to MainControlTab)
    std::shared_ptr<DialControl> cutoff_dial_;
    std::shared_ptr<DialControl> resonance_dial_;
    std::shared_ptr<DialControl> volume_dial_;
    std::shared_ptr<DialControl> attack_dial_;
    std::shared_ptr<DialControl> release_dial_;
    std::shared_ptr<DialControl> lfo_rate_dial_;
    
    // Button controls for testing
    // Flex layout containers (for new UI)
    lv_obj_t* dials_row_ = nullptr;
    lv_obj_t* button_col_ = nullptr;

    std::shared_ptr<ButtonControl> filter_enable_btn_;
    std::shared_ptr<ButtonControl> lfo_sync_btn_;
    std::shared_ptr<ButtonControl> trigger_btn_;
    
    
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
    
    // WindowObserver interface
    void onTabChanged(const std::string& old_tab, const std::string& new_tab) override;
    void onPopupOpened(const std::string& popup_name) override;
    void onPopupClosed(const std::string& popup_name) override;
    
    // Legacy methods (for backward compatibility)
    void createUI();
    void createParameterDials();    // Legacy: Create parameter-aware dials (absolute)
    void createButtonControls();    // Legacy: Create button controls (absolute)
    void createUndoRedoControls();  // Legacy: Create undo/redo UI (absolute)
    // Flex-based UI creation
    void createParameterDialsFlex();
    void createButtonControlsFlex();
    void createUndoRedoControlsFlex();
    void createStatusAreaFlex();
    void updateUndoRedoButtons();   // NEW: Update undo/redo button states
    void simulateMidiCC();
    void updateStatus(const char* control, int value);
    void createStatusArea();        // NEW: Create status/info area
    void handleUndo();              // Undo action
    void handleRedo();              // Redo action
    
private:
    void initHardware();
    void initDesktop();
    void initWindowManager();  // NEW: Initialize window management system
    void createTabs();         // NEW: Create all tabs
    void handleEvents();
    void handleMidiCC(int cc_number, int value);
    void platformDelay(int ms);
    unsigned long getPlatformTick();
};