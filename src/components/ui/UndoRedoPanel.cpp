#include "UndoRedoPanel.h"
#include "components/parameter/CommandManager.h"
#include "components/layout/LayoutManager.h"
#include "Constants.h"
#include <iostream>

UndoRedoPanel::UndoRedoPanel(CommandManager* command_manager)
    : command_manager_(command_manager)
    , container_(nullptr)
    , undo_btn_(nullptr)
    , redo_btn_(nullptr)
    , undo_label_(nullptr)
    , redo_label_(nullptr)
{
}

void UndoRedoPanel::create(lv_obj_t* parent) {
    if (container_) return; // Already created

    // Create container for undo/redo buttons
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 0, 0);

    // Set up flex layout for buttons
    lv_obj_set_layout(container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(container_, 10, 0);

    createUndoButton();
    createRedoButton();
    update();

    std::cout << "UndoRedoPanel created" << std::endl;
}

void UndoRedoPanel::destroy() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
        undo_btn_ = nullptr;
        redo_btn_ = nullptr;
        undo_label_ = nullptr;
        redo_label_ = nullptr;
    }
}

void UndoRedoPanel::createUndoButton() {
    const auto& layout = LayoutManager::getConfig();

    undo_btn_ = lv_btn_create(container_);
    lv_obj_set_size(undo_btn_, layout.button_width, layout.button_height);
    
    // Style the undo button
    lv_obj_set_style_bg_color(undo_btn_, lv_color_hex(SynthConstants::Color::BTN_UNDO_BG), 0);
    lv_obj_set_style_border_color(undo_btn_, lv_color_hex(SynthConstants::Color::BTN_UNDO_BORDER), 0);
    lv_obj_set_style_border_width(undo_btn_, 2, 0);
    lv_obj_set_style_radius(undo_btn_, 4, 0);
    
    undo_label_ = lv_label_create(undo_btn_);
    lv_label_set_text(undo_label_, "UNDO");
    lv_obj_center(undo_label_);
    
    // Add event callback
    lv_obj_add_event_cb(undo_btn_, onUndoClicked, LV_EVENT_CLICKED, this);
}

void UndoRedoPanel::createRedoButton() {
    const auto& layout = LayoutManager::getConfig();

    redo_btn_ = lv_btn_create(container_);
    lv_obj_set_size(redo_btn_, layout.button_width, layout.button_height);
    
    // Style the redo button
    lv_obj_set_style_bg_color(redo_btn_, lv_color_hex(SynthConstants::Color::BTN_REDO_BG), 0);
    lv_obj_set_style_border_color(redo_btn_, lv_color_hex(SynthConstants::Color::BTN_REDO_BORDER), 0);
    lv_obj_set_style_border_width(redo_btn_, 2, 0);
    lv_obj_set_style_radius(redo_btn_, 4, 0);
    
    redo_label_ = lv_label_create(redo_btn_);
    lv_label_set_text(redo_label_, "REDO");
    lv_obj_center(redo_label_);
    
    // Add event callback
    lv_obj_add_event_cb(redo_btn_, onRedoClicked, LV_EVENT_CLICKED, this);
}

void UndoRedoPanel::update() {
    if (!command_manager_ || !undo_btn_ || !redo_btn_) return;

    updateButtonStates();
}

void UndoRedoPanel::updateButtonStates() {
    bool can_undo = command_manager_->canUndo();
    bool can_redo = command_manager_->canRedo();

    // Update undo button
    if (undo_label_) {
        lv_label_set_text(undo_label_, can_undo ? "UNDO" : "UNDO-");
    }
    styleButton(undo_btn_, can_undo, 0x005500, SynthConstants::Color::BTN_UNDO_BG);

    // Update redo button
    if (redo_label_) {
        lv_label_set_text(redo_label_, can_redo ? "REDO" : "REDO-");
    }
    styleButton(redo_btn_, can_redo, 0x550000, SynthConstants::Color::BTN_REDO_BG);
}

void UndoRedoPanel::styleButton(lv_obj_t* button, bool active, uint32_t active_color, uint32_t inactive_color) {
    if (!button) return;
    
    lv_obj_set_style_bg_color(button, 
        active ? lv_color_hex(active_color) : lv_color_hex(inactive_color), 
        LV_STATE_DEFAULT);
}

void UndoRedoPanel::onUndoClicked(lv_event_t* e) {
    UndoRedoPanel* panel = static_cast<UndoRedoPanel*>(lv_event_get_user_data(e));
    if (panel && panel->undo_callback_) {
        panel->undo_callback_();
    }
}

void UndoRedoPanel::onRedoClicked(lv_event_t* e) {
    UndoRedoPanel* panel = static_cast<UndoRedoPanel*>(lv_event_get_user_data(e));
    if (panel && panel->redo_callback_) {
        panel->redo_callback_();
    }
}
