#pragma once

#include "components/parameter/SynthParameterManager.h"
#include "components/parameter/ParameterManager.h"
#include "components/parameter/Parameter.h"
#include "components/ui/Window.h"
#include "components/ui/ContainerFactory.h"
#include "FontConfig.h"
#include "Constants.h"
#include "lvgl.h"
#include <memory>
#include <vector>
#include <functional>

/**
 * @brief Parameter selection and browser UI for synthesizer parameters
 * 
 * This tab provides a comprehensive interface for browsing, searching, and selecting
 * synthesizer parameters. It integrates with both the SynthParameterManager for
 * parameter definitions and the ParameterManager for assignment and control.
 */
class ParameterSelectionUI : public Tab {
public:
    /**
     * @brief Callback function for parameter selection
     * @param parameter Selected parameter
     * @param action Action to perform ("assign", "favorite", "macro")
     */
    using ParameterSelectionCallback = std::function<void(std::shared_ptr<Parameter>, const std::string&)>;
    
    /**
     * @brief Constructor
     * @param synth_param_manager Enhanced parameter manager for browsing
     * @param param_manager Core parameter manager for assignment integration
     */
    ParameterSelectionUI(SynthParameterManager* synth_param_manager,
                        Parameters::ParameterManager* param_manager);
    
    ~ParameterSelectionUI() override;
    
    // Tab interface implementation
    void create(lv_obj_t* parent) override;
    void onActivated() override;
    void onDeactivated() override;
    void update() override;
    const char* getTabName() const { return "Parameters"; }
    
    /**
     * @brief Set callback for parameter selection events
     * @param callback Function to call when parameter is selected
     */
    void setSelectionCallback(ParameterSelectionCallback callback);
    
    /**
     * @brief Refresh the parameter list display
     */
    void refreshParameterList();
    
    /**
     * @brief Switch to a specific synthesizer
     * @param synth_name Name of synthesizer to load
     */
    void switchSynthesizer(const std::string& synth_name);

private:
    // Core managers
    SynthParameterManager* synth_param_manager_;
    Parameters::ParameterManager* param_manager_;
    ParameterSelectionCallback selection_callback_;
    
    // UI Components
    lv_obj_t* main_container_;
    lv_obj_t* search_container_;
    lv_obj_t* category_tabs_;
    lv_obj_t* parameter_list_;
    lv_obj_t* details_panel_;
    lv_obj_t* action_buttons_;
    
    // Search and filter
    lv_obj_t* search_input_;
    lv_obj_t* synth_dropdown_;
    lv_obj_t* favorites_btn_;
    lv_obj_t* recents_btn_;
    
    // Category filtering
    std::vector<lv_obj_t*> category_tab_buttons_;
    ParameterCategory current_category_;
    
    // Parameter display
    std::vector<lv_obj_t*> parameter_items_;
    std::shared_ptr<Parameter> selected_parameter_;
    
    // Current display mode
    enum class DisplayMode {
        ALL_PARAMETERS,
        CATEGORY_FILTERED,
        SEARCH_RESULTS,
        FAVORITES_ONLY,
        RECENTS_ONLY
    };
    DisplayMode current_mode_;
    std::string current_search_query_;
    
    // UI Creation methods - Compact layout for 480x320
    void createCompactLayout(lv_obj_t* parent);
    void createTopControls(lv_obj_t* parent);
    void createSplitView(lv_obj_t* parent);
    void createBottomActions(lv_obj_t* parent);
    void createParameterList(lv_obj_t* parent);
    void createDetailsPanel(lv_obj_t* parent);
    void createSearchSection(lv_obj_t* parent);
    void createCategoryTabs(lv_obj_t* parent);
    void createActionButtons(lv_obj_t* parent);
    
    // Parameter display methods
    void populateParameterList(const std::vector<std::shared_ptr<Parameter>>& parameters);
    void clearParameterList();
    lv_obj_t* createParameterItem(std::shared_ptr<Parameter> parameter);
    lv_obj_t* createCompactParameterItem(std::shared_ptr<Parameter> parameter);
    void updateParameterItemAppearance(lv_obj_t* item, std::shared_ptr<Parameter> parameter);
    
    // Parameter selection and details
    void selectParameter(std::shared_ptr<Parameter> parameter);
    void updateDetailsPanel();
    void showParameterInfo(std::shared_ptr<Parameter> parameter);
    
    // Category tab management
    void createCategoryTab(ParameterCategory category, const std::string& name);
    void selectCategoryTab(ParameterCategory category);
    void updateCategoryTabAppearance();
    
