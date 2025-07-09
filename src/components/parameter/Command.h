#pragma once

#include <string>
#include <memory>
#include <vector>

/**
 * Base interface for all reversible commands in the parameter system.
 * Implements the Command Pattern for undo/redo functionality.
 */
class Command {
public:
    virtual ~Command() = default;
    
    /**
     * Execute the command (apply the change)
     */
    virtual void execute() = 0;
    
    /**
     * Undo the command (reverse the change)
     */
    virtual void undo() = 0;
    
    /**
     * Get a human-readable description of what this command does
     */
    virtual std::string getDescription() const = 0;
    
    /**
     * Get the command type for safe casting (replaces RTTI)
     */
    virtual std::string getCommandType() const = 0;
    
    /**
     * Check if this command can be merged with another command
     * (useful for rapid parameter changes to avoid flooding history)
     */
    virtual bool canMerge(const Command* other) const { (void)other; return false; }
    
    /**
     * Merge this command with another compatible command
     */
    virtual void mergeWith(const Command* other) { (void)other; }
    
    /**
     * Get the timestamp when this command was created
     */
    virtual uint32_t getTimestamp() const { return timestamp_; }

protected:
    Command() : timestamp_(getCurrentTime()) {}
    
private:
    uint32_t timestamp_;
    
    static uint32_t getCurrentTime();
};

// Forward declarations
class Parameter;

/**
 * Command for changing a parameter value
 */
class SetParameterCommand : public Command {
public:
    SetParameterCommand(Parameter* parameter, uint8_t new_value);
    
    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    std::string getCommandType() const override { return "SetParameterCommand"; }
    bool canMerge(const Command* other) const override;
    void mergeWith(const Command* other) override;

private:
    Parameter* parameter_;
    uint8_t old_value_;
    uint8_t new_value_;
    std::string parameter_name_;  // Cache for description
    
    static constexpr uint32_t MERGE_WINDOW_MS = 500;  // 500ms window for merging
};

/**
 * Command for toggling a boolean parameter (buttons, switches)
 */
class ToggleParameterCommand : public Command {
public:
    ToggleParameterCommand(Parameter* parameter);
    
    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    std::string getCommandType() const override { return "ToggleParameterCommand"; }
    bool canMerge(const Command* other) const override;

private:
    Parameter* parameter_;
    uint8_t old_value_;
    uint8_t new_value_;
    std::string parameter_name_;
};

/**
 * Composite command that groups multiple commands together
 * Useful for complex operations like loading presets
 */
class CompositeCommand : public Command {
public:
    CompositeCommand(const std::string& description);
    
    void addCommand(std::unique_ptr<Command> command);
    
    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    std::string getCommandType() const override { return "CompositeCommand"; }
    
    size_t getCommandCount() const { return sub_commands_.size(); }

private:
    std::vector<std::unique_ptr<Command>> sub_commands_;
    std::string description_;
};
