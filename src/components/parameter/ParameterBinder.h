#pragma once

#include "Parameter.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

/**
 * @brief Manages synthesizer parameter definitions and control bindings
 * 
 * This class handles loading synthesizer definitions from JSON files,
 * managing parameter databases, and providing parameter lookup functionality.
 */
class ParameterBinder {
public:
    ParameterBinder();
    ~ParameterBinder();
    
    // Synthesizer definition management
    bool loadSynthDefinition(const std::string& synth_name);
    bool loadSynthDefinitionFromFile(const std::string& file_path);
    std::vector<std::string> getAvailableSynths() const;
    std::string getCurrentSynthName() const;
    
    // Parameter lookup
    std::shared_ptr<Parameter> findParameterByName(const std::string& name) const;
    std::shared_ptr<Parameter> findParameterByCC(uint8_t cc_number) const;
    std::vector<std::shared_ptr<Parameter>> getParametersByCategory(ParameterCategory category) const;
    std::vector<std::shared_ptr<Parameter>> searchParameters(const std::string& query) const;
    std::vector<std::shared_ptr<Parameter>> getAllParameters() const;
    
    // Parameter creation (for custom definitions)
    std::shared_ptr<Parameter> createParameter(
        const std::string& name,
        const std::string& short_name,
        uint8_t cc_number,
        ParameterCategory category,
        uint8_t min_value = 0,
        uint8_t max_value = 127,
        uint8_t default_value = 64,
        const std::string& description = ""
    );
    
    // Control binding management
    using ControlId = std::string;
    void bindControlToParameter(const ControlId& control_id, const std::string& parameter_name);
    void bindControlToParameter(const ControlId& control_id, std::shared_ptr<Parameter> parameter);
    void unbindControl(const ControlId& control_id);
    std::shared_ptr<Parameter> getParameterForControl(const ControlId& control_id) const;
    std::vector<ControlId> getControlsForParameter(const std::string& parameter_name) const;
    
    // Utility functions
    std::vector<ParameterCategory> getAvailableCategories() const;
    size_t getParameterCount() const;
    size_t getParameterCountByCategory(ParameterCategory category) const;
    
private:
    struct SynthDefinition {
        std::string name;
        std::string manufacturer;
        std::string description;
        std::vector<std::shared_ptr<Parameter>> parameters;
        std::map<std::string, std::shared_ptr<Parameter>> parameter_by_name;
        std::map<uint8_t, std::shared_ptr<Parameter>> parameter_by_cc;
        std::map<ParameterCategory, std::vector<std::shared_ptr<Parameter>>> parameters_by_category;
    };
    
    std::unique_ptr<SynthDefinition> current_synth_;
    std::map<ControlId, std::shared_ptr<Parameter>> control_bindings_;
    
    // Helper methods
    void buildParameterMaps(SynthDefinition& synth_def);
    bool parseJsonDefinition(const std::string& json_content, SynthDefinition& synth_def);
    std::shared_ptr<Parameter> createParameterFromJson(const std::string& param_name, 
                                                      const std::string& json_content);
};

/**
 * @brief Factory class for creating common parameter sets
 * 
 * Provides convenient methods for creating standard parameter configurations
 * for common synthesizer types.
 */
class ParameterFactory {
public:
    // Standard parameter sets
    static std::vector<std::shared_ptr<Parameter>> createBasicSynthParameters();
    static std::vector<std::shared_ptr<Parameter>> createFilterParameters();
    static std::vector<std::shared_ptr<Parameter>> createEnvelopeParameters();
    static std::vector<std::shared_ptr<Parameter>> createLFOParameters();
    
    // Individual parameter creators
    static std::shared_ptr<Parameter> createFilterCutoff(uint8_t cc = 74);
    static std::shared_ptr<Parameter> createFilterResonance(uint8_t cc = 71);
    static std::shared_ptr<Parameter> createAmpAttack(uint8_t cc = 73);
    static std::shared_ptr<Parameter> createAmpRelease(uint8_t cc = 72);
    static std::shared_ptr<Parameter> createLFORate(uint8_t cc = 76);
    static std::shared_ptr<Parameter> createLFOAmount(uint8_t cc = 77);
    
    // Hydrasynth-specific parameters
    static std::vector<std::shared_ptr<Parameter>> createHydrasynthParameters();
};
