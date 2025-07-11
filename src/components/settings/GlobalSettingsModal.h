#pragma once

#include "components/ui/Modal.h"
#include "components/settings/SettingComponents.h"
#include <memory>
#include <vector>

/**
 * @brief Global settings modal that can be accessed from anywhere in the app
 * 
 * This modal provides a UI for all registered global settings and handles
 * their display and editing using reusable setting components.
 */
class GlobalSettingsModal {
public:
    GlobalSettingsModal();
    ~GlobalSettingsModal() = default;

    // Modal management
    void create(lv_obj_t* parent);
    void show();
    void hide();
    bool isVisible() const;

    // Initialize with default global settings
    void registerGlobalSettings();

private:
    void createSettingsContent(lv_obj_t* parent);
    void createActionButtons(lv_obj_t* parent);
    
    // Event handlers
    void onSettingChanged(const std::string& key);
    void onSaveClicked();
    void onCancelClicked();
    void onResetToDefaultsClicked();

    // Button event callbacks
    static void onSaveButtonClicked(lv_event_t* e);
    static void onCancelButtonClicked(lv_event_t* e);
    static void onResetButtonClicked(lv_event_t* e);

    // UI Elements
    std::unique_ptr<Modal> modal_;
    lv_obj_t* settings_container_;
    lv_obj_t* scroll_container_;
    lv_obj_t* save_btn_;
    lv_obj_t* cancel_btn_;
    lv_obj_t* reset_btn_;

    // Setting components
    std::vector<std::unique_ptr<SettingComponent>> setting_components_;

    // State
    bool has_unsaved_changes_;
};
