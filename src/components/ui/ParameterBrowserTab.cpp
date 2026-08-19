#include "ParameterBrowserTab.h"
#include "components/parameter/ParameterManager.h"
#include "components/layout/LayoutManager.h"
#include <iostream>
#include <sstream>

// ============================================================================
// ParameterBrowserTab Implementation
// ============================================================================

ParameterBrowserTab::ParameterBrowserTab()
    : Tab("Parameters")
    , main_container_(nullptr)
    , header_panel_(nullptr)
    , status_label_(nullptr)
    , quick_stats_label_(nullptr)
    , core_param_manager_(nullptr)
{
    std::cout << "[ParameterBrowserTab] 🎛️ Initializing Parameter Browser Tab" << std::endl;
}

void ParameterBrowserTab::create(lv_obj_t* parent) {
    std::cout << "[ParameterBrowserTab] 🔧 Creating Parameter Browser Tab UI" << std::endl;
    
    // Initialize the parameter management system
    initializeManagers();
    
    // Create the main UI
    setupUI(parent);
    
    // Load default synthesizer
    loadDefaultSynthesizer();
    
    std::cout << "[ParameterBrowserTab] ✅ Parameter Browser Tab created successfully" << std::endl;
}

void ParameterBrowserTab::onActivated() {
    std::cout << "[ParameterBrowserTab] 🎯 Parameter Browser Tab activated" << std::endl;
    
    if (param_selection_ui_) {
        param_selection_ui_->onActivated();
    }
    
    updateStatusDisplay();
    updateQuickStats();
}

void ParameterBrowserTab::onDeactivated() {
    std::cout << "[ParameterBrowserTab] 💤 Parameter Browser Tab deactivated" << std::endl;
    
    if (param_selection_ui_) {
        param_selection_ui_->onDeactivated();
    }
}

void ParameterBrowserTab::update() {
    if (param_selection_ui_) {
        param_selection_ui_->update();
    }
    
    // Update status periodically
    static uint32_t last_status_update = 0;
    uint32_t now = lv_tick_get();
    if (now - last_status_update > 5000) { // Update every 5 seconds
        updateQuickStats();
        last_status_update = now;
    }
}

// ============================================================================
// Private Implementation
// ============================================================================

void ParameterBrowserTab::initializeManagers() {
    std::cout << "[ParameterBrowserTab] 🔧 Initializing parameter managers" << std::endl;
    
    try {
        // Create enhanced synthesizer parameter manager
        synth_param_manager_ = std::make_unique<SynthParameterManager>();
        
        // Get reference to core parameter manager (singleton)
        core_param_manager_ = &Parameters::ParameterManager::getInstance();
        
        // Create integration bridge
        param_bridge_ = std::make_unique<ParameterSystemBridge>(
            synth_param_manager_.get(),
            core_param_manager_
        );
        
        std::cout << "[ParameterBrowserTab] ✅ Parameter managers initialized" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[ParameterBrowserTab] ❌ Failed to initialize managers: " << e.what() << std::endl;
    }
}

