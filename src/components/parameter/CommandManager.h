#pragma once

#include "Command.h"
#include <vector>
#include <memory>
#include <functional>

/**
 * Manages the execution and history of commands for undo/redo functionality.
 * Implements a dual-stack approach with smart command merging.
 */
class CommandManager {
public:
    using HistoryChangedCallback = std::function<void()>;
    
    CommandManager();
    ~CommandManager() = default;
    
    /**
     * Execute a command and add it to the undo stack
     */
    void executeCommand(std::unique_ptr<Command> command);
    
    /**
     * Undo the last command
     */
    bool undo();
    
    /**
     * Redo the last undone command
     */
    bool redo();
    
    /**
     * Check if undo is possible
     */
    bool canUndo() const { return !undo_stack_.empty(); }
    
    /**
     * Check if redo is possible
     */
    bool canRedo() const { return !redo_stack_.empty(); }
    
    /**
     * Get description of the next undo operation
     */
    std::string getUndoDescription() const;
    
    /**
     * Get description of the next redo operation
     */
    std::string getRedoDescription() const;
    
    /**
     * Clear all command history
     */
    void clearHistory();
    
    /**
     * Set the maximum number of commands to keep in history
     */
    void setMaxHistorySize(size_t max_size) { max_history_size_ = max_size; }
    
    /**
     * Get current history sizes
     */
    size_t getUndoStackSize() const { return undo_stack_.size(); }
    size_t getRedoStackSize() const { return redo_stack_.size(); }
    
    /**
     * Set callback for when history changes (for UI updates)
     */
    void setHistoryChangedCallback(HistoryChangedCallback callback) {
        history_changed_callback_ = callback;
    }
    
    /**
     * Enable/disable command merging
     */
    void setMergingEnabled(bool enabled) { merging_enabled_ = enabled; }
    
private:
    std::vector<std::unique_ptr<Command>> undo_stack_;
    std::vector<std::unique_ptr<Command>> redo_stack_;
    size_t max_history_size_;
    bool is_executing_;  // Prevent recursion during command execution
    bool merging_enabled_;
    HistoryChangedCallback history_changed_callback_;
    
    void trimHistory();
    void notifyHistoryChanged();
};
