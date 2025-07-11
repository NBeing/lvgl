#pragma once

#include <lvgl.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>

class ButtonControl;
class Parameter;
class ParameterBinder;

struct ButtonDefinition {
    std::string parameter_name;
    std::string label;
    enum class Mode { TOGGLE, TRIGGER } mode;
};

class ControlButtonsRow {
public:
    using ParameterChangeCallback = std::function<void(uint8_t value, const Parameter* param)>;

    explicit ControlButtonsRow(ParameterBinder* parameter_binder);
    ~ControlButtonsRow() = default;

    // UI Creation
    void create(lv_obj_t* parent);
    void destroy();

    // Configuration
    void setButtonDefinitions(const std::vector<ButtonDefinition>& definitions);
    void setParameterChangeCallback(ParameterChangeCallback callback) { parameter_change_callback_ = callback; }

    // Parameter binding
    void bindParameters();

    // Getters
    lv_obj_t* getContainer() const { return container_; }
    std::shared_ptr<ButtonControl> getButton(const std::string& parameter_name) const;
    bool isCreated() const { return container_ != nullptr; }

private:
    void setupFlexLayout();
    void createButtons();
    void bindButtonToParameter(std::shared_ptr<ButtonControl> button, const std::string& parameter_name);

    ParameterBinder* parameter_binder_;
    lv_obj_t* container_;
    std::vector<ButtonDefinition> button_definitions_;
    std::vector<std::shared_ptr<ButtonControl>> buttons_;
    ParameterChangeCallback parameter_change_callback_;
};
