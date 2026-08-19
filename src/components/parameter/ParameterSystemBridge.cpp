#include "components/ui/ParameterSelectionUI.h"
#include "components/parameter/ParameterRegistry.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

// ============================================================================
// ParameterSystemBridge Implementation
// ============================================================================

ParameterSystemBridge::ParameterSystemBridge(SynthParameterManager* synth_manager, 
                                           Parameters::ParameterManager* param_manager)
    : synth_manager_(synth_manager)
    , param_manager_(param_manager)
{
    std::cout << "[ParameterSystemBridge] 🌉 Initialized parameter system bridge" << std::endl;
}

bool ParameterSystemBridge::assignParameter(std::shared_ptr<Parameter> synth_parameter,
                                           const std::string& target_id,
                                           const std::string& assignment_type) {
    if (!synth_parameter) {
        std::cerr << "[ParameterSystemBridge] ❌ Cannot assign null parameter" << std::endl;
        return false;
    }
    
    std::cout << "[ParameterSystemBridge] 🔗 Assigning " << synth_parameter->getName() 
              << " to " << target_id << " (" << assignment_type << ")" << std::endl;
    
    try {
        if (assignment_type == "midi_cc") {
            return assignToMidiCC(synth_parameter, target_id);
        } else if (assignment_type == "ui_control") {
            return assignToUIControl(synth_parameter, target_id);
        } else if (assignment_type == "automation") {
            return assignToAutomation(synth_parameter, target_id);
        } else {
            std::cerr << "[ParameterSystemBridge] ❌ Unknown assignment type: " << assignment_type << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ParameterSystemBridge] ❌ Assignment failed: " << e.what() << std::endl;
        return false;
    }
}

bool ParameterSystemBridge::startMidiLearn(std::shared_ptr<Parameter> synth_parameter) {
    if (!synth_parameter) {
        std::cerr << "[ParameterSystemBridge] ❌ Cannot start MIDI learn for null parameter" << std::endl;
        return false;
    }
    
    std::cout << "[ParameterSystemBridge] 🎹 Starting MIDI learn for: " << synth_parameter->getName() << std::endl;
    
    try {
        // Map synthesizer parameter to internal parameter ID
        Parameters::ParameterID param_id = mapToParameterID(synth_parameter);
        
        // Start MIDI learn in the parameter manager
        param_manager_->startMidiLearn(param_id);
        
        // Update assignment in synth manager
        synth_manager_->assignParameter(synth_parameter->getName(), 
                                      "midi_learn_pending", 
                                      "midi_cc");
        
        std::cout << "[ParameterSystemBridge] ✅ MIDI learn started for " << synth_parameter->getName() << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[ParameterSystemBridge] ❌ MIDI learn failed: " << e.what() << std::endl;
        return false;
    }
}

bool ParameterSystemBridge::assignToAutomation(std::shared_ptr<Parameter> synth_parameter,
                                              const std::string& automation_track_id) {
    if (!synth_parameter) return false;
    
    std::cout << "[ParameterSystemBridge] 🤖 Creating automation assignment for: " 
              << synth_parameter->getName() << std::endl;
    
    try {
        // Map to internal parameter system
        Parameters::ParameterID param_id = mapToParameterID(synth_parameter);
        
        // Create automation assignment (this would integrate with your sequencer)
        // For now, we'll just track it in the synth manager
        synth_manager_->assignParameter(synth_parameter->getName(),
                                      automation_track_id,
                                      "automation");
        
        std::cout << "[ParameterSystemBridge] ✅ Automation assignment created" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[ParameterSystemBridge] ❌ Automation assignment failed: " << e.what() << std::endl;
        return false;
    }
}

std::string ParameterSystemBridge::getAssignmentStatus(std::shared_ptr<Parameter> synth_parameter) const {
    if (!synth_parameter) return "";
    
    const auto* assignment = synth_manager_->getAssignment(synth_parameter->getName());
    if (!assignment) {
        return "Not assigned";
    }
    
    std::stringstream status;
    status << "Assigned to " << assignment->control_id;
    status << " (" << assignment->assignment_type << ")";
    
    if (!assignment->is_active) {
        status << " [Inactive]";
    }
    
    return status.str();
}

bool ParameterSystemBridge::removeAssignments(std::shared_ptr<Parameter> synth_parameter) {
    if (!synth_parameter) return false;
    
    std::cout << "[ParameterSystemBridge] 🗑️ Removing assignments for: " 
              << synth_parameter->getName() << std::endl;
    
    try {
        // Remove from synth manager
        bool removed = synth_manager_->unassignParameter(synth_parameter->getName());
        
        // Also remove from core parameter manager if it exists
        Parameters::ParameterID param_id = mapToParameterID(synth_parameter);
        
        // Remove MIDI mappings (this would need to be implemented in your ParameterManager)
        // param_manager_->removeMidiMapping(param_id);
        
        std::cout << "[ParameterSystemBridge] " << (removed ? "✅" : "ℹ️") 
                  << " Assignment removal completed" << std::endl;
        
        return removed;
        
    } catch (const std::exception& e) {
        std::cerr << "[ParameterSystemBridge] ❌ Assignment removal failed: " << e.what() << std::endl;
        return false;
    }
}

