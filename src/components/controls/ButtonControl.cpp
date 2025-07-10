
#include "ButtonControl.h"
#include "components/parameter/Command.h"
#include "components/parameter/CommandManager.h"
#include <lvgl.h>
#include <iostream>
#include "FontConfig.h"

ButtonControl::ButtonControl(lv_obj_t* parent, int x, int y, int width, int height)
    : ParameterControl()
    , button_(nullptr)
    , label_(nullptr)
    , mode_(ButtonMode::TOGGLE)
    , is_pressed_(false)
    , is_toggled_(false)
    , normal_color_(lv_color_hex(0x333333))
    , pressed_color_(lv_color_hex(0x666666))
    , off_color_(lv_color_hex(0x444444))
    , on_color_(lv_color_hex(0x00AA00))
    , off_value_(0)
    , on_value_(127)
    , trigger_value_(127)
    , default_value_(0)
{
    createButton(parent, x, y, width, height);
}

ButtonControl::~ButtonControl() {
    // LVGL objects are automatically cleaned up by parent
}

void ButtonControl::createButton(lv_obj_t* parent, int x, int y, int width, int height) {
    // Create button object
    button_ = lv_btn_create(parent);
    lv_obj_set_size(button_, width, height);
    lv_obj_set_pos(button_, x, y);
    
    // Set initial styling
    lv_obj_set_style_bg_color(button_, normal_color_, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(button_, pressed_color_, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(button_, 2, LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(button_, lv_color_hex(0x888888), LV_STATE_DEFAULT);
    lv_obj_set_style_radius(button_, 4, LV_STATE_DEFAULT);
    
    // Create label
    label_ = lv_label_create(button_);
    lv_label_set_text(label_, "BTN");
    lv_obj_set_style_text_color(label_, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_, FontA.small, LV_STATE_DEFAULT);
    lv_obj_center(label_);
    
    // Set up event callback
    lv_obj_add_event_cb(button_, button_event_cb, LV_EVENT_ALL, this);
}

void ButtonControl::bindParameter(std::shared_ptr<Parameter> parameter) {
    ParameterControl::bindParameter(parameter);
    
    if (parameter) {
        // Auto-configure based on parameter properties
        if (parameter->getName().find("Switch") != std::string::npos ||
            parameter->getName().find("On/Off") != std::string::npos ||
            parameter->getName().find("Enable") != std::string::npos) {
            setMode(ButtonMode::TOGGLE);
        }
        
        // Update text to parameter name
        setLabel(parameter->getShortName());
        
        // Sync initial state
        updateFromParameter();
    }
}

void ButtonControl::updateFromParameter() {
    if (!bound_parameter_) return;
    
    uint8_t value = bound_parameter_->getCurrentValue();
    
    switch (mode_) {
        case ButtonMode::TOGGLE:
            is_toggled_ = (value >= (off_value_ + on_value_) / 2);
            break;
        case ButtonMode::MOMENTARY:
            is_pressed_ = (value >= (off_value_ + on_value_) / 2);
            break;
        case ButtonMode::TRIGGER:
            // Trigger mode doesn't maintain state
            break;
    }
    
    updateVisualState();
}

void ButtonControl::setPosition(int x, int y) {
    if (button_) {
        lv_obj_set_pos(button_, x, y);
    }
}

void ButtonControl::getPosition(int& x, int& y) const {
    if (button_) {
        x = lv_obj_get_x(button_);
        y = lv_obj_get_y(button_);
    } else {
        x = y = 0;
    }
}

void ButtonControl::setSize(int width, int height) {
    if (button_) {
        lv_obj_set_size(button_, width, height);
    }
}

void ButtonControl::getSize(int& width, int& height) const {
    if (button_) {
        width = lv_obj_get_width(button_);
        height = lv_obj_get_height(button_);
    } else {
        width = height = 0;
    }
}

lv_obj_t* ButtonControl::getObject() {
    return button_;
}

void ButtonControl::updateDisplayFromParameter() {
    updateFromParameter();
}

void ButtonControl::updateParameterFromControl(uint8_t value) {
    sendParameterValue(value);
}

void ButtonControl::setMode(ButtonMode mode) {
    mode_ = mode;
    
    // Reset state when mode changes
    is_pressed_ = false;
    is_toggled_ = false;
    
    updateVisualState();
}

void ButtonControl::setLabel(const std::string& text) {
    if (label_) {
        lv_label_set_text(label_, text.c_str());
        lv_obj_set_style_text_font(label_, FontA.small, LV_STATE_DEFAULT);
    }
}

void ButtonControl::setColors(lv_color_t normal_color, lv_color_t pressed_color) {
    normal_color_ = normal_color;
    pressed_color_ = pressed_color;
    
    if (button_) {
        lv_obj_set_style_bg_color(button_, normal_color_, LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(button_, pressed_color_, LV_STATE_PRESSED);
    }
}

void ButtonControl::setToggleColors(lv_color_t off_color, lv_color_t on_color) {
    off_color_ = off_color;
    on_color_ = on_color;
    updateVisualState();
}

void ButtonControl::setMomentaryValues(uint8_t off_value, uint8_t on_value) {
    off_value_ = off_value;
    on_value_ = on_value;
}

void ButtonControl::setToggleValues(uint8_t off_value, uint8_t on_value) {
    off_value_ = off_value;
    on_value_ = on_value;
}

void ButtonControl::setTriggerValue(uint8_t trigger_value, uint8_t default_value) {
    trigger_value_ = trigger_value;
    default_value_ = default_value;
}

void ButtonControl::press() {
    if (!is_pressed_) {
        is_pressed_ = true;
        
        switch (mode_) {
            case ButtonMode::MOMENTARY:
                sendParameterValue(on_value_);
                break;
            case ButtonMode::TOGGLE:
                is_toggled_ = !is_toggled_;
                sendParameterValue(is_toggled_ ? on_value_ : off_value_);
                break;
            case ButtonMode::TRIGGER:
                sendParameterValue(trigger_value_);
                break;
        }
        
        updateVisualState();
    }
}

void ButtonControl::release() {
    if (is_pressed_) {
        is_pressed_ = false;
        
        switch (mode_) {
            case ButtonMode::MOMENTARY:
                sendParameterValue(off_value_);
                break;
            case ButtonMode::TOGGLE:
                // Toggle mode doesn't change on release
                break;
            case ButtonMode::TRIGGER:
                sendParameterValue(default_value_);
                break;
        }
        
        updateVisualState();
    }
}

void ButtonControl::toggle() {
    if (mode_ == ButtonMode::TOGGLE) {
        is_toggled_ = !is_toggled_;
        sendParameterValue(is_toggled_ ? on_value_ : off_value_);
        updateVisualState();
    }
}

void ButtonControl::trigger() {
    if (mode_ == ButtonMode::TRIGGER) {
        sendParameterValue(trigger_value_);
        // For now, immediately return to default value
        // TODO: Add proper timer support for trigger duration
        sendParameterValue(default_value_);
    }
}

void ButtonControl::updateVisualState() {
    if (!button_) return;
    
    lv_color_t bg_color;
    
    switch (mode_) {
        case ButtonMode::TOGGLE:
            bg_color = is_toggled_ ? on_color_ : off_color_;
            break;
        case ButtonMode::MOMENTARY:
            bg_color = is_pressed_ ? pressed_color_ : normal_color_;
            break;
        case ButtonMode::TRIGGER:
            bg_color = normal_color_;
            break;
    }
    
    lv_obj_set_style_bg_color(button_, bg_color, LV_STATE_DEFAULT);
}

void ButtonControl::sendParameterValue(uint8_t value) {
    if (bound_parameter_) {
        // Use command system for undo/redo support
        if (mode_ == ButtonMode::TOGGLE) {
            // For toggle buttons, use the specialized toggle command
            auto command = std::make_unique<ToggleParameterCommand>(bound_parameter_.get());
            // TODO: Get command manager from somewhere - we'll need to inject it
            // command_manager_->executeCommand(std::move(command));
            
            // For now, use direct setting
            bound_parameter_->setValue(value);
        } else {
            bound_parameter_->setValue(value);
        }
        
        // Call user callback
        if (value_changed_callback_) {
            value_changed_callback_(value, bound_parameter_.get());
        }
    }
}

void ButtonControl::onParameterChanged(const Parameter& parameter) {
    (void)parameter; // Suppress unused parameter warning
    updateFromParameter();
}

// Static event callback
void ButtonControl::button_event_cb(lv_event_t* e) {
    ButtonControl* self = static_cast<ButtonControl*>(lv_event_get_user_data(e));
    if (self) {
        self->handleButtonEvent(lv_event_get_code(e));
    }
}

void ButtonControl::handleButtonEvent(lv_event_code_t event_code) {
    switch (event_code) {
        case LV_EVENT_PRESSED:
            press();
            break;
        case LV_EVENT_RELEASED:
            release();
            break;
        case LV_EVENT_CLICKED:
            // Handle click for toggle mode
            if (mode_ == ButtonMode::TOGGLE) {
                // press() already handled the toggle
            } else if (mode_ == ButtonMode::TRIGGER) {
                trigger();
            }
            break;
        default:
            break;
    }
}
