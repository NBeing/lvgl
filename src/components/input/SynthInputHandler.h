#pragma once

#include "InputObserver.h"
#include "InputManager.h"
#include <string>

// Forward declarations
class MidiClockManager;
class UnifiedMidiManager;

/**
 * @brief Synth-specific input handler
 * 
 * Handles input actions for synthesizer functions like transport controls,
 * parameter adjustment, and navigation.
 */
class SynthInputHandler : public Input::ActionObserver {
private:
    MidiClockManager* clock_manager_;
    UnifiedMidiManager* midi_manager_;
    bool initialized_ = false;
    
public:
    SynthInputHandler();
    ~SynthInputHandler();
    
    bool initialize();
    void shutdown();
    
    // ActionObserver interface
    void onAction(const std::string& action, const Input::Event& event) override;
    
private:
    void handleTransportAction(const std::string& action, const Input::Event& event);
    void handleBPMAction(const std::string& action, const Input::Event& event);
    void handleNavigationAction(const std::string& action, const Input::Event& event);
    void handleParameterAction(const std::string& action, const Input::Event& event);
    void handleUtilityAction(const std::string& action, const Input::Event& event);
    
    void registerActions();
    void unregisterActions();
};
