#include "Parameter.h"
#include <algorithm>
#include <cmath>

Parameter::Parameter(const std::string& name, 
                    const std::string& short_name,
                    uint8_t cc_number,
                    ParameterCategory category,
                    uint8_t min_value,
                    uint8_t max_value,
                    uint8_t default_value,
                    const std::string& description)
    : name_(name)
    , short_name_(short_name)
    , cc_number_(cc_number)
    , category_(category)
    , min_value_(min_value)
    , max_value_(max_value)
    , default_value_(default_value)
    , description_(description)
    , current_value_(default_value)
    , is_bipolar_(false)
{
    // Auto-detect bipolar parameters based on common patterns
    if (name.find("Detune") != std::string::npos || 
        name.find("Bend") != std::string::npos ||
        name.find("Fine") != std::string::npos ||
        name.find("Cent") != std::string::npos) {
        is_bipolar_ = true;
    }
}

void Parameter::setValue(uint8_t value) {
    // Clamp to valid range
    value = std::max(min_value_, std::min(max_value_, value));
    
    if (current_value_ != value) {
        current_value_ = value;
        notifyObservers();
    }
}

void Parameter::resetToDefault() {
    setValue(default_value_);
}

bool Parameter::isAtDefault() const {
    return current_value_ == default_value_;
}

float Parameter::getValueAsPercent() const {
    if (max_value_ == min_value_) return 0.0f;
    return static_cast<float>(current_value_ - min_value_) / (max_value_ - min_value_);
}

void Parameter::setValueFromPercent(float percent) {
    percent = std::max(0.0f, std::min(1.0f, percent));
    uint8_t new_value = min_value_ + static_cast<uint8_t>(percent * (max_value_ - min_value_));
    setValue(new_value);
}

bool Parameter::isBipolar() const {
    return is_bipolar_;
}

void Parameter::setBipolar(bool bipolar) {
    is_bipolar_ = bipolar;
}

int Parameter::getBipolarValue() const {
    if (!is_bipolar_) return current_value_;
    
    // Convert 0-127 MIDI value to -64 to +63 range
    return static_cast<int>(current_value_) - 64;
}

void Parameter::setBipolarValue(int value) {
    if (!is_bipolar_) {
        setValue(static_cast<uint8_t>(value));
        return;
    }
    
    // Clamp to bipolar range
    value = std::max(-64, std::min(63, value));
    
    // Convert -64 to +63 range to 0-127 MIDI value
    setValue(static_cast<uint8_t>(value + 64));
}

void Parameter::addObserver(std::shared_ptr<ParameterObserver> observer) {
    observers_.push_back(observer);
}

void Parameter::removeObserver(std::shared_ptr<ParameterObserver> observer) {
    auto it = std::find_if(observers_.begin(), observers_.end(),
        [&observer](const std::weak_ptr<ParameterObserver>& weak_obs) {
            return weak_obs.lock() == observer;
        });
    
    if (it != observers_.end()) {
        observers_.erase(it);
    }
}

std::string Parameter::getCategoryName() const {
    return ParameterUtils::categoryToString(category_);
}

std::string Parameter::getValueDisplayText() const {
    if (is_bipolar_) {
        int bipolar_val = getBipolarValue();
        if (bipolar_val >= 0) {
            return "+" + std::to_string(bipolar_val);
        } else {
            return std::to_string(bipolar_val);
        }
    } else {
        return std::to_string(current_value_);
    }
}

void Parameter::notifyObservers() {
    cleanupObservers();
    
    for (auto& weak_obs : observers_) {
        if (auto obs = weak_obs.lock()) {
            obs->onParameterChanged(*this);
        }
    }
}

void Parameter::cleanupObservers() {
    observers_.erase(
        std::remove_if(observers_.begin(), observers_.end(),
            [](const std::weak_ptr<ParameterObserver>& weak_obs) {
                return weak_obs.expired();
            }),
        observers_.end());
}

// ParameterUtils implementation
std::string ParameterUtils::categoryToString(ParameterCategory category) {
    switch (category) {
        case ParameterCategory::SYSTEM: return "System";
        case ParameterCategory::VOICE: return "Voice";
        case ParameterCategory::OSCILLATORS: return "Oscillators";
        case ParameterCategory::MIXER: return "Mixer";
        case ParameterCategory::FILTERS: return "Filters";
        case ParameterCategory::ENVELOPES: return "Envelopes";
        case ParameterCategory::LFOS: return "LFOs";
        case ParameterCategory::MUTATORS: return "Mutators";
        case ParameterCategory::MACROS: return "Macros";
        case ParameterCategory::ARPEGGIATOR: return "Arpeggiator";
        case ParameterCategory::EFFECTS: return "Effects";
        case ParameterCategory::AMPLITUDE: return "Amplitude";
        default: return "Unknown";
    }
}

ParameterCategory ParameterUtils::stringToCategory(const std::string& category_str) {
    if (category_str == "System") return ParameterCategory::SYSTEM;
    if (category_str == "Voice") return ParameterCategory::VOICE;
    if (category_str == "Oscillators") return ParameterCategory::OSCILLATORS;
    if (category_str == "Mixer") return ParameterCategory::MIXER;
    if (category_str == "Filters") return ParameterCategory::FILTERS;
    if (category_str == "Envelopes") return ParameterCategory::ENVELOPES;
    if (category_str == "LFOs") return ParameterCategory::LFOS;
    if (category_str == "Mutators") return ParameterCategory::MUTATORS;
    if (category_str == "Macros") return ParameterCategory::MACROS;
    if (category_str == "Arpeggiator") return ParameterCategory::ARPEGGIATOR;
    if (category_str == "Effects") return ParameterCategory::EFFECTS;
    if (category_str == "Amplitude") return ParameterCategory::AMPLITUDE;
    return ParameterCategory::UNKNOWN;
}

std::vector<ParameterCategory> ParameterUtils::getAllCategories() {
    return {
        ParameterCategory::SYSTEM,
        ParameterCategory::VOICE,
        ParameterCategory::OSCILLATORS,
        ParameterCategory::MIXER,
        ParameterCategory::FILTERS,
        ParameterCategory::ENVELOPES,
        ParameterCategory::LFOS,
        ParameterCategory::MUTATORS,
        ParameterCategory::MACROS,
        ParameterCategory::ARPEGGIATOR,
        ParameterCategory::EFFECTS,
        ParameterCategory::AMPLITUDE
    };
}
