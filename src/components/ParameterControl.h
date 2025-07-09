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
    
    // Protected members for subclasses
    std::shared_ptr<Parameter> bound_parameter_;
    ValueChangedCallback value_changed_callback_;
    
private:
    bool enabled_;
    bool visible_;
    bool updating_from_parameter_;  // Prevents feedback loops
};
