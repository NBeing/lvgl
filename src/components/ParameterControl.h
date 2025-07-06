#pragma once

#include "Parameter.h"
#include <lvgl.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <string>

/**
 * @brief Abstract base class for parameter-aware UI controls
 * 
 * This class provides the interface between the Parameter model and UI controls.
 * It handles parameter binding, value synchronization, and observer pattern integration.
 */
class ParameterControl : public ParameterObserver, public std::enable_shared_from_this<ParameterControl> {
public:
    ParameterControl();
    virtual ~ParameterControl();
    
    // Parameter binding
    void bindParameter(std::shared_ptr<Parameter> parameter);
    void unbindParameter();
    std::shared_ptr<Parameter> getBoundParameter() const;
    bool isParameterBound() const;
    
    // Value management
    virtual void setControlValue(uint8_t value);
    virtual uint8_t getControlValue() const;
    
    // UI state management
    virtual void setEnabled(bool enabled);
    virtual bool isEnabled() const;
    virtual void setVisible(bool visible);
    virtual bool isVisible() const;
    
    // Position and sizing
    virtual void setPosition(int x, int y) = 0;
    virtual void getPosition(int& x, int& y) const = 0;
    virtual void setSize(int width, int height) = 0;
    virtual void getSize(int& width, int& height) const = 0;
    
    // LVGL object access
    virtual lv_obj_t* getObject() = 0;
    
    // Callbacks
    using ValueChangedCallback = std::function<void(uint8_t value, const Parameter* parameter)>;
    void setValueChangedCallback(ValueChangedCallback callback);
    
    // ParameterObserver implementation
    void onParameterChanged(const Parameter& parameter) override;
    
protected:
    // Pure virtual methods for subclasses to implement
    virtual void updateDisplayFromParameter() = 0;
    virtual void updateParameterFromControl(uint8_t value) = 0;
    virtual void onParameterBound() {}
    virtual void onParameterUnbound() {}
    virtual void onEnabledChanged(bool /* enabled */) {}
    virtual void onVisibilityChanged(bool /* visible */) {}
    
    // Helper methods for subclasses
    void notifyValueChanged(uint8_t value);
    bool isUpdatingFromParameter() const { return updating_from_parameter_; }
    
private:
    std::shared_ptr<Parameter> bound_parameter_;
    ValueChangedCallback value_changed_callback_;
    bool enabled_;
    bool visible_;
    bool updating_from_parameter_;  // Prevents feedback loops
};

/**
 * @brief Parameter-aware dial control implementation
 * 
 * This class wraps the existing MidiDial functionality with parameter awareness.
 * It automatically displays parameter names, handles value scaling, and integrates
 * with the observer pattern for synchronized updates.
 */
class DialControl : public ParameterControl {
public:
    DialControl(lv_obj_t* parent, int x = 0, int y = 0);
    ~DialControl();
    
    // ParameterControl implementation
    void setPosition(int x, int y) override;
    void getPosition(int& x, int& y) const override;
    void setSize(int width, int height) override;
    void getSize(int& width, int& height) const override;
    lv_obj_t* getObject() override;
    
    // Dial-specific customization
    void setColor(lv_color_t color);
    lv_color_t getColor() const;
    void setDialSize(int diameter);
    int getDialSize() const;
    
    // Manual label override (if not using parameter name)
    void setLabel(const std::string& label);
    std::string getLabel() const;
    
protected:
    // ParameterControl implementation
    void updateDisplayFromParameter() override;
    void updateParameterFromControl(uint8_t value) override;
    void onParameterBound() override;
    void onParameterUnbound() override;
    void onEnabledChanged(bool enabled) override;
    void onVisibilityChanged(bool visible) override;
    
private:
    // LVGL components
    lv_obj_t* container_;
    lv_obj_t* name_label_;
    lv_obj_t* value_label_;
    lv_obj_t* arc_display_;
    
    // Properties
    lv_color_t arc_color_;
    int dial_diameter_;
    std::string custom_label_;
    bool use_custom_label_;
    
    // Internal state
    uint8_t display_value_;
    
    // Setup and styling
    void createWidgets(lv_obj_t* parent, int x, int y);
    void setupStyling();
    void updateLabels();
    void updateArcDisplay();
    
    // Event handlers
    static void arc_event_cb(lv_event_t* e);
    static void container_click_cb(lv_event_t* e);
    
    // Static mapping for event callbacks
    static std::unordered_map<lv_obj_t*, DialControl*> dial_control_map_;
};