void ParameterBrowserTab::setupUI(lv_obj_t* parent) {
    // Create main container
    main_container_ = lv_obj_create(parent);
    lv_obj_set_size(main_container_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(main_container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(main_container_, 4, 0);
    
    // Create header panel
    createHeaderPanel(main_container_);
    
    // Create parameter selection UI
    try {
        param_selection_ui_ = std::make_unique<ParameterSelectionUI>(
            synth_param_manager_.get(),
            core_param_manager_
        );
        
        // Set up parameter selection callback
        param_selection_ui_->setSelectionCallback(
            [this](std::shared_ptr<Parameter> param, const std::string& action) {
                handleParameterSelection(param, action);
            }
        );
        
        // Create the parameter selection UI within our container
        param_selection_ui_->create(main_container_);
        
    } catch (const std::exception& e) {
        std::cerr << "[ParameterBrowserTab] ❌ Failed to create parameter selection UI: " << e.what() << std::endl;
        
        // Create fallback error message
        lv_obj_t* error_label = lv_label_create(main_container_);
        lv_label_set_text(error_label, "Failed to initialize parameter browser");
        lv_obj_set_style_text_color(error_label, lv_color_hex(0xFF0000), 0);
    }
}

void ParameterBrowserTab::createHeaderPanel(lv_obj_t* parent) {
    // Header panel with status and quick stats
    header_panel_ = lv_obj_create(parent);
    lv_obj_set_size(header_panel_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(header_panel_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header_panel_, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(header_panel_, 8, 0);
    lv_obj_set_style_bg_color(header_panel_, lv_color_hex(0x2E3440), 0);
    lv_obj_set_style_bg_opa(header_panel_, 150, 0);
    
    // Status label (left side)
    status_label_ = lv_label_create(header_panel_);
    lv_label_set_text(status_label_, "🎛️ Parameter Browser");
    lv_obj_set_style_text_font(status_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status_label_, lv_color_hex(0xD8DEE9), 0);
    
    // Spacer
    lv_obj_t* spacer = lv_obj_create(header_panel_);
    lv_obj_set_size(spacer, LV_PCT(100), 1);
    lv_obj_set_style_bg_opa(spacer, 0, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    
    // Quick stats label (right side)
    quick_stats_label_ = lv_label_create(header_panel_);
    lv_label_set_text(quick_stats_label_, "Initializing...");
    lv_obj_set_style_text_font(quick_stats_label_, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(quick_stats_label_, lv_color_hex(0x88C0D0), 0);
}

void ParameterBrowserTab::handleParameterSelection(std::shared_ptr<Parameter> parameter, 
                                                  const std::string& action) {
    if (!parameter) {
        showStatusMessage("No parameter selected", false);
        return;
    }
    
    std::cout << "[ParameterBrowserTab] 🎯 Parameter action: " << parameter->getName() 
              << " → " << action << std::endl;
    
    try {
        if (action == "assign") {
            showAssignmentDialog(parameter);
            
        } else if (action == "favorite") {
            // Toggle favorite status
            bool is_favorite = synth_param_manager_->isFavorite(parameter->getName());
            if (is_favorite) {
                synth_param_manager_->removeFromFavorites(parameter->getName());
                showStatusMessage("Removed from favorites: " + parameter->getName());
            } else {
                synth_param_manager_->addToFavorites(parameter->getName());
                showStatusMessage("Added to favorites: " + parameter->getName());
            }
            
            // Refresh UI to update favorite indicators
            if (param_selection_ui_) {
                param_selection_ui_->refreshParameterList();
            }
            
        } else if (action == "macro") {
            showMacroCreationDialog(parameter);
            
        } else {
            showStatusMessage("Unknown action: " + action, false);
        }
        
        updateQuickStats();
        
    } catch (const std::exception& e) {
        std::cerr << "[ParameterBrowserTab] ❌ Error handling parameter selection: " << e.what() << std::endl;
        showStatusMessage("Error: " + std::string(e.what()), false);
    }
}

void ParameterBrowserTab::showAssignmentDialog(std::shared_ptr<Parameter> parameter) {
    std::cout << "[ParameterBrowserTab] 🔗 Showing assignment dialog for: " << parameter->getName() << std::endl;
    
    // For now, create a simple assignment to MIDI learn
    // In a full implementation, this would show a dialog with assignment options
    
    try {
        bool success = param_bridge_->startMidiLearn(parameter);
        if (success) {
            showStatusMessage("MIDI Learn started for " + parameter->getName());
            std::cout << "[ParameterBrowserTab] ✅ MIDI Learn started for " << parameter->getName() << std::endl;
        } else {
            showStatusMessage("Failed to start MIDI Learn", false);
        }
    } catch (const std::exception& e) {
        showStatusMessage("Assignment error: " + std::string(e.what()), false);
    }
}

void ParameterBrowserTab::showMacroCreationDialog(std::shared_ptr<Parameter> parameter) {
    std::cout << "[ParameterBrowserTab] 🎛️ Showing macro creation for: " << parameter->getName() << std::endl;
    
    // For now, create a simple single-parameter macro
    // In a full implementation, this would show a dialog for multi-parameter macro creation
    
    try {
        std::string macro_name = parameter->getShortName() + " Macro";
        std::vector<std::string> param_names = {parameter->getName()};
        
        bool success = synth_param_manager_->createMacro(macro_name, param_names);
        if (success) {
            showStatusMessage("Created macro: " + macro_name);
            std::cout << "[ParameterBrowserTab] ✅ Created macro for " << parameter->getName() << std::endl;
        } else {
            showStatusMessage("Failed to create macro", false);
        }
    } catch (const std::exception& e) {
        showStatusMessage("Macro creation error: " + std::string(e.what()), false);
    }
}

void ParameterBrowserTab::updateStatusDisplay() {
    if (!status_label_ || !synth_param_manager_) return;
    
    std::string status = "🎛️ " + synth_param_manager_->getCurrentSynthName();
    if (status.find("ASM Hydrasynth") == std::string::npos && !synth_param_manager_->getCurrentSynthName().empty()) {
        status = "🎛️ Parameter Browser - " + synth_param_manager_->getCurrentSynthName();
    } else if (synth_param_manager_->getCurrentSynthName().empty()) {
        status = "🎛️ Parameter Browser - No Synthesizer Loaded";
    }
    
    lv_label_set_text(status_label_, status.c_str());
}

void ParameterBrowserTab::updateQuickStats() {
    if (!quick_stats_label_ || !synth_param_manager_) return;
    
    std::string stats = formatQuickStats();
    lv_label_set_text(quick_stats_label_, stats.c_str());
}

void ParameterBrowserTab::showStatusMessage(const std::string& message, bool is_success) {
    std::cout << "[ParameterBrowserTab] " << (is_success ? "✅" : "❌") << " " << message << std::endl;
    
    // In a full implementation, this could show a temporary status popup
    // For now, we'll just update the status label temporarily
    if (status_label_) {
        std::string icon = is_success ? "✅" : "❌";
        std::string temp_status = icon + " " + message;
        lv_label_set_text(status_label_, temp_status.c_str());
        
        // Reset to normal status after 3 seconds (in a real implementation)
        // For now, we'll just rely on the next status update
    }
}

void ParameterBrowserTab::loadDefaultSynthesizer() {
    std::cout << "[ParameterBrowserTab] 🎹 Loading default synthesizer" << std::endl;
    
    try {
        bool success = synth_param_manager_->loadSynthDefinition("hydrasynth");
        if (success) {
            std::cout << "[ParameterBrowserTab] ✅ Loaded Hydrasynth with " 
                      << synth_param_manager_->getParameterCount() << " parameters" << std::endl;
            
            showStatusMessage("Loaded " + synth_param_manager_->getCurrentSynthName());
            
            // Refresh the parameter selection UI
            if (param_selection_ui_) {
                param_selection_ui_->refreshParameterList();
            }
        } else {
            std::cerr << "[ParameterBrowserTab] ❌ Failed to load default synthesizer" << std::endl;
            showStatusMessage("Failed to load synthesizer", false);
        }
    } catch (const std::exception& e) {
        std::cerr << "[ParameterBrowserTab] ❌ Exception loading synthesizer: " << e.what() << std::endl;
        showStatusMessage("Error loading synthesizer", false);
    }
    
    updateStatusDisplay();
    updateQuickStats();
}

void ParameterBrowserTab::refreshParameterData() {
    if (param_selection_ui_) {
        param_selection_ui_->refreshParameterList();
    }
    updateQuickStats();
}

std::string ParameterBrowserTab::formatQuickStats() const {
    if (!synth_param_manager_) {
        return "No data";
    }
    
    std::stringstream stats;
    
    size_t total_params = synth_param_manager_->getParameterCount();
    size_t favorites = synth_param_manager_->getFavoriteParameters().size();
    size_t assignments = synth_param_manager_->getAllAssignments().size();
    size_t macros = synth_param_manager_->getAllMacros().size();
    
    stats << total_params << " params • " 
          << favorites << " favorites • "
          << assignments << " assigned • "
          << macros << " macros";
    
    return stats.str();
}