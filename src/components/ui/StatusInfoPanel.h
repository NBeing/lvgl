#pragma once

#include <lvgl.h>
#include <string>

class StatusInfoPanel {
public:
    StatusInfoPanel();
    ~StatusInfoPanel() = default;

    // UI Creation
    void create(lv_obj_t* parent);
    void destroy();

    // Status updates
    void setStatusText(const std::string& text);
    void setParameterInfo(const std::string& param_name, const std::string& value, int cc_number);
    void setUndoRedoInfo(const std::string& undo_desc, const std::string& redo_desc);
    void setReadyMessage();

    // Getters
    lv_obj_t* getContainer() const { return container_; }
    bool isCreated() const { return container_ != nullptr; }

private:
    void styleContainer();

    lv_obj_t* container_;
    lv_obj_t* status_label_;
};
