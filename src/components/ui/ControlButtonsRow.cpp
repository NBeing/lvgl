#include "ControlButtonsRow.h"
#include "components/controls/ButtonControl.h"
#include "components/parameter/ParameterBinder.h"
#include "components/parameter/Parameter.h"
#include "components/layout/LayoutManager.h"
#include <iostream>

ControlButtonsRow::ControlButtonsRow(ParameterBinder* parameter_binder)
    : parameter_binder_(parameter_binder)
    , container_(nullptr)
{
}

void ControlButtonsRow::create(lv_obj_t* parent) {
    if (container_) return; // Already created

    // Create container for buttons
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 2, 0);

    setupFlexLayout();
    createButtons();

    std::cout << "ControlButtonsRow created with " << buttons_.size() << " buttons" << std::endl;
}

void ControlButtonsRow::destroy() {
    if (container_) {
        buttons_.clear();
        lv_obj_del(container_);
        container_ = nullptr;
    }
}

void ControlButtonsRow::setButtonDefinitions(const std::vector<ButtonDefinition>& definitions) {
    button_definitions_ = definitions;
}

void ControlButtonsRow::setupFlexLayout() {
    // Set up flex layout for buttons
    lv_obj_set_layout(container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(container_, 20, 0);
}

void ControlButtonsRow::createButtons() {
    if (button_definitions_.empty()) {
        std::cout << "Warning: No button definitions provided" << std::endl;
        return;
    }

    buttons_.clear();
    const auto& layout = LayoutManager::getConfig();

    for (const auto& def : button_definitions_) {
        auto button = std::make_shared<ButtonControl>(container_, 0, 0, 
                                                     layout.button_width, layout.button_height);
        button->setLabel(def.label);
        
        // Set button mode
        switch (def.mode) {
            case ButtonDefinition::Mode::TOGGLE:
                button->setMode(ButtonControl::ButtonMode::TOGGLE);
                break;
            case ButtonDefinition::Mode::TRIGGER:
                button->setMode(ButtonControl::ButtonMode::TRIGGER);
                break;
        }
        
        buttons_.push_back(button);
        
        std::cout << "Created button: " << def.label << " for parameter " << def.parameter_name << std::endl;
    }
}

void ControlButtonsRow::bindParameters() {
    if (!parameter_binder_) {
        std::cout << "ERROR: ParameterBinder is null" << std::endl;
        return;
    }

    for (size_t i = 0; i < buttons_.size() && i < button_definitions_.size(); ++i) {
        bindButtonToParameter(buttons_[i], button_definitions_[i].parameter_name);
    }

    std::cout << "ControlButtonsRow bound " << buttons_.size() << " parameters" << std::endl;
}

void ControlButtonsRow::bindButtonToParameter(std::shared_ptr<ButtonControl> button, const std::string& parameter_name) {
    auto parameter = parameter_binder_->getParameter(parameter_name);
    if (!parameter) {
        std::cout << "Warning: Parameter '" << parameter_name << "' not found" << std::endl;
        return;
    }

    button->bindParameter(parameter);
    
    // Set up value changed callback
    button->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
        if (parameter_change_callback_) {
            parameter_change_callback_(value, param);
        }
    });

    std::cout << "Bound button to parameter: " << parameter_name << std::endl;
}

std::shared_ptr<ButtonControl> ControlButtonsRow::getButton(const std::string& parameter_name) const {
    for (size_t i = 0; i < button_definitions_.size() && i < buttons_.size(); ++i) {
        if (button_definitions_[i].parameter_name == parameter_name) {
            return buttons_[i];
        }
    }
    return nullptr;
}
