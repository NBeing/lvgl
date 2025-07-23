#include "SynthInputHandler.h"
#include "components/midi/MidiClockManager.h"
#include "components/midi/UnifiedMidiManager.h"
#include <iostream>

SynthInputHandler::SynthInputHandler()
    : clock_manager_(nullptr)
    , midi_manager_(nullptr)
{
}

SynthInputHandler::~SynthInputHandler() {
    shutdown();
}

bool SynthInputHandler::initialize() {
    if (initialized_) return true;
    
    std::cout << "SynthInputHandler: Initializing..." << std::endl;
    
    // Get manager instances
    clock_manager_ = &MidiClockManager::getInstance();
    midi_manager_ = &UnifiedMidiManager::getInstance();
    
    // Register for input actions
    registerActions();
    
    initialized_ = true;
    std::cout << "SynthInputHandler: Initialization complete" << std::endl;
    return true;
}

void SynthInputHandler::shutdown() {
    if (!initialized_) return;
    
    std::cout << "SynthInputHandler: Shutting down..." << std::endl;
    unregisterActions();
    initialized_ = false;
}

void SynthInputHandler::onAction(const std::string& action, const Input::Event& event) {
    if (!initialized_) return;
    
    // Only handle key press events for most actions
    if (event.type != Input::EventType::KEY_PRESS && 
        event.type != Input::EventType::BUTTON_PRESS &&
        event.type != Input::EventType::KEY_HOLD) {
        return;
    }
    
    std::cout << "SynthInputHandler: Action '" << action << "' triggered" << std::endl;
    
    // Route to appropriate handler based on action prefix
    if (action.find("transport_") == 0) {
        handleTransportAction(action, event);
    } else if (action.find("bpm_") == 0) {
        handleBPMAction(action, event);
    } else if (action.find("nav_") == 0 || action.find("tab") != std::string::npos) {
        handleNavigationAction(action, event);
    } else if (action.find("param_") == 0) {
        handleParameterAction(action, event);
    } else {
        handleUtilityAction(action, event);
    }
}

void SynthInputHandler::handleTransportAction(const std::string& action, const Input::Event& event) {
    if (!clock_manager_) return;
    
    if (action == "transport_play_pause") {
        auto state = clock_manager_->getTransportState();
        if (state == MidiClockManager::TransportState::PLAYING) {
            clock_manager_->pause();
        } else {
            clock_manager_->play();
        }
    } else if (action == "transport_stop") {
        clock_manager_->stop();
    } else if (action == "transport_continue") {
        clock_manager_->continue_playback();
    }
}

void SynthInputHandler::handleBPMAction(const std::string& action, const Input::Event& event) {
    if (!clock_manager_) return;
    
    float current_bpm = clock_manager_->getBPM();
    float increment = event.hasModifier(Input::Modifier::SHIFT) ? 10.0f : 1.0f;
    
    if (action == "bpm_increase") {
        clock_manager_->setBPM(current_bpm + increment);
    } else if (action == "bpm_decrease") {
        clock_manager_->setBPM(current_bpm - increment);
    }
}

void SynthInputHandler::handleNavigationAction(const std::string& action, const Input::Event& event) {
    // Tab navigation would be handled by the UI system
    // For now, just log the action
    std::cout << "SynthInputHandler: Navigation action '" << action << "' - UI integration needed" << std::endl;
    
    // TODO: Integrate with WindowManager or TabView system
    // Example:
    // if (action == "next_tab") {
    //     WindowManager::getInstance().nextTab();
    // } else if (action == "prev_tab") {
    //     WindowManager::getInstance().previousTab();
    // }
}

void SynthInputHandler::handleParameterAction(const std::string& action, const Input::Event& event) {
    // Parameter adjustment would be handled by the active control
    // For now, just log the action
    std::cout << "SynthInputHandler: Parameter action '" << action << "' - parameter system integration needed" << std::endl;
    
    // TODO: Integrate with ParameterManager or active UI control
    // Example:
    // if (action == "param_inc") {
    //     ParameterManager::getInstance().incrementActiveParameter();
    // }
}

void SynthInputHandler::handleUtilityAction(const std::string& action, const Input::Event& event) {
    if (action == "help") {
        std::cout << "SynthInputHandler: Help requested - showing help dialog" << std::endl;
        // TODO: Show help dialog
    } else if (action == "refresh") {
        std::cout << "SynthInputHandler: Refresh requested - refreshing UI" << std::endl;
        // TODO: Trigger UI refresh
    } else {
        std::cout << "SynthInputHandler: Unknown action '" << action << "'" << std::endl;
    }
}

void SynthInputHandler::registerActions() {
    auto& input_manager = Input::Manager::getInstance();
    
    // Transport actions
    input_manager.registerAction("transport_play_pause", this);
    input_manager.registerAction("transport_stop", this);
    input_manager.registerAction("transport_continue", this);
    
    // BPM actions
    input_manager.registerAction("bpm_increase", this);
    input_manager.registerAction("bpm_decrease", this);
    
    // Navigation actions
    input_manager.registerAction("next_tab", this);
    input_manager.registerAction("prev_tab", this);
    
    // Parameter actions
    input_manager.registerAction("param_up", this);
    input_manager.registerAction("param_down", this);
    input_manager.registerAction("param_inc", this);
    input_manager.registerAction("param_dec", this);
    
    // Utility actions
    input_manager.registerAction("help", this);
    input_manager.registerAction("refresh", this);
    
    std::cout << "SynthInputHandler: Actions registered" << std::endl;
}

void SynthInputHandler::unregisterActions() {
    auto& input_manager = Input::Manager::getInstance();
    
    // Unregister all actions
    input_manager.unregisterAction("transport_play_pause", this);
    input_manager.unregisterAction("transport_stop", this);
    input_manager.unregisterAction("transport_continue", this);
    input_manager.unregisterAction("bpm_increase", this);
    input_manager.unregisterAction("bpm_decrease", this);
    input_manager.unregisterAction("next_tab", this);
    input_manager.unregisterAction("prev_tab", this);
    input_manager.unregisterAction("param_up", this);
    input_manager.unregisterAction("param_down", this);
    input_manager.unregisterAction("param_inc", this);
    input_manager.unregisterAction("param_dec", this);
    input_manager.unregisterAction("help", this);
    input_manager.unregisterAction("refresh", this);
}
