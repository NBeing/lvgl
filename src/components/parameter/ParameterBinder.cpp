#include "ParameterBinder.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

// ============================================================================
// ParameterBinder Implementation
// ============================================================================

ParameterBinder::ParameterBinder() {
    // Empty constructor - parameters loaded on demand
}

ParameterBinder::~ParameterBinder() {
    // Smart pointers handle cleanup automatically
}

bool ParameterBinder::loadSynthDefinition(const std::string& synth_name) {
    if (synth_name == "hydrasynth") {
        // Create Hydrasynth definition from our existing JSON
        current_synth_ = std::make_unique<SynthDefinition>();
        current_synth_->name = "ASM Hydrasynth";
        current_synth_->manufacturer = "Ashun Sound Machines";
        current_synth_->description = "8-voice polyphonic wavetable synthesizer";
        
        // Create some key parameters for demonstration
        current_synth_->parameters = ParameterFactory::createHydrasynthParameters();
        
        buildParameterMaps(*current_synth_);
        return true;
    }
    
    return false;
}

std::vector<std::string> ParameterBinder::getAvailableSynths() const {
    return {"hydrasynth"};  // For now, just Hydrasynth
}

std::string ParameterBinder::getCurrentSynthName() const {
    return current_synth_ ? current_synth_->name : "";
}

std::shared_ptr<Parameter> ParameterBinder::findParameterByName(const std::string& name) const {
    if (!current_synth_) return nullptr;
    
    auto it = current_synth_->parameter_by_name.find(name);
    return (it != current_synth_->parameter_by_name.end()) ? it->second : nullptr;
}

std::shared_ptr<Parameter> ParameterBinder::getParameter(const std::string& name) const {
    return findParameterByName(name);
}

