#include "ParameterRegistry.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace Parameters {

// Forward declaration
void initializeDefaultParameters();

/**
 * @brief Initialize default parameters for the synthesizer
 * 
 * This function registers all the standard parameters with their
 * metadata, value ranges, and formatting functions.
 */
void initializeDefaultParameters() {
    auto& registry = ParameterRegistry::getInstance();
    
    // Filter Cutoff Parameter
    {
        ParameterInfo cutoff;
        cutoff.id = ParameterIDs::FILTER_CUTOFF;
        cutoff.name = "Filter Cutoff";
        cutoff.short_name = "Cutoff";
        cutoff.min_value = 20.0f;      // 20 Hz
        cutoff.max_value = 20000.0f;   // 20 kHz
        cutoff.default_value = 1000.0f; // 1 kHz
        cutoff.affects_audio = true;   // RT processing needed
        cutoff.affects_ui = true;      // UI updates needed
        cutoff.send_midi_feedback = true;
        cutoff.automatable = true;
        cutoff.midi_learnable = true;
        
        // Logarithmic scaling for frequency
        cutoff.normalizeValue = [](float hz) {
            return (std::log10(hz) - std::log10(20.0f)) / (std::log10(20000.0f) - std::log10(20.0f));
        };
        
        cutoff.denormalizeValue = [](float norm) {
            float log_min = std::log10(20.0f);
            float log_max = std::log10(20000.0f);
            return std::pow(10.0f, log_min + norm * (log_max - log_min));
        };
        
        cutoff.formatValue = [](float hz) {
            if (hz >= 1000.0f) {
                return std::to_string(static_cast<int>(hz / 100.0f) / 10.0f) + " kHz";
            } else {
                return std::to_string(static_cast<int>(hz)) + " Hz";
            }
        };
        
        registry.registerParameter(cutoff);
    }
    
    // Filter Resonance Parameter
    {
        ParameterInfo resonance;
        resonance.id = ParameterIDs::FILTER_RESONANCE;
        resonance.name = "Filter Resonance";
        resonance.short_name = "Res";
        resonance.min_value = 0.0f;
        resonance.max_value = 10.0f;
        resonance.default_value = 1.0f;
        resonance.affects_audio = true;
        resonance.affects_ui = true;
        resonance.send_midi_feedback = true;
        resonance.automatable = true;
        resonance.midi_learnable = true;
        
        // Linear scaling for resonance
        resonance.formatValue = [](float res) {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << res;
            return ss.str();
        };
        
        registry.registerParameter(resonance);
    }
    
    // Envelope Attack Parameter
    {
        ParameterInfo attack;
        attack.id = ParameterIDs::ENV_ATTACK;
        attack.name = "Envelope Attack";
        attack.short_name = "Attack";
        attack.min_value = 0.001f;    // 1ms
        attack.max_value = 10.0f;     // 10 seconds
        attack.default_value = 0.1f;  // 100ms
        attack.affects_audio = true;
        attack.affects_ui = true;
        attack.send_midi_feedback = true;
        attack.automatable = true;
        attack.midi_learnable = true;
        
        // Logarithmic scaling for time
        attack.normalizeValue = [](float seconds) {
            return (std::log10(seconds) - std::log10(0.001f)) / (std::log10(10.0f) - std::log10(0.001f));
        };
        
        attack.denormalizeValue = [](float norm) {
            float log_min = std::log10(0.001f);
            float log_max = std::log10(10.0f);
            return std::pow(10.0f, log_min + norm * (log_max - log_min));
        };
        
        attack.formatValue = [](float seconds) {
            if (seconds >= 1.0f) {
                return std::to_string(static_cast<int>(seconds * 10.0f) / 10.0f) + " s";
            } else {
                return std::to_string(static_cast<int>(seconds * 1000.0f)) + " ms";
            }
        };
        
        registry.registerParameter(attack);
    }
    
    // Master Volume Parameter
    {
        ParameterInfo volume;
        volume.id = ParameterIDs::MASTER_VOLUME;
        volume.name = "Master Volume";
        volume.short_name = "Volume";
        volume.min_value = 0.0f;      // Silent
        volume.max_value = 1.0f;      // Unity gain
        volume.default_value = 0.7f;  // -3dB
        volume.affects_audio = true;
        volume.affects_ui = true;
        volume.send_midi_feedback = true;
        volume.automatable = true;
        volume.midi_learnable = true;
        
        // Linear scaling but dB display
        volume.formatValue = [](float linear) {
            if (linear <= 0.0f) {
                return std::string("-∞ dB");
            } else {
                float db = 20.0f * std::log10(linear);
                std::ostringstream ss;
                ss << std::fixed << std::setprecision(1) << db << " dB";
                return ss.str();
            }
        };
        
        registry.registerParameter(volume);
    }
    
    // Clock BPM Parameter
    {
        ParameterInfo bpm;
        bpm.id = ParameterIDs::CLOCK_BPM;
        bpm.name = "Clock BPM";
        bpm.short_name = "BPM";
        bpm.min_value = 40.0f;
        bpm.max_value = 200.0f;
        bpm.default_value = 120.0f;
        bpm.affects_audio = false;  // Clock is handled separately
        bpm.affects_ui = true;
        bpm.send_midi_feedback = true;
        bpm.automatable = true;
        bpm.midi_learnable = true;
        
        // Linear scaling for BPM
        bpm.formatValue = [](float bpm_val) {
            return std::to_string(static_cast<int>(bpm_val + 0.5f)) + " BPM";
        };
        
        registry.registerParameter(bpm);
    }
    
    std::cout << "[ParameterRegistry] ⚡ Initialized default synthesizer parameters" << std::endl;
}

} // namespace Parameters