// ============================================================================
// Private Helper Methods
// ============================================================================

bool ParameterSystemBridge::assignToMidiCC(std::shared_ptr<Parameter> synth_parameter,
                                          const std::string& target_id) {
    // Parse MIDI channel and CC from target_id (e.g., "midi_ch1_cc74")
    uint8_t channel = 1;
    uint8_t cc = synth_parameter->getCCNumber();
    
    // Try to parse channel and CC from target_id
    if (target_id.find("midi_ch") == 0) {
        size_t ch_pos = target_id.find("ch") + 2;
        size_t cc_pos = target_id.find("_cc");
        
        if (cc_pos != std::string::npos) {
            channel = std::stoi(target_id.substr(ch_pos, cc_pos - ch_pos));
            cc = std::stoi(target_id.substr(cc_pos + 3));
        }
    }
    
    std::cout << "[ParameterSystemBridge] 🎹 Assigning to MIDI Ch" << (int)channel 
              << " CC" << (int)cc << std::endl;
    
    // Map to internal parameter ID
    Parameters::ParameterID param_id = mapToParameterID(synth_parameter);
    
    // Assign in parameter manager
    param_manager_->assignMidiCC(param_id, channel, cc);
    
    // Track in synth manager
    synth_manager_->assignParameter(synth_parameter->getName(),
                                  target_id,
                                  "midi_cc");
    
    return true;
}

bool ParameterSystemBridge::assignToUIControl(std::shared_ptr<Parameter> synth_parameter,
                                             const std::string& target_id) {
    std::cout << "[ParameterSystemBridge] 🎛️ Assigning to UI control: " << target_id << std::endl;
    
    // This would integrate with your existing UI parameter binding system
    // For now, just track it in the synth manager
    synth_manager_->assignParameter(synth_parameter->getName(),
                                  target_id,
                                  "ui_control");
    
    return true;
}

Parameters::ParameterID ParameterSystemBridge::mapToParameterID(std::shared_ptr<Parameter> synth_parameter) const {
    // Map synthesizer parameter to internal parameter ID
    // This is a simplified mapping - in a real implementation, you'd have a proper mapping system
    
    const std::string& param_name = synth_parameter->getName();
    
    // Map common parameter names to internal IDs
    if (param_name.find("Filter") != std::string::npos && param_name.find("Cutoff") != std::string::npos) {
        return Parameters::ParameterIDs::FILTER_CUTOFF;
    } else if (param_name.find("Filter") != std::string::npos && param_name.find("Resonance") != std::string::npos) {
        return Parameters::ParameterIDs::FILTER_RESONANCE;
    } else if (param_name.find("Attack") != std::string::npos) {
        return Parameters::ParameterIDs::ENV_ATTACK;
    } else if (param_name.find("Volume") != std::string::npos || param_name.find("Master") != std::string::npos) {
        return Parameters::ParameterIDs::MASTER_VOLUME;
    } else if (param_name.find("BPM") != std::string::npos || param_name.find("Tempo") != std::string::npos) {
        return Parameters::ParameterIDs::CLOCK_BPM;
    }
    
    // For unmapped parameters, create a dynamic mapping
    return createDynamicParameterID(synth_parameter);
}

Parameters::ParameterID ParameterSystemBridge::createDynamicParameterID(std::shared_ptr<Parameter> synth_parameter) const {
    // Create a unique parameter ID for synthesizer parameters that don't map to existing ones
    // This would typically involve registering new parameters in the ParameterRegistry
    
    std::string unique_name = "synth_" + synth_parameter->getName();
    std::replace(unique_name.begin(), unique_name.end(), ' ', '_');
    std::transform(unique_name.begin(), unique_name.end(), unique_name.begin(), ::tolower);
    
    // Generate a hash-based ID (simplified approach)
    uint32_t param_id = 0x10000; // Start dynamic IDs at 0x10000
    for (char c : unique_name) {
        param_id = param_id * 31 + c;
    }
    
    std::cout << "[ParameterSystemBridge] 🆔 Created dynamic parameter ID: 0x" 
              << std::hex << param_id << " for " << synth_parameter->getName() << std::endl;
    
    return param_id;
}

std::string ParameterSystemBridge::createParameterIDMapping(std::shared_ptr<Parameter> synth_parameter) {
    // Create a mapping entry for tracking synthesizer parameters
    std::stringstream mapping;
    mapping << "synth_param_" << synth_parameter->getCCNumber();
    return mapping.str();
}