#pragma once

#include "components/ui/Window.h"
#include "components/parameter/SynthParameterManager.h"
#include "components/parameter/ParameterManager.h"
#include "components/ui/ParameterSelectionUI.h"
#include <memory>

/**
 * @brief Parameter Browser Tab for the main application
 * 
 * This tab provides a complete interface for browsing synthesizer parameters,
 * managing favorites, creating assignments, and building macros. It integrates
 * the enhanced parameter system into the main application's tab interface.
 */
class ParameterBrowserTab : public Tab {
public:
    ParameterBrowserTab();
    ~ParameterBrowserTab() = default;

    // Tab interface
    void create(lv_obj_t* parent) override;
    void onActivated() override;
    void onDeactivated() override;
    void update() override;

private:
    // Core managers
    std::unique_ptr<SynthParameterManager> synth_param_manager_;
    std::unique_ptr<ParameterSystemBridge> param_bridge_;
    std::unique_ptr<ParameterSelectionUI> param_selection_ui_;
    
    // External dependencies (injected)
    Parameters::ParameterManager* core_param_manager_;
    
    // UI Components
    lv_obj_t* main_container_;
    lv_obj_t* header_panel_;
    lv_obj_t* status_label_;
    lv_obj_t* quick_stats_label_;
    
    // Initialization and setup
    void initializeManagers();
    void setupUI(lv_obj_t* parent);
    void createHeaderPanel(lv_obj_t* parent);
    void connectToParameterManager();
    
    // Parameter selection handling
    void handleParameterSelection(std::shared_ptr<Parameter> parameter, const std::string& action);
    void showAssignmentDialog(std::shared_ptr<Parameter> parameter);
    void showMacroCreationDialog(std::shared_ptr<Parameter> parameter);
    
    // Status and feedback
    void updateStatusDisplay();
    void updateQuickStats();
    void showStatusMessage(const std::string& message, bool is_success = true);
    
    // Integration helpers
    void loadDefaultSynthesizer();
    void refreshParameterData();
    std::string formatQuickStats() const;
};