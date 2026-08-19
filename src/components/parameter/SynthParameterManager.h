#pragma once

#include "Parameter.h"
#include "ParameterBinder.h"
#include <vector>
#include <map>
#include <memory>
#include <functional>

/**
 * @brief Enhanced synthesizer parameter management system
 * 
 * This class provides a comprehensive interface for loading, searching, and managing
 * synthesizer parameter definitions. It extends the basic ParameterBinder with
 * advanced features like parameter favorites, assignment tracking, and UI integration.
 */
class SynthParameterManager {
public:
    SynthParameterManager();
    ~SynthParameterManager();
    
    // ============================================================================
    // SYNTHESIZER DEFINITION MANAGEMENT
    // ============================================================================
    
    /**
     * @brief Load a synthesizer definition from the config directory
     * @param synth_name Name of the synthesizer (e.g., "hydrasynth", "moog_subsequent")
     * @return true if loaded successfully, false otherwise
     */
    bool loadSynthDefinition(const std::string& synth_name);
    
    /**
     * @brief Load a custom synthesizer definition from file path
     * @param file_path Full path to the JSON definition file
     * @return true if loaded successfully, false otherwise
     */
    bool loadCustomSynthDefinition(const std::string& file_path);
    
    /**
     * @brief Get list of available synthesizer definitions
     * @return Vector of synthesizer names that can be loaded
     */
    std::vector<std::string> getAvailableSynths() const;
    
    /**
     * @brief Get the currently loaded synthesizer name
     * @return Current synthesizer name, or empty string if none loaded
     */
    std::string getCurrentSynthName() const;
    
    /**
     * @brief Get total number of parameters in current synthesizer
     * @return Parameter count, or 0 if no synthesizer loaded
     */
    size_t getParameterCount() const;
    
    // ============================================================================
    // PARAMETER SEARCH AND DISCOVERY
    // ============================================================================
    
    /**
     * @brief Search parameters by text query
     * @param query Search text (searches name, short name, and description)
     * @param category Optional category filter (UNKNOWN = search all categories)
     * @return Vector of matching parameters
     */
    std::vector<std::shared_ptr<Parameter>> searchParameters(
        const std::string& query, 
        ParameterCategory category = ParameterCategory::UNKNOWN) const;
    
    /**
     * @brief Get all parameters in a specific category
     * @param category Parameter category to retrieve
     * @return Vector of parameters in the category
     */
    std::vector<std::shared_ptr<Parameter>> getParametersByCategory(ParameterCategory category) const;
    
    /**
     * @brief Get all available categories in the current synthesizer
     * @return Vector of categories that contain parameters
     */
    std::vector<ParameterCategory> getAvailableCategories() const;
    
    /**
     * @brief Find parameter by exact name match
     * @param name Exact parameter name
     * @return Parameter pointer, or nullptr if not found
     */
    std::shared_ptr<Parameter> findParameterByName(const std::string& name) const;
    
    /**
     * @brief Find parameter by CC number
     * @param cc_number MIDI CC number (0-127)
     * @return Parameter pointer, or nullptr if not found
     */
    std::shared_ptr<Parameter> findParameterByCC(uint8_t cc_number) const;
    
    // ============================================================================
    // PARAMETER FAVORITES SYSTEM
    // ============================================================================
    
    /**
     * @brief Add parameter to favorites list
     * @param parameter_name Name of parameter to favorite
     * @return true if added successfully
     */
    bool addToFavorites(const std::string& parameter_name);
    
    /**
     * @brief Remove parameter from favorites list
     * @param parameter_name Name of parameter to remove
     * @return true if removed successfully
     */
    bool removeFromFavorites(const std::string& parameter_name);
    
    /**
     * @brief Check if parameter is in favorites
     * @param parameter_name Name of parameter to check
     * @return true if parameter is favorited
     */
    bool isFavorite(const std::string& parameter_name) const;
    
    /**
     * @brief Get all favorite parameters
     * @return Vector of favorite parameters
     */
    std::vector<std::shared_ptr<Parameter>> getFavoriteParameters() const;
    
    /**
     * @brief Clear all favorites
     */
    void clearFavorites();
    
    // ============================================================================
    // PARAMETER ASSIGNMENT TRACKING
    // ============================================================================
    
    struct ParameterAssignment {
        std::string parameter_name;
        std::string control_id;      // UI control or automation target
        std::string assignment_type; // "dial", "automation", "macro", etc.
        bool is_active;
        
        ParameterAssignment() : is_active(true) {}
        
        ParameterAssignment(const std::string& param, const std::string& control, 
                          const std::string& type, bool active = true)
            : parameter_name(param), control_id(control), assignment_type(type), is_active(active) {}
    };
    
    /**
     * @brief Assign parameter to a control or automation target
     * @param parameter_name Name of the parameter
     * @param control_id Identifier of the control (e.g., "dial_1", "automation_track_2")
     * @param assignment_type Type of assignment ("dial", "automation", "macro")
     * @return true if assigned successfully
     */
    bool assignParameter(const std::string& parameter_name, 
                        const std::string& control_id, 
                        const std::string& assignment_type);
    
    /**
     * @brief Remove parameter assignment
     * @param control_id Control identifier to unassign
     * @return true if removed successfully
     */
    bool unassignParameter(const std::string& control_id);
    
    /**
     * @brief Get parameter assignment for a control
     * @param control_id Control identifier
     * @return Assignment info, or nullptr if not assigned
     */
    const ParameterAssignment* getAssignment(const std::string& control_id) const;
    
