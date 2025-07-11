#include "GlobalSettingsModal.h"
#include "SettingsManager.h"
#include "Constants.h"
#include "FontConfig.h"
#include <iostream>

GlobalSettingsModal::GlobalSettingsModal()
    : settings_container_(nullptr)
    , scroll_container_(nullptr)
    , save_btn_(nullptr)
    , cancel_btn_(nullptr)
    , reset_btn_(nullptr)
    , has_unsaved_changes_(false)
{
    // Register as observer to settings manager
    SettingsManager::getInstance().addObserver("GlobalSettingsModal", 
        [this](const std::string& key, const std::any& /* old_value */, const std::any& /* new_value */) {
            onSettingChanged(key);
        });
}

void GlobalSettingsModal::create(lv_obj_t* parent) {
    if (modal_) return; // Already created

    // Create modal
    modal_ = std::make_unique<Modal>();
    
    Modal::ModalConfig config;
    config.title = "Global Settings";
    config.width_percent = 90;
    config.height_percent = 85;
    config.modal_bg_color = 0x1E1E2E;
    config.modal_border_color = 0x6C7086;
    config.title_color = 0xCDD6F4;
    config.show_close_button = true;
    config.close_on_background_click = false; // Prevent accidental closes
    
    modal_->setConfig(config);
    modal_->setContentCreator([this](lv_obj_t* parent) {
        createSettingsContent(parent);
    });
    
    modal_->setOnCloseCallback([this]() {
        if (has_unsaved_changes_) {
            std::cout << "GlobalSettingsModal: Discarding unsaved changes" << std::endl;
            // Refresh all components from manager to discard changes
            for (auto& component : setting_components_) {
                component->updateFromManager();
            }
            has_unsaved_changes_ = false;
        }
    });

    modal_->create(parent);
    
    std::cout << "GlobalSettingsModal created" << std::endl;
}

void GlobalSettingsModal::show() {
    if (modal_) {
        modal_->show();
    }
}

void GlobalSettingsModal::hide() {
    if (modal_) {
        modal_->hide();
    }
}

bool GlobalSettingsModal::isVisible() const {
    return modal_ && modal_->isVisible();
}

void GlobalSettingsModal::registerGlobalSettings() {
    auto& settings = SettingsManager::getInstance();

    // Audio Settings
    SettingsManager::SettingDefinition audio_sample_rate;
    audio_sample_rate.key = "audio.sample_rate";
    audio_sample_rate.display_name = "Audio Sample Rate";
    audio_sample_rate.description = "Audio sampling frequency in Hz";
    audio_sample_rate.type = SettingsManager::SettingType::LIST_SELECTION;
    audio_sample_rate.list_options = {"22050 Hz", "44100 Hz", "48000 Hz", "96000 Hz"};
    audio_sample_rate.default_value = 1; // 44100 Hz
    audio_sample_rate.current_value = 1;
    settings.registerSetting(audio_sample_rate);

    // Display Settings
    SettingsManager::SettingDefinition display_brightness;
    display_brightness.key = "display.brightness";
    display_brightness.display_name = "Display Brightness";
    display_brightness.description = "Screen brightness percentage";
    display_brightness.type = SettingsManager::SettingType::INTEGER;
    display_brightness.min_value = 10;
    display_brightness.max_value = 100;
    display_brightness.default_value = 80;
    display_brightness.current_value = 80;
    settings.registerSetting(display_brightness);

    SettingsManager::SettingDefinition display_timeout;
    display_timeout.key = "display.timeout";
    display_timeout.display_name = "Display Auto-Sleep";
    display_timeout.description = "Enable automatic display sleep";
    display_timeout.type = SettingsManager::SettingType::BOOLEAN;
    display_timeout.default_value = true;
    display_timeout.current_value = true;
    settings.registerSetting(display_timeout);

    // MIDI Settings
    SettingsManager::SettingDefinition midi_channel;
    midi_channel.key = "midi.channel";
    midi_channel.display_name = "MIDI Channel";
    midi_channel.description = "Primary MIDI channel (1-16)";
    midi_channel.type = SettingsManager::SettingType::INTEGER;
    midi_channel.min_value = 1;
    midi_channel.max_value = 16;
    midi_channel.default_value = 1;
    midi_channel.current_value = 1;
    settings.registerSetting(midi_channel);

    SettingsManager::SettingDefinition midi_device;
    midi_device.key = "midi.device_name";
    midi_device.display_name = "Device Name";
    midi_device.description = "MIDI device name for identification";
    midi_device.type = SettingsManager::SettingType::STRING;
    midi_device.max_length = 32;
    midi_device.default_value = std::string("LVGL Synth");
    midi_device.current_value = std::string("LVGL Synth");
    settings.registerSetting(midi_device);

    // System Settings
    SettingsManager::SettingDefinition auto_save;
    auto_save.key = "system.auto_save";
    auto_save.display_name = "Auto-Save Presets";
    auto_save.description = "Automatically save parameter changes";
    auto_save.type = SettingsManager::SettingType::BOOLEAN;
    auto_save.default_value = true;
    auto_save.current_value = true;
    settings.registerSetting(auto_save);

    SettingsManager::SettingDefinition debug_mode;
    debug_mode.key = "system.debug_mode";
    debug_mode.display_name = "Debug Mode";
    debug_mode.description = "Enable debug output and logging";
    debug_mode.type = SettingsManager::SettingType::BOOLEAN;
    debug_mode.default_value = false;
    debug_mode.current_value = false;
    settings.registerSetting(debug_mode);

    std::cout << "GlobalSettingsModal: Registered " << settings.getAllSettingKeys().size() 
              << " global settings" << std::endl;
}

