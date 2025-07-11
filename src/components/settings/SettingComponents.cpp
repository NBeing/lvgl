#include "SettingComponents.h"
#include "SettingsManager.h"
#include "Constants.h"
#include "FontConfig.h"
#include <iostream>

// Base SettingComponent implementation
SettingComponent::SettingComponent(const std::string& key, const std::string& display_name)
    : key_(key)
    , display_name_(display_name)
    , container_(nullptr)
{
}

void SettingComponent::destroy() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
    }
}

// BooleanSetting implementation
BooleanSetting::BooleanSetting(const std::string& key, const std::string& display_name)
    : SettingComponent(key, display_name)
    , label_(nullptr)
    , switch_(nullptr)
{
}

void BooleanSetting::create(lv_obj_t* parent) {
    if (container_) return;

    // Create container
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 8, 0);

    // Create label
    label_ = lv_label_create(container_);
    lv_label_set_text(label_, display_name_.c_str());
    lv_obj_set_style_text_color(label_, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_, FontA.small, 0);
    lv_obj_align(label_, LV_ALIGN_LEFT_MID, 0, 0);

    // Create switch
    switch_ = lv_switch_create(container_);
    lv_obj_align(switch_, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(switch_, onSwitchToggled, LV_EVENT_VALUE_CHANGED, this);

    updateFromManager();
}

