#pragma once

#include "ParameterChangeEvent.h"
#include "components/memory/PSRAMManager.h"
#include <string>
#include <unordered_map>
#include <functional>

namespace Parameters {

/**
 * @brief Parameter characteristics and metadata
 * 
 * Defines how a parameter behaves, where it should be processed,
 * and what outputs it affects.
 */
struct ParameterInfo {
    ParameterID id;
    std::string name;                   // Human-readable name
    std::string short_name;             // Short display name
    float min_value;                    // Minimum real value
    float max_value;                    // Maximum real value
    float default_value;                // Default real value
    bool affects_audio;                 // Needs RT thread processing
    bool affects_ui;                    // Needs UI thread updates
    bool send_midi_feedback;            // Send MIDI echo
    bool automatable;                   // Can be automated
    bool midi_learnable;                // Can be MIDI learned
    
    // Value conversion functions
    std::function<float(float)> normalizeValue;    // Real → 0.0-1.0
    std::function<float(float)> denormalizeValue;  // 0.0-1.0 → Real
    std::function<std::string(float)> formatValue; // Real → Display string
    
    ParameterInfo() 
        : id(0), min_value(0.0f), max_value(1.0f), default_value(0.0f)
        , affects_audio(false), affects_ui(true), send_midi_feedback(false)
        , automatable(true), midi_learnable(true) {
        
        // Don't set default lambdas in constructor - they will be set properly later
        // This avoids the stack-use-after-return issue
    }
    
    // Method to set up default linear scaling after min/max values are set
    void setupDefaultScaling() {
        normalizeValue = [min = min_value, max = max_value](float real_val) {
            if (max == min) return 0.5f; // Avoid division by zero
            return (real_val - min) / (max - min);
        };
        
        denormalizeValue = [min = min_value, max = max_value](float norm_val) {
            return min + norm_val * (max - min);
        };
        
        if (!formatValue) { // Only set if not already set
            formatValue = [](float real_val) {
                return std::to_string(real_val);
            };
        }
    }
};

/**
 * @brief Central registry of all parameters in the system
 * 
 * Single source of truth for parameter metadata and behavior.
 * Thread-safe for read operations after initialization.
 */
class ParameterRegistry {
public:
    static ParameterRegistry& getInstance();
    
    // Parameter registration (called during initialization)
    void registerParameter(const ParameterInfo& info);
    
    // Parameter lookup (thread-safe after init)
    const ParameterInfo* getParameterInfo(ParameterID id) const;
    bool hasParameter(ParameterID id) const;
    
    // Parameter enumeration
    std::vector<ParameterID> getAllParameterIDs() const;
    std::vector<ParameterID> getAudioParameters() const;
    std::vector<ParameterID> getUIParameters() const;
    std::vector<ParameterID> getMidiLearnableParameters() const;
    
    // Value conversion helpers
    float normalizeValue(ParameterID id, float real_value) const;
    float denormalizeValue(ParameterID id, float normalized_value) const;
    std::string formatValue(ParameterID id, float real_value) const;
    
private:
    ParameterRegistry() = default;
    std::unordered_map<ParameterID, ParameterInfo> parameters_;
};

// Common parameter IDs (to be expanded)
namespace ParameterIDs {
    // Filter parameters
    constexpr ParameterID FILTER_CUTOFF = 1001;
    constexpr ParameterID FILTER_RESONANCE = 1002;
    constexpr ParameterID FILTER_TYPE = 1003;
    
    // Envelope parameters
    constexpr ParameterID ENV_ATTACK = 2001;
    constexpr ParameterID ENV_DECAY = 2002;
    constexpr ParameterID ENV_SUSTAIN = 2003;
    constexpr ParameterID ENV_RELEASE = 2004;
    
    // Oscillator parameters
    constexpr ParameterID OSC_PITCH = 3001;
    constexpr ParameterID OSC_DETUNE = 3002;
    constexpr ParameterID OSC_WAVEFORM = 3003;
    
    // Master parameters
    constexpr ParameterID MASTER_VOLUME = 4001;
    constexpr ParameterID MASTER_PAN = 4002;
    
    // Clock parameters
    constexpr ParameterID CLOCK_BPM = 5001;
    constexpr ParameterID CLOCK_SWING = 5002;
}

// Parameter initialization
void initializeDefaultParameters();

} // namespace Parameters
