#pragma once

#include <lvgl.h>
#include <string>
#include <functional>
#include <vector>

/**
 * @brief Base class for all setting UI components
 * 
 * Provides common functionality for displaying and editing settings
 */
class SettingComponent {
public:
    using ValueChangedCallback = std::function<void(const std::string& key)>;

    SettingComponent(const std::string& key, const std::string& display_name);
    virtual ~SettingComponent() = default;

    // UI Creation
    virtual void create(lv_obj_t* parent) = 0;
    virtual void destroy();

    // Value management
    virtual void updateFromManager() = 0;  // Update UI from SettingsManager
    virtual void commitToManager() = 0;    // Commit UI value to SettingsManager

    // Events
    void setValueChangedCallback(ValueChangedCallback callback) { value_changed_callback_ = callback; }

    // Getters
    const std::string& getKey() const { return key_; }
    const std::string& getDisplayName() const { return display_name_; }
    lv_obj_t* getContainer() const { return container_; }
    bool isCreated() const { return container_ != nullptr; }

protected:
    void notifyValueChanged() { if (value_changed_callback_) value_changed_callback_(key_); }

    std::string key_;
    std::string display_name_;
    lv_obj_t* container_;
    ValueChangedCallback value_changed_callback_;
};

/**
 * @brief Boolean toggle setting (ON/OFF switch)
 */
class BooleanSetting : public SettingComponent {
public:
    BooleanSetting(const std::string& key, const std::string& display_name);

    void create(lv_obj_t* parent) override;
    void updateFromManager() override;
    void commitToManager() override;

private:
    static void onSwitchToggled(lv_event_t* e);

    lv_obj_t* label_;
    lv_obj_t* switch_;
};

/**
 * @brief Integer setting with +/- buttons
 */
class IntegerSetting : public SettingComponent {
public:
    IntegerSetting(const std::string& key, const std::string& display_name);

    void create(lv_obj_t* parent) override;
    void updateFromManager() override;
    void commitToManager() override;

private:
    void updateLabel();
    static void onMinusClicked(lv_event_t* e);
    static void onPlusClicked(lv_event_t* e);

    lv_obj_t* label_;
    lv_obj_t* value_label_;
    lv_obj_t* minus_btn_;
    lv_obj_t* plus_btn_;
    int current_value_;
};

/**
 * @brief String setting with text input
 */
class StringSetting : public SettingComponent {
public:
    StringSetting(const std::string& key, const std::string& display_name);

    void create(lv_obj_t* parent) override;
    void updateFromManager() override;
    void commitToManager() override;

private:
    static void onTextChanged(lv_event_t* e);

    lv_obj_t* label_;
    lv_obj_t* textarea_;
};

/**
 * @brief List selection setting (dropdown-like)
 */
class ListSetting : public SettingComponent {
public:
    ListSetting(const std::string& key, const std::string& display_name);

    void create(lv_obj_t* parent) override;
    void updateFromManager() override;
    void commitToManager() override;

private:
    void updateSelectionLabel();
    static void onPreviousClicked(lv_event_t* e);
    static void onNextClicked(lv_event_t* e);

    lv_obj_t* label_;
    lv_obj_t* selection_label_;
    lv_obj_t* prev_btn_;
    lv_obj_t* next_btn_;
    std::vector<std::string> options_;
    int current_selection_;
};
