#pragma once

#include <lvgl.h>
#include <memory>

#include "components/parameter/ParameterBinder.h"
#include "components/parameter/CommandManager.h"
#include "components/ui/WindowManager.h"
#include "components/ui/MainControlTab.h"
#include "components/ui/HelloTab.h"
#include "components/ui/WorldTab.h"
#include "components/ui/SettingsTab.h"
#include "components/ui/ClockTab.h"
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
    #else
        // Desktop LVGL objects
        lv_display_t* display_;
        lv_indev_t* mouse_;
        lv_indev_t* keyboard_;
        lv_indev_t* mousewheel_;
    #endif
    
    // Core components
    std::shared_ptr<MidiHandler> midi_handler_;  // Shared so UnifiedMidiManager can use it
    
    // Parameter system
    std::unique_ptr<ParameterBinder> parameter_binder_;
    std::unique_ptr<CommandManager> command_manager_;
    
    // Window management system
    std::unique_ptr<WindowManager> window_manager_;
    
    // Tab instances
    std::unique_ptr<MainControlTab> main_tab_;
    std::unique_ptr<HelloTab> hello_tab_;
    std::unique_ptr<WorldTab> world_tab_;
    std::unique_ptr<SettingsTab> settings_tab_;
    std::unique_ptr<ClockTab> clock_tab_;
    
public:
    SynthApp();
    ~SynthApp();
    
    void setup();
    void loop();
    
    // WindowObserver interface
    void onTabChanged(const std::string& old_tab, const std::string& new_tab) override;
    
    // Utility methods
    void simulateMidiCC();
    void testHardwareMidi();
    bool isInitialized() const;
    
    // Access to shared components
    std::shared_ptr<MidiHandler> getMidiHandler() const;
    
private:
    // Initialization methods
    void initHardware();
    #if !defined(ESP32_BUILD)
    void initDesktop();
    #endif
    void initWindowManager();
    void createTabs();
};