    /**
     * @brief Get all current parameter assignments
     * @return Vector of all assignments
     */
    std::vector<ParameterAssignment> getAllAssignments() const;
    
    /**
     * @brief Check if a parameter is currently assigned to any control
     * @param parameter_name Name of parameter to check
     * @return true if parameter is assigned somewhere
     */
    bool isParameterAssigned(const std::string& parameter_name) const;
    
    // ============================================================================
    // MACRO SYSTEM
    // ============================================================================
    
    struct MacroDefinition {
        std::string name;
        std::string description;
        std::vector<std::string> parameter_names;
        std::vector<float> parameter_weights;  // 0.0 to 1.0 influence
        float min_value;
        float max_value;
        bool is_bipolar;
        
        MacroDefinition() 
            : min_value(0.0f), max_value(1.0f), is_bipolar(false) {}
        
        MacroDefinition(const std::string& macro_name) 
            : name(macro_name), min_value(0.0f), max_value(1.0f), is_bipolar(false) {}
    };
    
    /**
     * @brief Create a new macro that controls multiple parameters
     * @param macro_name Name for the macro
     * @param parameter_names List of parameters to control
     * @param weights Influence weights for each parameter (0.0-1.0)
     * @return true if macro created successfully
     */
    bool createMacro(const std::string& macro_name,
                    const std::vector<std::string>& parameter_names,
                    const std::vector<float>& weights = {});
    
    /**
     * @brief Remove a macro definition
     * @param macro_name Name of macro to remove
     * @return true if removed successfully
     */
    bool removeMacro(const std::string& macro_name);
    
    /**
     * @brief Get all defined macros
     * @return Vector of macro definitions
     */
    std::vector<MacroDefinition> getAllMacros() const;
    
    /**
     * @brief Apply macro value to all its assigned parameters
     * @param macro_name Name of the macro
     * @param value Macro value (typically 0.0-1.0)
     * @param callback Function to call for each parameter change
     */
    void applyMacroValue(const std::string& macro_name, 
                        float value,
                        std::function<void(const std::string&, float)> callback = nullptr);
    
    // ============================================================================
    // PERSISTENCE
    // ============================================================================
    
    /**
     * @brief Save current favorites and assignments to file
     * @param filename File to save to (in user config directory)
     * @return true if saved successfully
     */
    bool saveUserSettings(const std::string& filename = "synth_parameters.json");
    
    /**
     * @brief Load favorites and assignments from file
     * @param filename File to load from (in user config directory)  
     * @return true if loaded successfully
     */
    bool loadUserSettings(const std::string& filename = "synth_parameters.json");
    
    // ============================================================================
    // UI INTEGRATION HELPERS
    // ============================================================================
    
    /**
     * @brief Get parameters formatted for UI display
     * @param category Category to display, UNKNOWN for all
     * @param include_favorites_first If true, favorite parameters appear first
     * @return Vector of parameters ready for UI display
     */
    std::vector<std::shared_ptr<Parameter>> getParametersForUI(
        ParameterCategory category = ParameterCategory::UNKNOWN,
        bool include_favorites_first = false) const;
    
    /**
     * @brief Get category statistics for UI
     * @return Map of category to parameter count
     */
    std::map<ParameterCategory, size_t> getCategoryStatistics() const;
    
    /**
     * @brief Get recently used parameters for quick access
     * @param max_count Maximum number of recent parameters to return
     * @return Vector of recently used parameters
     */
    std::vector<std::shared_ptr<Parameter>> getRecentParameters(size_t max_count = 10) const;
    
    /**
     * @brief Mark parameter as recently used (for recents list)
     * @param parameter_name Name of parameter that was used
     */
    void markParameterAsUsed(const std::string& parameter_name);

private:
    // Core parameter management
    std::unique_ptr<ParameterBinder> parameter_binder_;
    
    // Favorites system
    std::vector<std::string> favorite_parameter_names_;
    
    // Assignment tracking
    std::map<std::string, ParameterAssignment> assignments_;  // control_id -> assignment
    
    // Macro system
    std::map<std::string, MacroDefinition> macros_;  // macro_name -> definition
    
    // Recent usage tracking
    std::vector<std::string> recent_parameter_names_;
    static constexpr size_t MAX_RECENT_PARAMETERS = 20;
    
    // Helper methods
    void addToRecents(const std::string& parameter_name);
    std::string getConfigDirectory() const;
    std::string getUserConfigPath(const std::string& filename) const;
    std::vector<std::string> scanAvailableSynthDefinitions() const;
};

/**
 * @brief Utility class for parameter system operations
 */
class ParameterSystemUtils {
public:
    /**
     * @brief Convert parameter category to display string
     * @param category Parameter category enum
     * @return Human-readable category name
     */
    static std::string categoryToDisplayString(ParameterCategory category);
    
    /**
     * @brief Convert parameter to search keywords
     * @param parameter Parameter to extract keywords from
     * @return Vector of search keywords
     */
    static std::vector<std::string> extractSearchKeywords(const Parameter& parameter);
    
    /**
     * @brief Validate synthesizer definition JSON
     * @param json_content JSON content to validate
     * @return true if valid synthesizer definition
     */
    static bool validateSynthDefinition(const std::string& json_content);
    
    /**
     * @brief Generate parameter statistics for current synth
     * @param manager Parameter manager instance
     * @return Human-readable statistics string
     */
    static std::string generateParameterStatistics(const SynthParameterManager& manager);
};