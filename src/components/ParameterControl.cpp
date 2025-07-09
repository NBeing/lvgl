
#include "ParameterControl.h"
#include <unordered_map>
#include <iostream>
#include "../../include/FontConfig.h"

// ============================================================================
// ParameterControl Base Class Implementation
// ============================================================================

ParameterControl::ParameterControl()
    : bound_parameter_(nullptr)
    , enabled_(true)
    , visible_(true)
    , updating_from_parameter_(false)
{
}

ParameterControl::~ParameterControl() {
    unbindParameter();
}

void ParameterControl::bindParameter(std::shared_ptr<Parameter> parameter) {
    if (bound_parameter_ == parameter) return;
    
    // Unbind current parameter if any
    unbindParameter();
    
    // Bind new parameter
    bound_parameter_ = parameter;
    if (bound_parameter_) {
        bound_parameter_->addObserver(shared_from_this());
        updateDisplayFromParameter();
        onParameterBound();
    }
}

void ParameterControl::unbindParameter() {
    if (bound_parameter_) {
        bound_parameter_->removeObserver(shared_from_this());
        bound_parameter_.reset();
        onParameterUnbound();
    }
}

std::shared_ptr<Parameter> ParameterControl::getBoundParameter() const {
    return bound_parameter_;
}

bool ParameterControl::isParameterBound() const {
    return bound_parameter_ != nullptr;
}

void ParameterControl::setControlValue(uint8_t value) {
    if (bound_parameter_) {
        // Update parameter, which will trigger observer notification
        updating_from_parameter_ = false;
        updateParameterFromControl(value);
    }
}

uint8_t ParameterControl::getControlValue() const {
    if (bound_parameter_) {
        return bound_parameter_->getCurrentValue();
    }
    return 0;
}

void ParameterControl::setEnabled(bool enabled) {
    if (enabled_ != enabled) {
        enabled_ = enabled;
        onEnabledChanged(enabled);
    }
}

bool ParameterControl::isEnabled() const {
    return enabled_;
}

void ParameterControl::setVisible(bool visible) {
    if (visible_ != visible) {
        visible_ = visible;
        onVisibilityChanged(visible);
    }
}

bool ParameterControl::isVisible() const {
    return visible_;
}

void ParameterControl::setValueChangedCallback(ValueChangedCallback callback) {
    value_changed_callback_ = callback;
}

void ParameterControl::onParameterChanged(const Parameter& /* parameter */) {
    // This is called when the parameter value changes from external sources
    updating_from_parameter_ = true;
    updateDisplayFromParameter();
    updating_from_parameter_ = false;
}

void ParameterControl::notifyValueChanged(uint8_t value) {
    if (value_changed_callback_ && bound_parameter_) {
        value_changed_callback_(value, bound_parameter_.get());
    }
}
