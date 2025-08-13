#include "MidiControlIntegration.h"
#include <iostream>

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
#include "debug/RTEventTracer.h"
#endif

MidiControlIntegration& MidiControlIntegration::getInstance() {
    static MidiControlIntegration instance;
    return instance;
}

void MidiControlIntegration::initialize() {
    std::cout << "[MidiControlIntegration] Initialized" << std::endl;
    
    // Connect to the MIDI parameter bridge for incoming MIDI
    auto& midi_bridge = Parameters::MidiParameterBridge::getInstance();
    midi_bridge.initialize();
    
    resetStatistics();
}

void MidiControlIntegration::handleControlValueChange(const Parameter* parameter, uint8_t value) {
    if (!parameter) {
        std::cout << "[MidiControlIntegration] Warning: Null parameter in value change" << std::endl;
        return;
    }
    
    #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
    // Trace the control value change
    std::string param_data = parameter->getName() + ":" + std::to_string(value);
    TRACE_PARAMETER_EVENT("ParameterControl", "MidiControlIntegration", "controlValueChange", param_data.c_str());
    #endif
    
    stats_.parameter_updates++;
    
    // Send MIDI output if configured
    sendMidiOutput(parameter, value);
    
    // Send MIDI feedback if enabled
    if (midi_feedback_enabled_) {
        sendMidiFeedback(parameter, value);
    }
    
    // Notify UI callback
    if (parameter_changed_callback_) {
        parameter_changed_callback_(value, parameter);
    }
    
    std::cout << "[MidiControlIntegration] Parameter '" << parameter->getName() 
              << "' changed to " << (int)value << std::endl;
}

void MidiControlIntegration::sendMidiOutput(const Parameter* parameter, uint8_t value) {
    auto& unified_midi = UnifiedMidiManager::getInstance();
    
    if (unified_midi.isConnected() && parameter->getCCNumber() > 0) {
        unified_midi.sendControlChange(1, parameter->getCCNumber(), value);
        stats_.control_changes_sent++;
        
        #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
        // Trace the MIDI output
        std::string cc_data = "CC" + std::to_string(parameter->getCCNumber()) + ":" + std::to_string(value);
        TRACE_MIDI_EVENT("MidiControlIntegration", "ExternalSynth", "sendCC", cc_data.c_str());
        #endif
        
        std::cout << "[MidiControlIntegration] Sent MIDI CC " << (int)parameter->getCCNumber() 
                  << " = " << (int)value << std::endl;
    }
}

void MidiControlIntegration::sendMidiFeedback(const Parameter* parameter, uint8_t value) {
    // For now, feedback is the same as output
    // In a full implementation, this might go to a different MIDI channel
    // or use a different mechanism
    stats_.midi_feedback_sent++;
}

void MidiControlIntegration::onClockTick() {
    // Handle tempo-synced parameter updates
    // This could be used for LFO-synced controls or other clock-dependent parameters
    stats_.clock_synced_updates++;
}

void MidiControlIntegration::onClockStart() {
    std::cout << "[MidiControlIntegration] Clock started - resetting tempo-synced controls" << std::endl;
}

void MidiControlIntegration::onClockStop() {
    std::cout << "[MidiControlIntegration] Clock stopped" << std::endl;
}
