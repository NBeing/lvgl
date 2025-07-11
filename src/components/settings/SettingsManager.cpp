#include "SettingsManager.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

SettingsManager& SettingsManager::getInstance() {
    static SettingsManager instance;
    return instance;
}

void SettingsManager::addObserver(const std::string& observer_id, SettingChangedCallback callback) {
    observers_[observer_id] = callback;
    std::cout << "SettingsManager: Added observer '" << observer_id << "'" << std::endl;
}

void SettingsManager::removeObserver(const std::string& observer_id) {
    auto it = observers_.find(observer_id);
    if (it != observers_.end()) {
        observers_.erase(it);
        std::cout << "SettingsManager: Removed observer '" << observer_id << "'" << std::endl;
    }
}

void SettingsManager::registerSetting(const SettingDefinition& definition) {
    settings_[definition.key] = definition;
    std::cout << "SettingsManager: Registered setting '" << definition.key 
              << "' (" << definition.display_name << ")" << std::endl;
}

void SettingsManager::setValue(const std::string& key, const std::any& value) {
    auto it = settings_.find(key);
    if (it == settings_.end()) {
        std::cout << "SettingsManager: Warning - setting '" << key << "' not found" << std::endl;
        return;
    }

    if (!isValidValue(key, value)) {
        std::cout << "SettingsManager: Invalid value for setting '" << key << "'" << std::endl;
        return;
    }

    std::any old_value = it->second.current_value;
    it->second.current_value = value;

    std::cout << "SettingsManager: Setting '" << key << "' changed" << std::endl;
    notifyObservers(key, old_value, value);
}

std::any SettingsManager::getValue(const std::string& key) const {
    auto it = settings_.find(key);
    if (it != settings_.end()) {
        return it->second.current_value;
    }
    throw std::runtime_error("Setting not found: " + key);
}

bool SettingsManager::getBool(const std::string& key) const {
    try {
        return std::any_cast<bool>(getValue(key));
    } catch (const std::bad_any_cast&) {
        std::cout << "SettingsManager: Type mismatch for boolean setting '" << key << "'" << std::endl;
        return false;
    }
}

int SettingsManager::getInt(const std::string& key) const {
    try {
        return std::any_cast<int>(getValue(key));
    } catch (const std::bad_any_cast&) {
        std::cout << "SettingsManager: Type mismatch for integer setting '" << key << "'" << std::endl;
        return 0;
    }
}

float SettingsManager::getFloat(const std::string& key) const {
    try {
        return std::any_cast<float>(getValue(key));
    } catch (const std::bad_any_cast&) {
        std::cout << "SettingsManager: Type mismatch for float setting '" << key << "'" << std::endl;
        return 0.0f;
    }
}

std::string SettingsManager::getString(const std::string& key) const {
    try {
        return std::any_cast<std::string>(getValue(key));
    } catch (const std::bad_any_cast&) {
        std::cout << "SettingsManager: Type mismatch for string setting '" << key << "'" << std::endl;
        return "";
    }
}

int SettingsManager::getListSelection(const std::string& key) const {
    try {
        return std::any_cast<int>(getValue(key));
    } catch (const std::bad_any_cast&) {
        std::cout << "SettingsManager: Type mismatch for list selection setting '" << key << "'" << std::endl;
        return 0;
    }
}

const SettingsManager::SettingDefinition* SettingsManager::getSettingDefinition(const std::string& key) const {
    auto it = settings_.find(key);
    if (it != settings_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> SettingsManager::getAllSettingKeys() const {
    std::vector<std::string> keys;
    keys.reserve(settings_.size());
    for (const auto& pair : settings_) {
        keys.push_back(pair.first);
    }
    return keys;
}

std::vector<SettingsManager::SettingDefinition> SettingsManager::getAllSettings() const {
    std::vector<SettingDefinition> all_settings;
    all_settings.reserve(settings_.size());
    for (const auto& pair : settings_) {
        all_settings.push_back(pair.second);
    }
    return all_settings;
}

bool SettingsManager::isValidValue(const std::string& key, const std::any& value) const {
    auto it = settings_.find(key);
    if (it == settings_.end()) {
        return false;
    }

    const auto& def = it->second;

    try {
        switch (def.type) {
            case SettingType::BOOLEAN:
                std::any_cast<bool>(value);
                break;
            
            case SettingType::INTEGER: {
                int val = std::any_cast<int>(value);
                if (def.min_value.has_value()) {
                    int min_val = std::any_cast<int>(def.min_value);
                    if (val < min_val) return false;
                }
                if (def.max_value.has_value()) {
                    int max_val = std::any_cast<int>(def.max_value);
                    if (val > max_val) return false;
                }
                break;
            }
            
            case SettingType::FLOAT: {
                float val = std::any_cast<float>(value);
                if (def.min_value.has_value()) {
                    float min_val = std::any_cast<float>(def.min_value);
                    if (val < min_val) return false;
                }
                if (def.max_value.has_value()) {
                    float max_val = std::any_cast<float>(def.max_value);
                    if (val > max_val) return false;
                }
                break;
            }
            
            case SettingType::STRING: {
                std::string val = std::any_cast<std::string>(value);
                if (def.max_length > 0 && val.length() > static_cast<size_t>(def.max_length)) {
                    return false;
                }
                break;
            }
            
            case SettingType::LIST_SELECTION: {
                int val = std::any_cast<int>(value);
                if (val < 0 || val >= static_cast<int>(def.list_options.size())) {
                    return false;
                }
                break;
            }
        }
        return true;
    } catch (const std::bad_any_cast&) {
        return false;
    }
}

void SettingsManager::resetToDefaults() {
    std::cout << "SettingsManager: Resetting all settings to defaults..." << std::endl;
    for (auto& pair : settings_) {
        std::any old_value = pair.second.current_value;
        pair.second.current_value = pair.second.default_value;
        notifyObservers(pair.first, old_value, pair.second.current_value);
    }
}

void SettingsManager::notifyObservers(const std::string& key, const std::any& old_value, const std::any& new_value) {
    for (const auto& observer : observers_) {
        try {
            observer.second(key, old_value, new_value);
        } catch (const std::exception& e) {
            std::cout << "SettingsManager: Error in observer '" << observer.first 
                      << "': " << e.what() << std::endl;
        }
    }
}

// Placeholder implementations for file I/O (we'll implement JSON later)
bool SettingsManager::loadFromFile(const std::string& file_path) {
    std::cout << "SettingsManager: Loading from file '" << file_path << "' (not implemented yet)" << std::endl;
    // TODO: Implement JSON loading
    return false;
}

bool SettingsManager::saveToFile(const std::string& file_path) const {
    std::cout << "SettingsManager: Saving to file '" << file_path << "' (not implemented yet)" << std::endl;
    // TODO: Implement JSON saving
    return false;
}
