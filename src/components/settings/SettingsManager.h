#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <any>

/**
 * @brief Global settings manager with observer pattern
 * 
 * This class manages application-wide settings and notifies observers
 * when settings change. Supports various setting types and persistence.
 */
class SettingsManager {
public:
    enum class SettingType {
        BOOLEAN,
        INTEGER,
        FLOAT,
        STRING,
        LIST_SELECTION  // Index-based selection from a list
    };

    struct SettingDefinition {
        std::string key;
        std::string display_name;
        std::string description;
        SettingType type;
        std::any default_value;
        std::any current_value;
        
        // For LIST_SELECTION type
        std::vector<std::string> list_options;
        
        // Validation constraints
        std::any min_value;  // For INTEGER/FLOAT
        std::any max_value;  // For INTEGER/FLOAT
        int max_length = 0;  // For STRING
    };

    using SettingChangedCallback = std::function<void(const std::string& key, const std::any& old_value, const std::any& new_value)>;

    // Singleton pattern for global access
    static SettingsManager& getInstance();

    // Observer pattern
    void addObserver(const std::string& observer_id, SettingChangedCallback callback);
    void removeObserver(const std::string& observer_id);

    // Setting management
    void registerSetting(const SettingDefinition& definition);
    void setValue(const std::string& key, const std::any& value);
    std::any getValue(const std::string& key) const;
    
    // Type-safe getters
    bool getBool(const std::string& key) const;
    int getInt(const std::string& key) const;
    float getFloat(const std::string& key) const;
    std::string getString(const std::string& key) const;
    int getListSelection(const std::string& key) const;

    // Setting info
    const SettingDefinition* getSettingDefinition(const std::string& key) const;
    std::vector<std::string> getAllSettingKeys() const;
    std::vector<SettingDefinition> getAllSettings() const;

    // Persistence
    bool loadFromFile(const std::string& file_path);
    bool saveToFile(const std::string& file_path) const;
    void resetToDefaults();

    // Validation
    bool isValidValue(const std::string& key, const std::any& value) const;

private:
    SettingsManager() = default;
    ~SettingsManager() = default;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    void notifyObservers(const std::string& key, const std::any& old_value, const std::any& new_value);

    std::unordered_map<std::string, SettingDefinition> settings_;
    std::unordered_map<std::string, SettingChangedCallback> observers_;
    
    // Default file paths
    static constexpr const char* DEFAULT_SETTINGS_FILE = "user_settings.json";
    static constexpr const char* DEFAULT_DEFAULTS_FILE = "default_settings.json";
};
