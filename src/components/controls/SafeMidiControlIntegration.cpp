#include "SafeMidiControlIntegration.h"
#include "components/midi/UnifiedMidiManager.h"
#include <iostream>

void SafeMidiControlIntegration::initialize() {
    if (initialized_) return;
    
    std::cout << "[SafeMidiControlIntegration] Simple initialization" << std::endl;
    resetStatistics();
    initialized_ = true;
}

void SafeMidiControlIntegration::handleControlValueChange(const Parameter* parameter, uint8_t value) {
    if (!parameter) {
        return; // Silent failure to avoid excessive logging
    }
    
    stats_.parameter_updates++;
    
    // Send MIDI output if configured (simplified)
    sendMidiOutput(parameter, value);
    
    // Simplified logging (less frequent)
    if (stats_.parameter_updates % 10 == 0) {
        std::cout << "[SafeMidiControlIntegration] Processed " << stats_.parameter_updates << " parameter updates" << std::endl;
    }
}

void SafeMidiControlIntegration::sendMidiOutput(const Parameter* parameter, uint8_t value) {
    // Simplified MIDI output without excessive singleton access
    try {
        auto& unified_midi = UnifiedMidiManager::getInstance();
        
        if (unified_midi.isConnected() && parameter->getCCNumber() > 0) {
            unified_midi.sendControlChange(1, parameter->getCCNumber(), value);
            stats_.control_changes_sent++;
        }
    } catch (...) {
        // Silent error handling to prevent crashes
    }
}
