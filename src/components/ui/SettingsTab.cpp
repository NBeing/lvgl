#include "SettingsTab.h"
#include "Constants.h"
#include "FontConfig.h"
#include "components/settings/SettingsManager.h"
#include <iostream>

SettingsTab::SettingsTab()
    : Tab("Settings")
    , buttons_container_(nullptr)
    , global_settings_btn_(nullptr)
    , about_btn_(nullptr)
    , system_info_btn_(nullptr)
{
}

void SettingsTab::create(lv_obj_t* parent) {
    if (container_) return; // Already created

    // Create main container for this tab
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(98), LV_PCT(98));
    lv_obj_set_pos(container_, 0, 0);
    lv_obj_set_style_bg_color(container_, lv_color_hex(SynthConstants::Color::BG), 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 8, 0);

    setContainer(container_);

    // Create title
    lv_obj_t* title = lv_label_create(container_);
    lv_label_set_text(title, "Application Settings");
    lv_obj_set_style_text_color(title, lv_color_hex(SynthConstants::Color::TITLE), 0);
    lv_obj_set_style_text_font(title, FontA.med, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Create subtitle
    lv_obj_t* subtitle = lv_label_create(container_);
    lv_label_set_text(subtitle, "Configure global application settings and preferences");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(SynthConstants::Color::HELP), 0);
    lv_obj_set_style_text_font(subtitle, FontA.small, 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 50);

    createSettingsButtons();

    // Create global settings modal
    global_settings_modal_ = std::make_unique<GlobalSettingsModal>();
    global_settings_modal_->registerGlobalSettings();
    global_settings_modal_->create(container_);

    std::cout << "SettingsTab created with global settings integration" << std::endl;
}

