#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

// Forward declarations
class ParameterObserver;
class CommandManager;

enum class ParameterCategory {
    SYSTEM,
    VOICE,
    OSCILLATORS,
    MIXER,
    FILTERS,
    ENVELOPES,
    LFOS,
    MUTATORS,
    MACROS,
    ARPEGGIATOR,
    EFFECTS,
    AMPLITUDE,
    UNKNOWN
};

/**
 * @brief Core parameter data model - UI-agnostic
 * 
 * Represents a single synthesizer parameter with all its properties.
 * This class is pure data and business logic, with no UI dependencies.
 */
class Parameter {
public:
    Parameter(const std::string& name, 
              const std::string& short_name,
              uint8_t cc_number,
              ParameterCategory category,
              uint8_t min_value = 0,
              uint8_t max_value = 127,
              uint8_t default_value = 64,
              const std::string& description = "");
    
    // Getters
    const std::string& getName() const { return name_; }
    const std::string& getShortName() const { return short_name_; }
    uint8_t getCCNumber() const { return cc_number_; }
    ParameterCategory getCategory() const { return category_; }
    uint8_t getMinValue() const { return min_value_; }
    uint8_t getMaxValue() const { return max_value_; }
    uint8_t getDefaultValue() const { return default_value_; }
    const std::string& getDescription() const { return description_; }
    uint8_t getCurrentValue() const { return current_value_; }
    
    // Value management
    void setValue(uint8_t value);
    void setValueDirect(uint8_t value);  // Bypasses command system (for undo/redo)
    void resetToDefault();
    bool isAtDefault() const;
    
    // Command manager integration
    void setCommandManager(CommandManager* command_manager) { command_manager_ = command_manager; }
    
    // Value scaling utilities
    float getValueAsPercent() const;
    void setValueFromPercent(float percent);
    
    // Bipolar parameter support (-64 to +63 style)
    bool isBipolar() const;
    void setBipolar(bool bipolar);
    int getBipolarValue() const;  // Returns -64 to +63
    void setBipolarValue(int value);  // Accepts -64 to +63
    
    // Observer pattern for UI updates
    void addObserver(std::shared_ptr<ParameterObserver> observer);
    void removeObserver(std::shared_ptr<ParameterObserver> observer);
    
    // Utility functions
    std::string getCategoryName() const;
    std::string getValueDisplayText() const;
    
private:
    std::string name_;
    std::string short_name_;
    uint8_t cc_number_;
    ParameterCategory category_;
    uint8_t min_value_;
    uint8_t max_value_;
    uint8_t default_value_;
    std::string description_;
    uint8_t current_value_;
    bool is_bipolar_;
    CommandManager* command_manager_;  // Injected dependency for undo/redo
    
    std::vector<std::weak_ptr<ParameterObserver>> observers_;
    
    void notifyObservers();
    void cleanupObservers();
};

/**
 * @brief Observer interface for parameter changes
 * 
 * UI controls implement this interface to receive parameter change notifications.
 */
class ParameterObserver {
public:
    virtual ~ParameterObserver() = default;
    virtual void onParameterChanged(const Parameter& parameter) = 0;
};

/**
 * @brief Utility functions for parameter categories
 */
class ParameterUtils {
public:
    static std::string categoryToString(ParameterCategory category);
    static ParameterCategory stringToCategory(const std::string& category_str);
    static std::vector<ParameterCategory> getAllCategories();
};