std::shared_ptr<Parameter> ParameterBinder::findParameterByCC(uint8_t cc_number) const {
    if (!current_synth_) return nullptr;
    
    auto it = current_synth_->parameter_by_cc.find(cc_number);
    return (it != current_synth_->parameter_by_cc.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<Parameter>> ParameterBinder::getParametersByCategory(ParameterCategory category) const {
    if (!current_synth_) return {};
    
    auto it = current_synth_->parameters_by_category.find(category);
    return (it != current_synth_->parameters_by_category.end()) ? it->second : std::vector<std::shared_ptr<Parameter>>();
}

std::vector<std::shared_ptr<Parameter>> ParameterBinder::searchParameters(const std::string& query) const {
    if (!current_synth_) return {};
    
    std::vector<std::shared_ptr<Parameter>> results;
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    for (const auto& param : current_synth_->parameters) {
        std::string param_name = param->getName();
        std::string param_short = param->getShortName();
        std::string param_desc = param->getDescription();
        
        std::transform(param_name.begin(), param_name.end(), param_name.begin(), ::tolower);
        std::transform(param_short.begin(), param_short.end(), param_short.begin(), ::tolower);
        std::transform(param_desc.begin(), param_desc.end(), param_desc.begin(), ::tolower);
        
        if (param_name.find(lower_query) != std::string::npos ||
            param_short.find(lower_query) != std::string::npos ||
            param_desc.find(lower_query) != std::string::npos) {
            results.push_back(param);
        }
    }
    
    return results;
}

std::vector<std::shared_ptr<Parameter>> ParameterBinder::getAllParameters() const {
    return current_synth_ ? current_synth_->parameters : std::vector<std::shared_ptr<Parameter>>();
}

void ParameterBinder::bindControlToParameter(const ControlId& control_id, const std::string& parameter_name) {
    auto parameter = findParameterByName(parameter_name);
    if (parameter) {
        control_bindings_[control_id] = parameter;
    }
}

void ParameterBinder::bindControlToParameter(const ControlId& control_id, std::shared_ptr<Parameter> parameter) {
    if (parameter) {
        control_bindings_[control_id] = parameter;
    }
}

void ParameterBinder::unbindControl(const ControlId& control_id) {
    control_bindings_.erase(control_id);
}

std::shared_ptr<Parameter> ParameterBinder::getParameterForControl(const ControlId& control_id) const {
    auto it = control_bindings_.find(control_id);
    return (it != control_bindings_.end()) ? it->second : nullptr;
}

size_t ParameterBinder::getParameterCount() const {
    return current_synth_ ? current_synth_->parameters.size() : 0;
}

void ParameterBinder::buildParameterMaps(SynthDefinition& synth_def) {
    synth_def.parameter_by_name.clear();
    synth_def.parameter_by_cc.clear();
    synth_def.parameters_by_category.clear();
    
    for (const auto& param : synth_def.parameters) {
        // Build name lookup
        synth_def.parameter_by_name[param->getName()] = param;
        
        // Build CC lookup
        synth_def.parameter_by_cc[param->getCCNumber()] = param;
        
        // Build category lookup
        synth_def.parameters_by_category[param->getCategory()].push_back(param);
    }
}

// ============================================================================
// ParameterFactory Implementation
// ============================================================================

std::vector<std::shared_ptr<Parameter>> ParameterFactory::createHydrasynthParameters() {
    std::vector<std::shared_ptr<Parameter>> parameters;
    
    // System parameters
    parameters.push_back(std::make_shared<Parameter>(
        "Master Volume", "Volume", 7, ParameterCategory::SYSTEM, 0, 127, 100,
        "Global volume control"
    ));
    
    parameters.push_back(std::make_shared<Parameter>(
        "Modulation Wheel", "Mod Wheel", 1, ParameterCategory::SYSTEM, 0, 127, 0,
        "Standard modulation wheel control"
    ));
    
    // Filter parameters
    parameters.push_back(std::make_shared<Parameter>(
        "Filter 1 Cutoff", "F1 Cutoff", 74, ParameterCategory::FILTERS, 0, 127, 64,
        "Cutoff frequency of filter 1"
    ));
    
    parameters.push_back(std::make_shared<Parameter>(
        "Filter 1 Resonance", "F1 Res", 71, ParameterCategory::FILTERS, 0, 127, 0,
        "Resonance amount of filter 1"
    ));
    
    parameters.push_back(std::make_shared<Parameter>(
        "Filter 1 Drive", "F1 Drive", 50, ParameterCategory::FILTERS, 0, 127, 0,
        "Drive/saturation amount for filter 1"
    ));
    
    // Envelope parameters
    parameters.push_back(std::make_shared<Parameter>(
        "ENV 1 Attack", "E1 Att", 81, ParameterCategory::ENVELOPES, 0, 127, 0,
        "Attack time of envelope 1"
    ));
    
    parameters.push_back(std::make_shared<Parameter>(
        "ENV 1 Decay", "E1 Dec", 82, ParameterCategory::ENVELOPES, 0, 127, 64,
        "Decay time of envelope 1"
    ));
    
    parameters.push_back(std::make_shared<Parameter>(
        "ENV 1 Sustain", "E1 Sus", 83, ParameterCategory::ENVELOPES, 0, 127, 100,
        "Sustain level of envelope 1"
    ));
    
    parameters.push_back(std::make_shared<Parameter>(
        "ENV 1 Release", "E1 Rel", 84, ParameterCategory::ENVELOPES, 0, 127, 32,
        "Release time of envelope 1"
    ));
    
    // LFO parameters
    parameters.push_back(std::make_shared<Parameter>(
        "LFO 1 Rate", "LFO1 Rate", 72, ParameterCategory::LFOS, 0, 127, 64,
        "Rate/speed of LFO 1"
    ));
    
    parameters.push_back(std::make_shared<Parameter>(
        "LFO 1 Gain", "LFO1 Gain", 70, ParameterCategory::LFOS, 0, 127, 0,
        "Output level of LFO 1"
    ));
    
    // Oscillator parameters
    parameters.push_back(std::make_shared<Parameter>(
        "OSC 1 Volume", "O1 Vol", 44, ParameterCategory::OSCILLATORS, 0, 127, 100,
        "Volume level of oscillator 1"
    ));
    
    parameters.push_back(std::make_shared<Parameter>(
        "OSC 1 Wavscan", "O1 Wave", 24, ParameterCategory::OSCILLATORS, 0, 127, 0,
        "Wavetable scanning position for oscillator 1"
    ));
    
    // Macro parameters
    parameters.push_back(std::make_shared<Parameter>(
        "Macro 1", "Macro 1", 16, ParameterCategory::MACROS, 0, 127, 64,
        "Assignable macro control 1"
    ));
    
    parameters.push_back(std::make_shared<Parameter>(
        "Macro 2", "Macro 2", 17, ParameterCategory::MACROS, 0, 127, 64,
        "Assignable macro control 2"
    ));
    
    // Mutator parameters
    parameters.push_back(std::make_shared<Parameter>(
        "Mutator 1 Depth", "M1 Depth", 30, ParameterCategory::MUTATORS, 0, 127, 0,
        "Processing depth for mutator 1"
    ));
    
    return parameters;
}

std::shared_ptr<Parameter> ParameterFactory::createFilterCutoff(uint8_t cc) {
    return std::make_shared<Parameter>(
        "Filter Cutoff", "Cutoff", cc, ParameterCategory::FILTERS, 0, 127, 64,
        "Filter cutoff frequency"
    );
}

std::shared_ptr<Parameter> ParameterFactory::createFilterResonance(uint8_t cc) {
    return std::make_shared<Parameter>(
        "Filter Resonance", "Resonance", cc, ParameterCategory::FILTERS, 0, 127, 0,
        "Filter resonance/emphasis"
    );
}
