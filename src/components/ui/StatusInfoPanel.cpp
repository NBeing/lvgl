#include "StatusInfoPanel.h"
#include "Constants.h"
#include <iostream>

StatusInfoPanel::StatusInfoPanel()
    : container_(nullptr)
    , status_label_(nullptr)
{
}

void StatusInfoPanel::create(lv_obj_t* parent) {
    if (container_) return; // Already created

    // Create container for status info
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_SIZE_CONTENT);
    styleContainer();

    // Create status label
    status_label_ = lv_label_create(container_);
    lv_label_set_text(status_label_, SynthConstants::Text::STATUS_READY);
    lv_obj_set_style_text_color(status_label_, lv_color_hex(SynthConstants::Color::STATUS), 0);
    lv_obj_align(status_label_, LV_ALIGN_LEFT_MID, 10, 0);

    std::cout << "StatusInfoPanel created" << std::endl;
}

void StatusInfoPanel::destroy() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
        status_label_ = nullptr;
    }
}

void StatusInfoPanel::styleContainer() {
    lv_obj_set_style_bg_color(container_, lv_color_hex(SynthConstants::Color::STATUS_BG), 0);
    lv_obj_set_style_border_color(container_, lv_color_hex(SynthConstants::Color::STATUS_BORDER), 0);
    lv_obj_set_style_border_width(container_, 1, 0);
    lv_obj_set_style_radius(container_, 4, 0);
    lv_obj_set_style_pad_all(container_, 8, 0);
}

void StatusInfoPanel::setStatusText(const std::string& text) {
    if (status_label_) {
        lv_label_set_text(status_label_, text.c_str());
    }
}

void StatusInfoPanel::setParameterInfo(const std::string& param_name, const std::string& value, int cc_number) {
    char status_text[256];
    snprintf(status_text, sizeof(status_text), "%s = %s (CC%d)", 
             param_name.c_str(), value.c_str(), cc_number);
    setStatusText(status_text);
}

void StatusInfoPanel::setUndoRedoInfo(const std::string& undo_desc, const std::string& redo_desc) {
    char status_text[256];
    snprintf(status_text, sizeof(status_text), "U:%s | R:%s", 
             undo_desc.c_str(), redo_desc.c_str());
    setStatusText(status_text);
}

void StatusInfoPanel::setReadyMessage() {
    setStatusText("Ready - Parameters loaded");
}
