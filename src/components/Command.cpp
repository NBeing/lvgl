#include "Command.h"
#include "Parameter.h"
#include "lvgl.h"
#include <iostream>

// Static method implementation
uint32_t Command::getCurrentTime() {
    return lv_tick_get();
}

// ============================================================================
// SetParameterCommand Implementation
// ============================================================================

SetParameterCommand::SetParameterCommand(Parameter* parameter, uint8_t new_value)
    : parameter_(parameter), new_value_(new_value) {
    if (!parameter_) {
        throw std::invalid_argument("Parameter cannot be null");
    }
    
    old_value_ = parameter_->getCurrentValue();
    parameter_name_ = parameter_->getName();
}

void SetParameterCommand::execute() {
    if (parameter_) {
        parameter_->setValueDirect(new_value_);
    }
}

void SetParameterCommand::undo() {
    if (parameter_) {
        parameter_->setValueDirect(old_value_);
    }
}

std::string SetParameterCommand::getDescription() const {
    return parameter_name_ + ": " + 
           std::to_string(old_value_) + " → " + std::to_string(new_value_);
}

bool SetParameterCommand::canMerge(const Command* other) const {
    // Check if it's the same command type without RTTI
    if (!other || other->getCommandType() != "SetParameterCommand") {
        return false;
    }
    
    // Safe to cast now since we know the type
    auto other_param = static_cast<const SetParameterCommand*>(other);
    if (!parameter_ || !other_param->parameter_) {
        return false;
    }
    
    // Same parameter and within time window
    bool same_parameter = (other_param->parameter_ == parameter_);
    bool within_time_window = (other->getTimestamp() - getTimestamp()) <= MERGE_WINDOW_MS;
    
    return same_parameter && within_time_window;
}

void SetParameterCommand::mergeWith(const Command* other) {
    auto other_param = static_cast<const SetParameterCommand*>(other);
    // Update target value but keep original old_value for proper undo
    new_value_ = other_param->new_value_;
}

// ============================================================================
// ToggleParameterCommand Implementation
// ============================================================================

ToggleParameterCommand::ToggleParameterCommand(Parameter* parameter)
    : parameter_(parameter) {
    if (!parameter_) {
        throw std::invalid_argument("Parameter cannot be null");
    }
    
    old_value_ = parameter_->getCurrentValue();
    new_value_ = (old_value_ == 0) ? 127 : 0;  // Toggle between 0 and 127
    parameter_name_ = parameter_->getName();
}

void ToggleParameterCommand::execute() {
    if (parameter_) {
        parameter_->setValueDirect(new_value_);
    }
}

void ToggleParameterCommand::undo() {
    if (parameter_) {
        parameter_->setValueDirect(old_value_);
    }
}

std::string ToggleParameterCommand::getDescription() const {
    return "Toggle " + parameter_name_ + ": " + 
           (old_value_ == 0 ? "OFF" : "ON") + " → " + 
           (new_value_ == 0 ? "OFF" : "ON");
}

bool ToggleParameterCommand::canMerge(const Command* other) const {
    (void)other; // Suppress unused parameter warning
    // Toggle commands should not merge - each toggle is significant
    return false;
}

// ============================================================================
// CompositeCommand Implementation
// ============================================================================

CompositeCommand::CompositeCommand(const std::string& description)
    : description_(description) {
}

void CompositeCommand::addCommand(std::unique_ptr<Command> command) {
    if (command) {
        sub_commands_.push_back(std::move(command));
    }
}

void CompositeCommand::execute() {
    for (auto& command : sub_commands_) {
        command->execute();
    }
}

void CompositeCommand::undo() {
    // Undo in reverse order
    for (auto it = sub_commands_.rbegin(); it != sub_commands_.rend(); ++it) {
        (*it)->undo();
    }
}

std::string CompositeCommand::getDescription() const {
    if (sub_commands_.empty()) {
        return description_;
    }
    
    return description_ + " (" + std::to_string(sub_commands_.size()) + " changes)";
}
