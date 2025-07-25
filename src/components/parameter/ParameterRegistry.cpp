#include "ParameterRegistry.h"
#include <iostream>
#include <algorithm>

namespace Parameters {

ParameterRegistry& ParameterRegistry::getInstance() {
    static ParameterRegistry instance;
    return instance;
}

void ParameterRegistry::registerParameter(const ParameterInfo& info) {
    parameters_[info.id] = info;
    std::cout << "[ParameterRegistry] Registered parameter: " << info.name 
              << " (ID: " << info.id << ")" << std::endl;
}

const ParameterInfo* ParameterRegistry::getParameterInfo(ParameterID id) const {
    auto it = parameters_.find(id);
    return (it != parameters_.end()) ? &it->second : nullptr;
}

bool ParameterRegistry::hasParameter(ParameterID id) const {
    return parameters_.find(id) != parameters_.end();
}

std::vector<ParameterID> ParameterRegistry::getAllParameterIDs() const {
    std::vector<ParameterID> ids;
    ids.reserve(parameters_.size());
    
    for (const auto& pair : parameters_) {
        ids.push_back(pair.first);
    }
    
    return ids;
}

std::vector<ParameterID> ParameterRegistry::getAudioParameters() const {
    std::vector<ParameterID> audio_params;
    
    for (const auto& pair : parameters_) {
        if (pair.second.affects_audio) {
            audio_params.push_back(pair.first);
        }
    }
    
    return audio_params;
}

std::vector<ParameterID> ParameterRegistry::getUIParameters() const {
    std::vector<ParameterID> ui_params;
    
    for (const auto& pair : parameters_) {
        if (pair.second.affects_ui) {
            ui_params.push_back(pair.first);
        }
    }
    
    return ui_params;
}

std::vector<ParameterID> ParameterRegistry::getMidiLearnableParameters() const {
    std::vector<ParameterID> learnable_params;
    
    for (const auto& pair : parameters_) {
        if (pair.second.midi_learnable) {
            learnable_params.push_back(pair.first);
        }
    }
    
    return learnable_params;
}

float ParameterRegistry::normalizeValue(ParameterID id, float real_value) const {
    const auto* info = getParameterInfo(id);
    if (!info) return 0.0f;
    
    return info->normalizeValue(real_value);
}

float ParameterRegistry::denormalizeValue(ParameterID id, float normalized_value) const {
    const auto* info = getParameterInfo(id);
    if (!info) return 0.0f;
    
    return info->denormalizeValue(normalized_value);
}

std::string ParameterRegistry::formatValue(ParameterID id, float real_value) const {
    const auto* info = getParameterInfo(id);
    if (!info) return "Unknown";
    
    return info->formatValue(real_value);
}

} // namespace Parameters