    // Search and filtering
    void performSearch(const std::string& query);
    void showFavorites();
    void showRecents();
    void showAllParameters();
    void showCategory(ParameterCategory category);
    
    // Action handlers
    void handleParameterAssign();
    void handleParameterFavorite();
    void handleParameterMacro();
    void handleSynthesizerChange();
    
    // Event callbacks
    static void onSearchInputChanged(lv_event_t* e);
    static void onSynthDropdownChanged(lv_event_t* e);
    static void onCategoryDropdownChanged(lv_event_t* e);
    static void onCategoryTabClicked(lv_event_t* e);
    static void onParameterItemClicked(lv_event_t* e);
    static void onFavoritesButtonClicked(lv_event_t* e);
    static void onRecentsButtonClicked(lv_event_t* e);
    static void onAssignButtonClicked(lv_event_t* e);
    static void onFavoriteButtonClicked(lv_event_t* e);
    static void onMacroButtonClicked(lv_event_t* e);
    
    // Utility methods
    std::string formatParameterInfo(std::shared_ptr<Parameter> parameter) const;
    lv_color_t getCategoryColor(ParameterCategory category) const;
    const char* getCategoryIcon(ParameterCategory category) const;
    void setButtonStyle(lv_obj_t* btn, bool active);
    void showStatusMessage(const std::string& message, bool is_error = false);
    
    // Helper methods for 8-bit styled components
    void createQuickFilterButton(lv_obj_t* parent, const char* text, lv_obj_t** btn_ptr, lv_event_cb_t callback);
    void createActionButton(lv_obj_t* parent, const char* text, lv_event_cb_t callback, uint32_t color);
    
    // Statistics and info
    void updateStatistics();
    std::string generateQuickStats() const;
};

/**
 * @brief Integration bridge between SynthParameterManager and ParameterManager
 * 
 * This class bridges the enhanced parameter browser with the real-time parameter system,
 * handling assignments, MIDI learn, and observer integration.
 */
class ParameterSystemBridge {
public:
    ParameterSystemBridge(SynthParameterManager* synth_manager, 
                         Parameters::ParameterManager* param_manager);
    
    /**
     * @brief Assign a synthesizer parameter to a MIDI CC or UI control
     * @param synth_parameter Parameter from synthesizer definition
     * @param target_id Target control ID or parameter ID for assignment
     * @param assignment_type Type of assignment ("midi_cc", "ui_control", "automation")
     * @return true if assignment successful
     */
    bool assignParameter(std::shared_ptr<Parameter> synth_parameter,
                        const std::string& target_id,
                        const std::string& assignment_type);
    
    /**
     * @brief Start MIDI learn for a synthesizer parameter
     * @param synth_parameter Parameter to learn MIDI CC for
     * @return true if MIDI learn started successfully
     */
    bool startMidiLearn(std::shared_ptr<Parameter> synth_parameter);
    
    /**
     * @brief Create automation assignment for parameter
     * @param synth_parameter Parameter to automate
     * @param automation_track_id Automation track identifier
     * @return true if automation assignment created
     */
    bool assignToAutomation(std::shared_ptr<Parameter> synth_parameter,
                           const std::string& automation_track_id);
    
    /**
     * @brief Get assignment status for a parameter
     * @param synth_parameter Parameter to check
     * @return Assignment info string, or empty if not assigned
     */
    std::string getAssignmentStatus(std::shared_ptr<Parameter> synth_parameter) const;
    
    /**
     * @brief Remove all assignments for a parameter
     * @param synth_parameter Parameter to unassign
     * @return true if any assignments were removed
     */
    bool removeAssignments(std::shared_ptr<Parameter> synth_parameter);

private:
    SynthParameterManager* synth_manager_;
    Parameters::ParameterManager* param_manager_;
    
    // Helper methods for assignment types
    bool assignToMidiCC(std::shared_ptr<Parameter> synth_parameter, const std::string& target_id);
    bool assignToUIControl(std::shared_ptr<Parameter> synth_parameter, const std::string& target_id);
    
    // Helper methods for parameter ID mapping
    Parameters::ParameterID mapToParameterID(std::shared_ptr<Parameter> synth_parameter) const;
    Parameters::ParameterID createDynamicParameterID(std::shared_ptr<Parameter> synth_parameter) const;
    std::string createParameterIDMapping(std::shared_ptr<Parameter> synth_parameter);
};