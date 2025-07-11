#pragma once

#include "components/ui/Window.h"
#include "components/settings/GlobalSettingsModal.h"
#include <memory>

/**
 * @brief Settings tab for application configuration
 * 
 * This tab provides access to global settings and potentially
 * tab-specific settings in the future.
 */
class SettingsTab : public Tab {
public:
    SettingsTab();
    ~SettingsTab() = default;

    // Tab interface
    void create(lv_obj_t* parent) override;
    void onActivated() override;
    void onDeactivated() override;

private:
    void createSettingsButtons();
    
    // Event handlers
    void onGlobalSettingsClicked();
    void onAboutClicked();
    void onSystemInfoClicked();

    // Static event callbacks
    static void onGlobalSettingsButtonClicked(lv_event_t* e);
    static void onAboutButtonClicked(lv_event_t* e);
    static void onSystemInfoButtonClicked(lv_event_t* e);

    // UI Elements
    lv_obj_t* buttons_container_;
    lv_obj_t* global_settings_btn_;
    lv_obj_t* about_btn_;
    lv_obj_t* system_info_btn_;

    // Settings modal
    std::unique_ptr<GlobalSettingsModal> global_settings_modal_;
};
