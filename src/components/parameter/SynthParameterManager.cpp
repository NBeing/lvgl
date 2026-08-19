#include "SynthParameterManager.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <sstream>

// Mock JSON parsing for now (replace with actual JSON library)
namespace {
    std::vector<std::string> scanSynthDefinitions(const std::string& directory) {
        std::vector<std::string> synths;
        try {
            if (std::filesystem::exists(directory)) {
                for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                    if (entry.path().extension() == ".json") {
                        std::string name = entry.path().stem().string();
                        synths.push_back(name);
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[SynthParameterManager] Error scanning directory: " << e.what() << std::endl;
        }
        return synths;
    }
}

// ============================================================================
// SynthParameterManager Implementation
// ============================================================================

SynthParameterManager::SynthParameterManager() {
    parameter_binder_ = std::make_unique<ParameterBinder>();
    std::cout << "[SynthParameterManager] ⚡ Initialized enhanced parameter system" << std::endl;
}

SynthParameterManager::~SynthParameterManager() = default;

// ============================================================================
// SYNTHESIZER DEFINITION MANAGEMENT
// ============================================================================

bool SynthParameterManager::loadSynthDefinition(const std::string& synth_name) {
    std::cout << "[SynthParameterManager] 🎹 Loading synthesizer: " << synth_name << std::endl;
    
    bool success = parameter_binder_->loadSynthDefinition(synth_name);
    if (success) {
        std::cout << "[SynthParameterManager] ✅ Loaded " << getParameterCount() 
                  << " parameters for " << getCurrentSynthName() << std::endl;
                  
        // Load user settings for this synthesizer
        loadUserSettings(synth_name + "_settings.json");
    } else {
        std::cerr << "[SynthParameterManager] ❌ Failed to load synthesizer: " << synth_name << std::endl;
    }
    
    return success;
}

bool SynthParameterManager::loadCustomSynthDefinition(const std::string& file_path) {
    std::cout << "[SynthParameterManager] 📁 Loading custom definition: " << file_path << std::endl;
    
    bool success = parameter_binder_->loadSynthDefinitionFromFile(file_path);
    if (success) {
        std::cout << "[SynthParameterManager] ✅ Loaded custom synthesizer: " 
                  << getCurrentSynthName() << std::endl;
    }
    
    return success;
}

std::vector<std::string> SynthParameterManager::getAvailableSynths() const {
    // Combine built-in and custom synthesizers
    auto builtin_synths = parameter_binder_->getAvailableSynths();
    auto custom_synths = scanAvailableSynthDefinitions();
    
    // Merge and deduplicate
    std::vector<std::string> all_synths = builtin_synths;
    for (const auto& custom : custom_synths) {
        if (std::find(all_synths.begin(), all_synths.end(), custom) == all_synths.end()) {
            all_synths.push_back(custom);
        }
    }
    
    return all_synths;
}

std::string SynthParameterManager::getCurrentSynthName() const {
    return parameter_binder_->getCurrentSynthName();
}

size_t SynthParameterManager::getParameterCount() const {
    return parameter_binder_->getParameterCount();
}

// ============================================================================
// PARAMETER SEARCH AND DISCOVERY
// ============================================================================

std::vector<std::shared_ptr<Parameter>> SynthParameterManager::searchParameters(
    const std::string& query, ParameterCategory category) const {
    
    if (query.empty()) {
        return getParametersByCategory(category);
    }
    
    auto search_results = parameter_binder_->searchParameters(query);
    
    // Filter by category if specified
    if (category != ParameterCategory::UNKNOWN) {
        search_results.erase(
            std::remove_if(search_results.begin(), search_results.end(),
                [category](const auto& param) { return param->getCategory() != category; }),
            search_results.end()
        );
    }
    
    std::cout << "[SynthParameterManager] 🔍 Search '" << query << "' found " 
              << search_results.size() << " parameters" << std::endl;
    
    return search_results;
}

std::vector<std::shared_ptr<Parameter>> SynthParameterManager::getParametersByCategory(ParameterCategory category) const {
    if (category == ParameterCategory::UNKNOWN) {
        return parameter_binder_->getAllParameters();
    }
    return parameter_binder_->getParametersByCategory(category);
}

std::vector<ParameterCategory> SynthParameterManager::getAvailableCategories() const {
    return parameter_binder_->getAvailableCategories();
}

std::shared_ptr<Parameter> SynthParameterManager::findParameterByName(const std::string& name) const {
    auto param = parameter_binder_->findParameterByName(name);
    if (param) {
        // Mark as recently used
        const_cast<SynthParameterManager*>(this)->markParameterAsUsed(name);
    }
    return param;
}

std::shared_ptr<Parameter> SynthParameterManager::findParameterByCC(uint8_t cc_number) const {
    auto param = parameter_binder_->findParameterByCC(cc_number);
    if (param) {
        const_cast<SynthParameterManager*>(this)->markParameterAsUsed(param->getName());
    }
    return param;
}

// ============================================================================
// PARAMETER FAVORITES SYSTEM
// ============================================================================

bool SynthParameterManager::addToFavorites(const std::string& parameter_name) {
    if (isFavorite(parameter_name)) {
        return false; // Already in favorites
    }
    
    // Verify parameter exists
    if (!findParameterByName(parameter_name)) {
        std::cerr << "[SynthParameterManager] ❌ Cannot favorite unknown parameter: " << parameter_name << std::endl;
        return false;
    }
    
    favorite_parameter_names_.push_back(parameter_name);
    std::cout << "[SynthParameterManager] ⭐ Added to favorites: " << parameter_name << std::endl;
    return true;
}

bool SynthParameterManager::removeFromFavorites(const std::string& parameter_name) {
    auto it = std::find(favorite_parameter_names_.begin(), favorite_parameter_names_.end(), parameter_name);
    if (it != favorite_parameter_names_.end()) {
        favorite_parameter_names_.erase(it);
        std::cout << "[SynthParameterManager] ✨ Removed from favorites: " << parameter_name << std::endl;
        return true;
    }
    return false;
}

bool SynthParameterManager::isFavorite(const std::string& parameter_name) const {
    return std::find(favorite_parameter_names_.begin(), favorite_parameter_names_.end(), parameter_name) 
           != favorite_parameter_names_.end();
}

std::vector<std::shared_ptr<Parameter>> SynthParameterManager::getFavoriteParameters() const {
    std::vector<std::shared_ptr<Parameter>> favorites;
    for (const auto& name : favorite_parameter_names_) {
        auto param = parameter_binder_->findParameterByName(name);
        if (param) {
            favorites.push_back(param);
        }
    }
    return favorites;
}

void SynthParameterManager::clearFavorites() {
    favorite_parameter_names_.clear();
    std::cout << "[SynthParameterManager] 🧹 Cleared all favorites" << std::endl;
}

// ============================================================================
// PARAMETER ASSIGNMENT TRACKING
// ============================================================================

bool SynthParameterManager::assignParameter(const std::string& parameter_name, 
                                           const std::string& control_id, 
                                           const std::string& assignment_type) {
    // Verify parameter exists
    if (!findParameterByName(parameter_name)) {
        std::cerr << "[SynthParameterManager] ❌ Cannot assign unknown parameter: " << parameter_name << std::endl;
        return false;
    }
    
    // Remove any existing assignment for this control
    unassignParameter(control_id);
    
    // Create new assignment
    assignments_[control_id] = ParameterAssignment(parameter_name, control_id, assignment_type);
    
    std::cout << "[SynthParameterManager] 🔗 Assigned " << parameter_name 
              << " to " << control_id << " (" << assignment_type << ")" << std::endl;
    
    return true;
}

bool SynthParameterManager::unassignParameter(const std::string& control_id) {
    auto it = assignments_.find(control_id);
    if (it != assignments_.end()) {
        std::cout << "[SynthParameterManager] 🔓 Unassigned " << it->second.parameter_name 
                  << " from " << control_id << std::endl;
        assignments_.erase(it);
        return true;
    }
    return false;
}

const SynthParameterManager::ParameterAssignment* SynthParameterManager::getAssignment(const std::string& control_id) const {
    auto it = assignments_.find(control_id);
    return (it != assignments_.end()) ? &it->second : nullptr;
}

std::vector<SynthParameterManager::ParameterAssignment> SynthParameterManager::getAllAssignments() const {
    std::vector<ParameterAssignment> all_assignments;
    for (const auto& pair : assignments_) {
        all_assignments.push_back(pair.second);
    }
    return all_assignments;
}

bool SynthParameterManager::isParameterAssigned(const std::string& parameter_name) const {
    for (const auto& pair : assignments_) {
        if (pair.second.parameter_name == parameter_name) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// MACRO SYSTEM
// ============================================================================

bool SynthParameterManager::createMacro(const std::string& macro_name,
                                       const std::vector<std::string>& parameter_names,
                                       const std::vector<float>& weights) {
    // Verify all parameters exist
    for (const auto& param_name : parameter_names) {
        if (!findParameterByName(param_name)) {
            std::cerr << "[SynthParameterManager] ❌ Cannot create macro with unknown parameter: " << param_name << std::endl;
            return false;
        }
    }
    
    MacroDefinition macro(macro_name);
    macro.parameter_names = parameter_names;
    
    // Use provided weights or default to equal weights
    if (weights.empty()) {
        macro.parameter_weights.assign(parameter_names.size(), 1.0f);
    } else {
        macro.parameter_weights = weights;
        // Ensure weights vector matches parameter count
        macro.parameter_weights.resize(parameter_names.size(), 1.0f);
    }
    
    macros_[macro_name] = macro;
    
    std::cout << "[SynthParameterManager] 🎛️ Created macro '" << macro_name 
              << "' with " << parameter_names.size() << " parameters" << std::endl;
    
    return true;
}

bool SynthParameterManager::removeMacro(const std::string& macro_name) {
    auto it = macros_.find(macro_name);
    if (it != macros_.end()) {
        std::cout << "[SynthParameterManager] 🗑️ Removed macro: " << macro_name << std::endl;
        macros_.erase(it);
        return true;
    }
    return false;
}

std::vector<SynthParameterManager::MacroDefinition> SynthParameterManager::getAllMacros() const {
    std::vector<MacroDefinition> all_macros;
    for (const auto& pair : macros_) {
        all_macros.push_back(pair.second);
    }
    return all_macros;
}

void SynthParameterManager::applyMacroValue(const std::string& macro_name, 
                                          float value,
                                          std::function<void(const std::string&, float)> callback) {
    auto it = macros_.find(macro_name);
    if (it == macros_.end()) {
        std::cerr << "[SynthParameterManager] ❌ Unknown macro: " << macro_name << std::endl;
        return;
    }
    
    const auto& macro = it->second;
    
    // Apply value to each parameter with its weight
    for (size_t i = 0; i < macro.parameter_names.size(); ++i) {
        float weighted_value = value * macro.parameter_weights[i];
        
        // Clamp to parameter range
        weighted_value = std::max(macro.min_value, std::min(macro.max_value, weighted_value));
        
        if (callback) {
            callback(macro.parameter_names[i], weighted_value);
        }
    }
}

// ============================================================================
// PERSISTENCE
// ============================================================================

bool SynthParameterManager::saveUserSettings(const std::string& filename) {
    try {
        std::string filepath = getUserConfigPath(filename);
        std::ofstream file(filepath);
        
        if (!file.is_open()) {
            std::cerr << "[SynthParameterManager] ❌ Cannot save to: " << filepath << std::endl;
            return false;
        }
        
        // Simple JSON-like format (replace with proper JSON library)
        file << "{\n";
        file << "  \"synthesizer\": \"" << getCurrentSynthName() << "\",\n";
        
        // Save favorites
        file << "  \"favorites\": [\n";
        for (size_t i = 0; i < favorite_parameter_names_.size(); ++i) {
            file << "    \"" << favorite_parameter_names_[i] << "\"";
            if (i < favorite_parameter_names_.size() - 1) file << ",";
            file << "\n";
        }
        file << "  ],\n";
        
        // Save assignments
        file << "  \"assignments\": [\n";
        auto all_assignments = getAllAssignments();
        for (size_t i = 0; i < all_assignments.size(); ++i) {
            const auto& assignment = all_assignments[i];
            file << "    {\n";
            file << "      \"parameter\": \"" << assignment.parameter_name << "\",\n";
            file << "      \"control\": \"" << assignment.control_id << "\",\n";
            file << "      \"type\": \"" << assignment.assignment_type << "\"\n";
            file << "    }";
            if (i < all_assignments.size() - 1) file << ",";
            file << "\n";
        }
        file << "  ]\n";
        file << "}\n";
        
        std::cout << "[SynthParameterManager] 💾 Saved user settings to: " << filepath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[SynthParameterManager] ❌ Save error: " << e.what() << std::endl;
        return false;
    }
}

bool SynthParameterManager::loadUserSettings(const std::string& filename) {
    try {
        std::string filepath = getUserConfigPath(filename);
        
        if (!std::filesystem::exists(filepath)) {
            std::cout << "[SynthParameterManager] ℹ️ No user settings file found: " << filepath << std::endl;
            return true; // Not an error, just no settings to load
        }
        
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[SynthParameterManager] ❌ Cannot load from: " << filepath << std::endl;
            return false;
        }
        
        // Simple parsing (replace with proper JSON library)
        std::string line;
        bool in_favorites = false;
        bool in_assignments = false;
        
        while (std::getline(file, line)) {
            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            
            if (line.find("\"favorites\"") != std::string::npos) {
                in_favorites = true;
                continue;
            } else if (line.find("\"assignments\"") != std::string::npos) {
                in_favorites = false;
                in_assignments = true;
                continue;
            }
            
            if (in_favorites && line.find("\"") != std::string::npos && line.find("]") == std::string::npos) {
                // Extract parameter name from quoted string
                size_t start = line.find("\"") + 1;
                size_t end = line.find("\"", start);
                if (end != std::string::npos) {
                    std::string param_name = line.substr(start, end - start);
                    addToFavorites(param_name);
                }
            }
        }
        
        std::cout << "[SynthParameterManager] 📂 Loaded user settings from: " << filepath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[SynthParameterManager] ❌ Load error: " << e.what() << std::endl;
        return false;
    }
}

// ============================================================================
// UI INTEGRATION HELPERS
// ============================================================================

std::vector<std::shared_ptr<Parameter>> SynthParameterManager::getParametersForUI(
    ParameterCategory category, bool include_favorites_first) const {
    
    auto parameters = getParametersByCategory(category);
    
    if (include_favorites_first) {
        auto favorites = getFavoriteParameters();
        
        // Remove favorites from main list to avoid duplicates
        if (category == ParameterCategory::UNKNOWN) {
            parameters.erase(
                std::remove_if(parameters.begin(), parameters.end(),
                    [this](const auto& param) { return isFavorite(param->getName()); }),
                parameters.end()
            );
        }
        
        // Prepend favorites
        favorites.insert(favorites.end(), parameters.begin(), parameters.end());
        return favorites;
    }
    
    return parameters;
}

std::map<ParameterCategory, size_t> SynthParameterManager::getCategoryStatistics() const {
    std::map<ParameterCategory, size_t> stats;
    
    for (auto category : getAvailableCategories()) {
        stats[category] = getParametersByCategory(category).size();
    }
    
    return stats;
}

std::vector<std::shared_ptr<Parameter>> SynthParameterManager::getRecentParameters(size_t max_count) const {
    std::vector<std::shared_ptr<Parameter>> recent_params;
    
    size_t count = std::min(max_count, recent_parameter_names_.size());
    for (size_t i = 0; i < count; ++i) {
        auto param = parameter_binder_->findParameterByName(recent_parameter_names_[i]);
        if (param) {
            recent_params.push_back(param);
        }
    }
    
    return recent_params;
}

void SynthParameterManager::markParameterAsUsed(const std::string& parameter_name) {
    addToRecents(parameter_name);
}

// ============================================================================
// PRIVATE HELPER METHODS
// ============================================================================

void SynthParameterManager::addToRecents(const std::string& parameter_name) {
    // Remove if already in list
    auto it = std::find(recent_parameter_names_.begin(), recent_parameter_names_.end(), parameter_name);
    if (it != recent_parameter_names_.end()) {
        recent_parameter_names_.erase(it);
    }
    
    // Add to front
    recent_parameter_names_.insert(recent_parameter_names_.begin(), parameter_name);
    
    // Limit size
    if (recent_parameter_names_.size() > MAX_RECENT_PARAMETERS) {
        recent_parameter_names_.resize(MAX_RECENT_PARAMETERS);
    }
}

std::string SynthParameterManager::getConfigDirectory() const {
    return "src/config/synthesizers";
}

std::string SynthParameterManager::getUserConfigPath(const std::string& filename) const {
    return "user_config/" + filename;
}

std::vector<std::string> SynthParameterManager::scanAvailableSynthDefinitions() const {
    return scanSynthDefinitions(getConfigDirectory());
}

// ============================================================================
// ParameterSystemUtils Implementation
// ============================================================================

std::string ParameterSystemUtils::categoryToDisplayString(ParameterCategory category) {
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
        default: return "Other";
    }
}

std::vector<std::string> ParameterSystemUtils::extractSearchKeywords(const Parameter& parameter) {
    std::vector<std::string> keywords;
    
    // Add parameter name words
    std::stringstream ss(parameter.getName());
    std::string word;
    while (ss >> word) {
        keywords.push_back(word);
    }
    
    // Add short name
    keywords.push_back(parameter.getShortName());
    
    // Add category
    keywords.push_back(categoryToDisplayString(parameter.getCategory()));
    
    return keywords;
}

bool ParameterSystemUtils::validateSynthDefinition(const std::string& json_content) {
    // Basic validation - check for required fields
    return json_content.find("\"name\"") != std::string::npos &&
           json_content.find("\"categories\"") != std::string::npos;
}

std::string ParameterSystemUtils::generateParameterStatistics(const SynthParameterManager& manager) {
    std::stringstream stats;
    
    stats << "📊 Parameter Statistics for " << manager.getCurrentSynthName() << "\n";
    stats << "Total Parameters: " << manager.getParameterCount() << "\n";
    stats << "Favorites: " << manager.getFavoriteParameters().size() << "\n";
    stats << "Assignments: " << manager.getAllAssignments().size() << "\n";
    stats << "Macros: " << manager.getAllMacros().size() << "\n\n";
    
    auto category_stats = manager.getCategoryStatistics();
    stats << "By Category:\n";
    for (const auto& pair : category_stats) {
        stats << "- " << ParameterSystemUtils::categoryToDisplayString(pair.first) 
              << ": " << pair.second << "\n";
    }
    
    return stats.str();
}