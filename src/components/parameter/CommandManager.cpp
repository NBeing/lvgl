#include "CommandManager.h"
#include <iostream>

CommandManager::CommandManager()
    : max_history_size_(100), is_executing_(false), merging_enabled_(true) {
    
#if defined(ESP32_BUILD)
    // Smaller history for embedded systems
    max_history_size_ = 50;
#endif
}

void CommandManager::executeCommand(std::unique_ptr<Command> command) {
    if (!command || is_executing_) {
        return;  // Prevent null commands and recursion
    }
    
    // Try to merge with the last command if merging is enabled
    if (merging_enabled_ && !undo_stack_.empty()) {
        auto& last_command = undo_stack_.back();
        if (last_command->canMerge(command.get())) {
            std::cout << "Merging command: " << command->getDescription() << std::endl;
            last_command->mergeWith(command.get());
            // Execute the merged command to apply the new value
            is_executing_ = true;
            last_command->execute();
            is_executing_ = false;
            notifyHistoryChanged();
            return;
        }
    }
    
    // Execute the command
    is_executing_ = true;
    try {
        command->execute();
        std::cout << "Executed command: " << command->getDescription() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Command execution failed: " << e.what() << std::endl;
        is_executing_ = false;
        return;
    }
    is_executing_ = false;
    
    // Add to undo stack
    undo_stack_.push_back(std::move(command));
    
    // Clear redo stack (new action invalidates redo)
    redo_stack_.clear();
    
    // Maintain size limit
    trimHistory();
    
    notifyHistoryChanged();
}

bool CommandManager::undo() {
    if (!canUndo() || is_executing_) {
        return false;
    }
    
    is_executing_ = true;
    auto command = std::move(undo_stack_.back());
    undo_stack_.pop_back();
    
    try {
        command->undo();
        std::cout << "Undid command: " << command->getDescription() << std::endl;
        redo_stack_.push_back(std::move(command));
    } catch (const std::exception& e) {
        std::cerr << "Undo failed: " << e.what() << std::endl;
        // Put command back in undo stack
        undo_stack_.push_back(std::move(command));
        is_executing_ = false;
        return false;
    }
    
    is_executing_ = false;
    notifyHistoryChanged();
    return true;
}

bool CommandManager::redo() {
    if (!canRedo() || is_executing_) {
        return false;
    }
    
    is_executing_ = true;
    auto command = std::move(redo_stack_.back());
    redo_stack_.pop_back();
    
    try {
        command->execute();
        std::cout << "Redid command: " << command->getDescription() << std::endl;
        undo_stack_.push_back(std::move(command));
    } catch (const std::exception& e) {
        std::cerr << "Redo failed: " << e.what() << std::endl;
        // Put command back in redo stack
        redo_stack_.push_back(std::move(command));
        is_executing_ = false;
        return false;
    }
    
    is_executing_ = false;
    notifyHistoryChanged();
    return true;
}

std::string CommandManager::getUndoDescription() const {
    if (canUndo()) {
        return "Undo: " + undo_stack_.back()->getDescription();
    }
    return "No undo available";
}

std::string CommandManager::getRedoDescription() const {
    if (canRedo()) {
        return "Redo: " + redo_stack_.back()->getDescription();
    }
    return "No redo available";
}

void CommandManager::clearHistory() {
    undo_stack_.clear();
    redo_stack_.clear();
    notifyHistoryChanged();
}

void CommandManager::trimHistory() {
    while (undo_stack_.size() > max_history_size_) {
        undo_stack_.erase(undo_stack_.begin());
    }
    
    // Also limit redo stack
    while (redo_stack_.size() > max_history_size_) {
        redo_stack_.erase(redo_stack_.begin());
    }
}

void CommandManager::notifyHistoryChanged() {
    if (history_changed_callback_) {
        history_changed_callback_();
    }
}