void SettingsTab::createSettingsButtons() {
    // Create container for buttons
    buttons_container_ = lv_obj_create(container_);
    lv_obj_set_size(buttons_container_, LV_PCT(90), LV_SIZE_CONTENT);
    lv_obj_align(buttons_container_, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_opa(buttons_container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(buttons_container_, 0, 0);
    lv_obj_set_style_pad_all(buttons_container_, 10, 0);

    // Set up flex layout
    lv_obj_set_layout(buttons_container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(buttons_container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(buttons_container_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(buttons_container_, 15, 0);

    // Global Settings Button
    global_settings_btn_ = lv_btn_create(buttons_container_);
    lv_obj_set_size(global_settings_btn_, 280, 60);
    lv_obj_set_style_bg_color(global_settings_btn_, lv_color_hex(0x2E2E3A), 0);
    lv_obj_set_style_border_color(global_settings_btn_, lv_color_hex(0x6C7086), 0);
    lv_obj_set_style_border_width(global_settings_btn_, 2, 0);
    lv_obj_set_style_radius(global_settings_btn_, 8, 0);

    // Create container for button content
    lv_obj_t* global_content = lv_obj_create(global_settings_btn_);
    lv_obj_set_size(global_content, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(global_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(global_content, 0, 0);
    lv_obj_set_style_pad_all(global_content, 8, 0);
    lv_obj_add_flag(global_content, LV_OBJ_FLAG_EVENT_BUBBLE); // Allow events to bubble up to button

    lv_obj_t* global_title = lv_label_create(global_content);
    lv_label_set_text(global_title, "Global Settings");
    lv_obj_set_style_text_color(global_title, lv_color_hex(0xCDD6F4), 0);
    lv_obj_set_style_text_font(global_title, FontA.small, 0);
    lv_obj_align(global_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* global_desc = lv_label_create(global_content);
    lv_label_set_text(global_desc, "Configure audio, display, MIDI, and system settings");
    lv_obj_set_style_text_color(global_desc, lv_color_hex(0x9399B2), 0);
    lv_obj_set_style_text_font(global_desc, FontA.small, 0);
    lv_obj_align(global_desc, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_add_event_cb(global_settings_btn_, onGlobalSettingsButtonClicked, LV_EVENT_CLICKED, this);

    // About Button
    about_btn_ = lv_btn_create(buttons_container_);
    lv_obj_set_size(about_btn_, 280, 60);
    lv_obj_set_style_bg_color(about_btn_, lv_color_hex(0x1E1E2E), 0);
    lv_obj_set_style_border_color(about_btn_, lv_color_hex(0x585B70), 0);
    lv_obj_set_style_border_width(about_btn_, 2, 0);
    lv_obj_set_style_radius(about_btn_, 8, 0);

    lv_obj_t* about_content = lv_obj_create(about_btn_);
    lv_obj_set_size(about_content, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(about_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(about_content, 0, 0);
    lv_obj_set_style_pad_all(about_content, 8, 0);
    lv_obj_add_flag(about_content, LV_OBJ_FLAG_EVENT_BUBBLE); // Allow events to bubble up to button

    lv_obj_t* about_title = lv_label_create(about_content);
    lv_label_set_text(about_title, "About Application");
    lv_obj_set_style_text_color(about_title, lv_color_hex(0xCDD6F4), 0);
    lv_obj_set_style_text_font(about_title, FontA.small, 0);
    lv_obj_align(about_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* about_desc = lv_label_create(about_content);
    lv_label_set_text(about_desc, "Version information and credits");
    lv_obj_set_style_text_color(about_desc, lv_color_hex(0x9399B2), 0);
    lv_obj_set_style_text_font(about_desc, FontA.small, 0);
    lv_obj_align(about_desc, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_add_event_cb(about_btn_, onAboutButtonClicked, LV_EVENT_CLICKED, this);

    // System Info Button
    system_info_btn_ = lv_btn_create(buttons_container_);
    lv_obj_set_size(system_info_btn_, 280, 60);
    lv_obj_set_style_bg_color(system_info_btn_, lv_color_hex(0x1E1E2E), 0);
    lv_obj_set_style_border_color(system_info_btn_, lv_color_hex(0x585B70), 0);
    lv_obj_set_style_border_width(system_info_btn_, 2, 0);
    lv_obj_set_style_radius(system_info_btn_, 8, 0);

    lv_obj_t* info_content = lv_obj_create(system_info_btn_);
    lv_obj_set_size(info_content, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(info_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(info_content, 0, 0);
    lv_obj_set_style_pad_all(info_content, 8, 0);
    lv_obj_add_flag(info_content, LV_OBJ_FLAG_EVENT_BUBBLE); // Allow events to bubble up to button

    lv_obj_t* info_title = lv_label_create(info_content);
    lv_label_set_text(info_title, "System Information");
    lv_obj_set_style_text_color(info_title, lv_color_hex(0xCDD6F4), 0);
    lv_obj_set_style_text_font(info_title, FontA.small, 0);
    lv_obj_align(info_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* info_desc = lv_label_create(info_content);
    lv_label_set_text(info_desc, "Hardware status, memory usage, and diagnostics");
    lv_obj_set_style_text_color(info_desc, lv_color_hex(0x9399B2), 0);
    lv_obj_set_style_text_font(info_desc, FontA.small, 0);
    lv_obj_align(info_desc, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_add_event_cb(system_info_btn_, onSystemInfoButtonClicked, LV_EVENT_CLICKED, this);
}

void SettingsTab::onGlobalSettingsClicked() {
    std::cout << "SettingsTab: onGlobalSettingsClicked() called!" << std::endl;
    std::cout << "SettingsTab: Opening global settings modal..." << std::endl;
    if (global_settings_modal_) {
        std::cout << "SettingsTab: Modal exists, calling show()..." << std::endl;
        global_settings_modal_->show();
    } else {
        std::cout << "SettingsTab: ERROR - global_settings_modal_ is null!" << std::endl;
    }
}

void SettingsTab::onAboutClicked() {
    std::cout << "SettingsTab: About clicked (not implemented yet)" << std::endl;
    // TODO: Show about modal with version info
}

void SettingsTab::onSystemInfoClicked() {
    std::cout << "SettingsTab: System info clicked (not implemented yet)" << std::endl;
    // TODO: Show system info modal with hardware status
}

void SettingsTab::onActivated() {
    std::cout << "SettingsTab activated - Settings interface ready" << std::endl;
}

void SettingsTab::onDeactivated() {
    std::cout << "SettingsTab deactivated" << std::endl;
}

// Static event handlers
void SettingsTab::onGlobalSettingsButtonClicked(lv_event_t* e) {
    std::cout << "SettingsTab: onGlobalSettingsButtonClicked() event received!" << std::endl;
    SettingsTab* tab = static_cast<SettingsTab*>(lv_event_get_user_data(e));
    if (tab) {
        std::cout << "SettingsTab: Tab pointer valid, calling onGlobalSettingsClicked()" << std::endl;
        tab->onGlobalSettingsClicked();
    } else {
        std::cout << "SettingsTab: ERROR - Tab pointer is null!" << std::endl;
    }
}

void SettingsTab::onAboutButtonClicked(lv_event_t* e) {
    SettingsTab* tab = static_cast<SettingsTab*>(lv_event_get_user_data(e));
    if (tab) {
        tab->onAboutClicked();
    }
}

void SettingsTab::onSystemInfoButtonClicked(lv_event_t* e) {
    SettingsTab* tab = static_cast<SettingsTab*>(lv_event_get_user_data(e));
    if (tab) {
        tab->onSystemInfoClicked();
    }
}