void BooleanSetting::updateFromManager() {
    if (!switch_) return;

    bool value = SettingsManager::getInstance().getBool(key_);
    if (value) {
        lv_obj_add_state(switch_, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(switch_, LV_STATE_CHECKED);
    }
}

void BooleanSetting::commitToManager() {
    if (!switch_) return;

    bool value = lv_obj_has_state(switch_, LV_STATE_CHECKED);
    SettingsManager::getInstance().setValue(key_, value);
    notifyValueChanged();
}

void BooleanSetting::onSwitchToggled(lv_event_t* e) {
    BooleanSetting* setting = static_cast<BooleanSetting*>(lv_event_get_user_data(e));
    if (setting) {
        setting->commitToManager();
    }
}

// IntegerSetting implementation
IntegerSetting::IntegerSetting(const std::string& key, const std::string& display_name)
    : SettingComponent(key, display_name)
    , label_(nullptr)
    , value_label_(nullptr)
    , minus_btn_(nullptr)
    , plus_btn_(nullptr)
    , current_value_(0)
{
}

void IntegerSetting::create(lv_obj_t* parent) {
    if (container_) return;

    // Create container
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 8, 0);

    // Create label
    label_ = lv_label_create(container_);
    lv_label_set_text(label_, display_name_.c_str());
    lv_obj_set_style_text_color(label_, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_, FontA.small, 0);
    lv_obj_align(label_, LV_ALIGN_LEFT_MID, 0, 0);

    // Create minus button
    minus_btn_ = lv_btn_create(container_);
    lv_obj_set_size(minus_btn_, 30, 30);
    lv_obj_align(minus_btn_, LV_ALIGN_RIGHT_MID, -80, 0);
    lv_obj_add_event_cb(minus_btn_, onMinusClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* minus_label = lv_label_create(minus_btn_);
    lv_label_set_text(minus_label, "-");
    lv_obj_center(minus_label);

    // Create plus button
    plus_btn_ = lv_btn_create(container_);
    lv_obj_set_size(plus_btn_, 30, 30);
    lv_obj_align(plus_btn_, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_event_cb(plus_btn_, onPlusClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* plus_label = lv_label_create(plus_btn_);
    lv_label_set_text(plus_label, "+");
    lv_obj_center(plus_label);

    // Create value label
    value_label_ = lv_label_create(container_);
    lv_obj_set_style_text_color(value_label_, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(value_label_, FontA.small, 0);
    lv_obj_align(value_label_, LV_ALIGN_RIGHT_MID, -45, 0);

    updateFromManager();
}

void IntegerSetting::updateFromManager() {
    if (!value_label_) return;

    current_value_ = SettingsManager::getInstance().getInt(key_);
    updateLabel();
}

void IntegerSetting::updateLabel() {
    if (value_label_) {
        lv_label_set_text_fmt(value_label_, "%d", current_value_);
    }
}

void IntegerSetting::commitToManager() {
    SettingsManager::getInstance().setValue(key_, current_value_);
    notifyValueChanged();
}

void IntegerSetting::onMinusClicked(lv_event_t* e) {
    IntegerSetting* setting = static_cast<IntegerSetting*>(lv_event_get_user_data(e));
    if (setting) {
        setting->current_value_--;
        setting->updateLabel();
        setting->commitToManager();
    }
}

void IntegerSetting::onPlusClicked(lv_event_t* e) {
    IntegerSetting* setting = static_cast<IntegerSetting*>(lv_event_get_user_data(e));
    if (setting) {
        setting->current_value_++;
        setting->updateLabel();
        setting->commitToManager();
    }
}

// StringSetting implementation
StringSetting::StringSetting(const std::string& key, const std::string& display_name)
    : SettingComponent(key, display_name)
    , label_(nullptr)
    , textarea_(nullptr)
{
}

void StringSetting::create(lv_obj_t* parent) {
    if (container_) return;

    // Create container
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 8, 0);

    // Create label
    label_ = lv_label_create(container_);
    lv_label_set_text(label_, display_name_.c_str());
    lv_obj_set_style_text_color(label_, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_, FontA.small, 0);
    lv_obj_align(label_, LV_ALIGN_TOP_LEFT, 0, 0);

    // Create text area
    textarea_ = lv_textarea_create(container_);
    lv_obj_set_size(textarea_, LV_PCT(100), 40);
    lv_obj_align(textarea_, LV_ALIGN_TOP_LEFT, 0, 25);
    lv_obj_add_event_cb(textarea_, onTextChanged, LV_EVENT_VALUE_CHANGED, this);

    updateFromManager();
}

void StringSetting::updateFromManager() {
    if (!textarea_) return;

    std::string value = SettingsManager::getInstance().getString(key_);
    lv_textarea_set_text(textarea_, value.c_str());
}

void StringSetting::commitToManager() {
    if (!textarea_) return;

    const char* text = lv_textarea_get_text(textarea_);
    std::string value = text ? text : "";
    SettingsManager::getInstance().setValue(key_, value);
    notifyValueChanged();
}

void StringSetting::onTextChanged(lv_event_t* e) {
    StringSetting* setting = static_cast<StringSetting*>(lv_event_get_user_data(e));
    if (setting) {
        setting->commitToManager();
    }
}

// ListSetting implementation
ListSetting::ListSetting(const std::string& key, const std::string& display_name)
    : SettingComponent(key, display_name)
    , label_(nullptr)
    , selection_label_(nullptr)
    , prev_btn_(nullptr)
    , next_btn_(nullptr)
    , current_selection_(0)
{
}

void ListSetting::create(lv_obj_t* parent) {
    if (container_) return;

    // Get options from settings manager
    auto* def = SettingsManager::getInstance().getSettingDefinition(key_);
    if (def) {
        options_ = def->list_options;
    }

    // Create container
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 8, 0);

    // Create label
    label_ = lv_label_create(container_);
    lv_label_set_text(label_, display_name_.c_str());
    lv_obj_set_style_text_color(label_, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_, FontA.small, 0);
    lv_obj_align(label_, LV_ALIGN_LEFT_MID, 0, 0);

    // Create previous button
    prev_btn_ = lv_btn_create(container_);
    lv_obj_set_size(prev_btn_, 30, 30);
    lv_obj_align(prev_btn_, LV_ALIGN_RIGHT_MID, -120, 0);
    lv_obj_add_event_cb(prev_btn_, onPreviousClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* prev_label = lv_label_create(prev_btn_);
    lv_label_set_text(prev_label, "<");
    lv_obj_center(prev_label);

    // Create next button
    next_btn_ = lv_btn_create(container_);
    lv_obj_set_size(next_btn_, 30, 30);
    lv_obj_align(next_btn_, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_event_cb(next_btn_, onNextClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* next_label = lv_label_create(next_btn_);
    lv_label_set_text(next_label, ">");
    lv_obj_center(next_label);

    // Create selection label
    selection_label_ = lv_label_create(container_);
    lv_obj_set_style_text_color(selection_label_, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(selection_label_, FontA.small, 0);
    lv_obj_align(selection_label_, LV_ALIGN_RIGHT_MID, -65, 0);

    updateFromManager();
}

void ListSetting::updateFromManager() {
    if (!selection_label_) return;

    current_selection_ = SettingsManager::getInstance().getListSelection(key_);
    updateSelectionLabel();
}

void ListSetting::updateSelectionLabel() {
    if (selection_label_ && current_selection_ >= 0 && 
        current_selection_ < static_cast<int>(options_.size())) {
        lv_label_set_text(selection_label_, options_[current_selection_].c_str());
    }
}

void ListSetting::commitToManager() {
    SettingsManager::getInstance().setValue(key_, current_selection_);
    notifyValueChanged();
}

void ListSetting::onPreviousClicked(lv_event_t* e) {
    ListSetting* setting = static_cast<ListSetting*>(lv_event_get_user_data(e));
    if (setting && setting->current_selection_ > 0) {
        setting->current_selection_--;
        setting->updateSelectionLabel();
        setting->commitToManager();
    }
}

void ListSetting::onNextClicked(lv_event_t* e) {
    ListSetting* setting = static_cast<ListSetting*>(lv_event_get_user_data(e));
    if (setting && setting->current_selection_ < static_cast<int>(setting->options_.size()) - 1) {
        setting->current_selection_++;
        setting->updateSelectionLabel();
        setting->commitToManager();
    }
}
