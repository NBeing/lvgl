#pragma once

#include "components/controls/ParameterControl.h"
#include <lvgl.h>
#include <memory>
#include <functional>

/**
 * Button control that integrates with the parameter system.
 * Supports both momentary and toggle modes.
 */
class ButtonControl : public ParameterControl {
public:
    enum class ButtonMode {
        MOMENTARY,  // Button sends value while pressed, returns to default when released
        TOGGLE,     // Button toggles between two values
        TRIGGER     // Button sends a single trigger value then returns to default
    };
    
    ButtonControl(lv_obj_t* parent, int x, int y, int width = 80, int height = 40);
    virtual ~ButtonControl();
    
    // ParameterControl overrides
    void setPosition(int x, int y) override;
    void getPosition(int& x, int& y) const override;
    void setSize(int width, int height) override;
    void getSize(int& width, int& height) const override;
    lv_obj_t* getObject() override;
    
    // ButtonControl-specific methods
    void bindParameter(std::shared_ptr<Parameter> parameter);
    void updateFromParameter();
    
    // Button-specific configuration
    void setMode(ButtonMode mode);
    ButtonMode getMode() const { return mode_; }
    
    void setLabel(const std::string& text);
    void setColors(lv_color_t normal_color, lv_color_t pressed_color);
    void setToggleColors(lv_color_t off_color, lv_color_t on_color);
    
    // Value configuration for different modes
    void setMomentaryValues(uint8_t off_value, uint8_t on_value);
    void setToggleValues(uint8_t off_value, uint8_t on_value);
    void setTriggerValue(uint8_t trigger_value, uint8_t default_value = 0);
    
    // State queries
    bool isPressed() const { return is_pressed_; }
    bool isToggled() const { return is_toggled_; }
    
    // Manual control (useful for testing)
    void press();
    void release();
    void toggle();
    void trigger();
    
protected:
    void updateDisplayFromParameter() override;
    void updateParameterFromControl(uint8_t value) override;
    void onParameterChanged(const Parameter& parameter) override;
    
    // Button-specific configuration
    lv_obj_t* button_;
    lv_obj_t* label_;
    ButtonMode mode_;
    bool is_pressed_;
    bool is_toggled_;
    
    // Visual configuration
    lv_color_t normal_color_;
    lv_color_t pressed_color_;
    lv_color_t off_color_;
    lv_color_t on_color_;
    
    // Value configuration
    uint8_t off_value_;
    uint8_t on_value_;
    uint8_t trigger_value_;
    uint8_t default_value_;
    
    void createButton(lv_obj_t* parent, int x, int y, int width, int height);
    void updateVisualState();
    void sendParameterValue(uint8_t value);
    
    // Event handlers
    static void button_event_cb(lv_event_t* e);
    void handleButtonEvent(lv_event_code_t event_code);
    
private:
};
