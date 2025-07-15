#pragma once
#include "components/ui/Window.h"
#include "components/ui/MidiMonitor.h"

/**
 * @brief MIDI Monitor tab for real-time MIDI message monitoring
 * 
 * This tab provides a live view of all incoming and outgoing MIDI messages
 * across all available MIDI backends (hardware, USB, etc.)
 */
class MidiMonitorTab : public Tab {
public:
    MidiMonitorTab();
    virtual ~MidiMonitorTab() = default;

    // Tab interface
    void create(lv_obj_t* parent) override;
    
    // Access to the monitor for external MIDI logging
    UI::MidiMonitor& getMonitor() { return monitor_; }
    
protected:
    void onActivated() override;
    void onDeactivated() override;
    
private:
    lv_obj_t* container_ = nullptr;
    UI::MidiMonitor monitor_;
};
