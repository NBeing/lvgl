#pragma once

#include <lvgl.h>
#include <functional>
#include <memory>

class CommandManager;

class UndoRedoPanel {
public:
    using UndoRedoCallback = std::function<void()>;

    explicit UndoRedoPanel(CommandManager* command_manager);
    ~UndoRedoPanel() = default;

    // UI Creation
    void create(lv_obj_t* parent);
    void destroy();

    // Event callbacks
    void setUndoCallback(UndoRedoCallback callback) { undo_callback_ = callback; }
    void setRedoCallback(UndoRedoCallback callback) { redo_callback_ = callback; }

    // State updates
    void update();

    // Getters
    lv_obj_t* getContainer() const { return container_; }
    bool isCreated() const { return container_ != nullptr; }

private:
    void createUndoButton();
    void createRedoButton();
    void updateButtonStates();
    void styleButton(lv_obj_t* button, bool active, uint32_t active_color, uint32_t inactive_color);

    // Static event handlers
    static void onUndoClicked(lv_event_t* e);
    static void onRedoClicked(lv_event_t* e);

    CommandManager* command_manager_;
    lv_obj_t* container_;
    lv_obj_t* undo_btn_;
    lv_obj_t* redo_btn_;
    lv_obj_t* undo_label_;
    lv_obj_t* redo_label_;

    UndoRedoCallback undo_callback_;
    UndoRedoCallback redo_callback_;
};
