#include "MidiParameterBridge.h"
#include <iostream>

namespace Parameters {

MidiParameterBridge& MidiParameterBridge::getInstance() {
    static MidiParameterBridge instance;
    return instance;
}

void MidiParameterBridge::initialize() {
    if (initialized_.load()) return;
    
    std::cout << "[MidiParameterBridge] Initializing MIDI to parameter bridge..." << std::endl;
    
    initialized_.store(true);
    std::cout << "[MidiParameterBridge] ⚡ MIDI parameter bridge initialized" << std::endl;
}

void MidiParameterBridge::processMidiCC(uint8_t channel, uint8_t cc, uint8_t value) {
    if (!initialized_.load()) return;
    
    stats_.midi_cc_received.fetch_add(1);
    
    // Look up parameter mapping
    auto& param_manager = ParameterManager::getInstance();
    ParameterID mapped_param = param_manager.getMidiMapping(channel, cc);
    
    if (mapped_param != 0) {
        // Convert MIDI value to normalized parameter value
        float normalized_value = midiValueToNormalized(value);
        
        // Create parameter change event
        ParameterChangeEvent event = ParameterChangeEvent::fromMidiCC(
            mapped_param, normalized_value, channel, cc);
        
        // Process through parameter manager
        param_manager.processParameterChange(event);
        
        stats_.midi_cc_mapped.fetch_add(1);
        
        std::cout << "[MidiParameterBridge] 🎛️ MIDI CC" << (int)cc 
                  << " Ch" << (int)channel << " (" << (int)value 
                  << ") --> Parameter " << mapped_param 
                  << " (" << normalized_value << ")" << std::endl;
    } else {
        // Handle MIDI learn if active
        if (param_manager.isMidiLearning()) {
            // The parameter manager will handle the MIDI learn assignment
            ParameterID learn_param = param_manager.getMidiLearnParameter();
            if (learn_param != 0) {
                float normalized_value = midiValueToNormalized(value);
                ParameterChangeEvent event = ParameterChangeEvent::fromMidiCC(
                    learn_param, normalized_value, channel, cc);
                param_manager.processParameterChange(event);
                
                stats_.midi_cc_mapped.fetch_add(1);
                return;
            }
        }
        
        stats_.midi_cc_unmapped.fetch_add(1);
        
        // Uncomment for debugging unmapped MIDI CCs
        // std::cout << "[MidiParameterBridge] Unmapped MIDI CC" << (int)cc 
        //           << " Ch" << (int)channel << " = " << (int)value << std::endl;
    }
}

void MidiParameterBridge::processMidiNRPN(uint8_t channel, uint16_t nrpn, uint16_t value) {
    // NRPN support for high-resolution parameter control
    // Convert 14-bit NRPN value to normalized float
    float normalized_value = static_cast<float>(value) / 16383.0f;
    
    // For now, treat NRPN as extended parameter IDs
    // In a full implementation, you'd have an NRPN mapping table
    ParameterID nrpn_param = 10000 + nrpn; // Offset to avoid conflicts
    
    ParameterChangeEvent event(nrpn_param, normalized_value, ParameterSource::MIDI_NRPN);
    event.midi_channel = channel;
    
    auto& param_manager = ParameterManager::getInstance();
    param_manager.processParameterChange(event);
}

void MidiParameterBridge::sendMidiFeedback(const ParameterChangeEvent& event) {
    // Send parameter changes back as MIDI CC for external device feedback
    // This would integrate with MidiHandler to send outbound MIDI
    
    stats_.midi_feedback_sent.fetch_add(1);
    
    // Implementation would go here when MidiHandler integration is ready
    // For now, just log the feedback
    std::cout << "[MidiParameterBridge] 📤 MIDI feedback: Parameter " 
              << event.parameter_id << " = " << event.normalized_value << std::endl;
}

float MidiParameterBridge::midiValueToNormalized(uint8_t midi_value) {
    // Convert 7-bit MIDI value (0-127) to normalized float (0.0-1.0)
    return static_cast<float>(midi_value) / 127.0f;
}

uint8_t MidiParameterBridge::normalizedToMidiValue(float normalized) {
    // Convert normalized float (0.0-1.0) to 7-bit MIDI value (0-127)
    return static_cast<uint8_t>(normalized * 127.0f + 0.5f); // +0.5f for rounding
}

} // namespace Parameters