void GlobalSettingsModal::createSettingsContent(lv_obj_t* parent) {
    // Create main settings container
    settings_container_ = lv_obj_create(parent);
    lv_obj_set_size(settings_container_, LV_PCT(100), LV_PCT(85));
    lv_obj_set_style_bg_opa(settings_container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(settings_container_, 0, 0);
    lv_obj_set_style_pad_all(settings_container_, 5, 0);
    lv_obj_align(settings_container_, LV_ALIGN_TOP_MID, 0, 0);

    // Create scrollable container for settings
    scroll_container_ = lv_obj_create(settings_container_);
    lv_obj_set_size(scroll_container_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(scroll_container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(scroll_container_, 0, 0);
    lv_obj_set_style_pad_all(scroll_container_, 5, 0);
    
    // Set up scrolling
    lv_obj_set_scroll_dir(scroll_container_, LV_DIR_VER);
    // Note: scrollbar styling may need different approach in this LVGL version

    // Set up flex layout for settings
    lv_obj_set_layout(scroll_container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scroll_container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scroll_container_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Create setting components for all registered settings
    auto all_settings = SettingsManager::getInstance().getAllSettings();
    
    for (const auto& setting : all_settings) {
        std::unique_ptr<SettingComponent> component;
        
        switch (setting.type) {
            case SettingsManager::SettingType::BOOLEAN:
                component = std::make_unique<BooleanSetting>(setting.key, setting.display_name);
                break;
                
            case SettingsManager::SettingType::INTEGER:
                component = std::make_unique<IntegerSetting>(setting.key, setting.display_name);
                break;
                
            case SettingsManager::SettingType::STRING:
                component = std::make_unique<StringSetting>(setting.key, setting.display_name);
                break;
                
            case SettingsManager::SettingType::LIST_SELECTION:
                component = std::make_unique<ListSetting>(setting.key, setting.display_name);
                break;
                
            default:
                std::cout << "GlobalSettingsModal: Unsupported setting type for " << setting.key << std::endl;
                continue;
        }
        
        if (component) {
            component->setValueChangedCallback([this](const std::string& key) {
                has_unsaved_changes_ = true;
                std::cout << "GlobalSettingsModal: Setting '" << key << "' changed (unsaved)" << std::endl;
            });
            
            component->create(scroll_container_);
            setting_components_.push_back(std::move(component));
        }
    }

    // Create action buttons
    createActionButtons(parent);

    std::cout << "GlobalSettingsModal: Created " << setting_components_.size() << " setting components" << std::endl;
}

void GlobalSettingsModal::createActionButtons(lv_obj_t* parent) {
    // Create button container
    lv_obj_t* button_container = lv_obj_create(parent);
    lv_obj_set_size(button_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_align(button_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(button_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(button_container, 0, 0);
    lv_obj_set_style_pad_all(button_container, 10, 0);

    // Set up flex layout for buttons
    lv_obj_set_layout(button_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(button_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(button_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Save button
    save_btn_ = lv_btn_create(button_container);
    lv_obj_set_size(save_btn_, 80, 40);
    lv_obj_set_style_bg_color(save_btn_, lv_color_hex(0x00AA00), 0);
    lv_obj_add_event_cb(save_btn_, onSaveButtonClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* save_label = lv_label_create(save_btn_);
    lv_label_set_text(save_label, "Save");
    lv_obj_set_style_text_font(save_label, FontA.small, 0);
    lv_obj_center(save_label);

    // Cancel button
    cancel_btn_ = lv_btn_create(button_container);
    lv_obj_set_size(cancel_btn_, 80, 40);
    lv_obj_set_style_bg_color(cancel_btn_, lv_color_hex(0xAA0000), 0);
    lv_obj_add_event_cb(cancel_btn_, onCancelButtonClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* cancel_label = lv_label_create(cancel_btn_);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_set_style_text_font(cancel_label, FontA.small, 0);
    lv_obj_center(cancel_label);

    // Reset button
    reset_btn_ = lv_btn_create(button_container);
    lv_obj_set_size(reset_btn_, 100, 40);
    lv_obj_set_style_bg_color(reset_btn_, lv_color_hex(0xFFAA00), 0);
    lv_obj_add_event_cb(reset_btn_, onResetButtonClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* reset_label = lv_label_create(reset_btn_);
    lv_label_set_text(reset_label, "Reset Defaults");
    lv_obj_set_style_text_font(reset_label, FontA.small, 0);
    lv_obj_center(reset_label);
}

void GlobalSettingsModal::onSettingChanged(const std::string& /* key */) {
    // This is called when settings change via the SettingsManager
    // We don't need to do anything here as the UI components handle their own updates
}

void GlobalSettingsModal::onSaveClicked() {
    std::cout << "GlobalSettingsModal: Saving settings to file..." << std::endl;
    
    // TODO: Save to file
    // SettingsManager::getInstance().saveToFile("user_settings.json");
    
    has_unsaved_changes_ = false;
    hide();
}

void GlobalSettingsModal::onCancelClicked() {
    std::cout << "GlobalSettingsModal: Canceling changes..." << std::endl;
    
    if (has_unsaved_changes_) {
        // Restore all settings from manager
        for (auto& component : setting_components_) {
            component->updateFromManager();
        }
        has_unsaved_changes_ = false;
    }
    
    hide();
}

void GlobalSettingsModal::onResetToDefaultsClicked() {
    std::cout << "GlobalSettingsModal: Resetting to defaults..." << std::endl;
    
    SettingsManager::getInstance().resetToDefaults();
    
    // Update all UI components
    for (auto& component : setting_components_) {
        component->updateFromManager();
    }
    
    has_unsaved_changes_ = true;
}

// Static event handlers
void GlobalSettingsModal::onSaveButtonClicked(lv_event_t* e) {
    GlobalSettingsModal* modal = static_cast<GlobalSettingsModal*>(lv_event_get_user_data(e));
    if (modal) {
        modal->onSaveClicked();
    }
}

void GlobalSettingsModal::onCancelButtonClicked(lv_event_t* e) {
    GlobalSettingsModal* modal = static_cast<GlobalSettingsModal*>(lv_event_get_user_data(e));
    if (modal) {
        modal->onCancelClicked();
    }
}

void GlobalSettingsModal::onResetButtonClicked(lv_event_t* e) {
    GlobalSettingsModal* modal = static_cast<GlobalSettingsModal*>(lv_event_get_user_data(e));
    if (modal) {
        modal->onResetToDefaultsClicked();
    }
}
