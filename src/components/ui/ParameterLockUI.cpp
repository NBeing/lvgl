#include "ParameterLockUI.h"
#include <iostream>

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
#include "debug/RTEventTracer.h"
#endif

namespace UI {

// Define quick parameters for common locks
const ParameterLockUI::QuickParameter ParameterLockUI::quick_parameters_[] = {
    {static_cast<ParameterID>(1), "Filter Cutoff", 0.0f, 127.0f, 64.0f},
    {static_cast<ParameterID>(2), "Resonance", 0.0f, 127.0f, 32.0f},
    {static_cast<ParameterID>(3), "LFO Rate", 0.0f, 127.0f, 64.0f},
    {static_cast<ParameterID>(4), "Env Attack", 0.0f, 127.0f, 10.0f},
    {static_cast<ParameterID>(5), "Env Decay", 0.0f, 127.0f, 40.0f},
    {static_cast<ParameterID>(6), "Delay Time", 0.0f, 127.0f, 32.0f},
    {static_cast<ParameterID>(7), "Reverb Level", 0.0f, 127.0f, 20.0f},
    {static_cast<ParameterID>(8), "Distortion", 0.0f, 127.0f, 0.0f}
};

const size_t ParameterLockUI::num_quick_parameters_ = sizeof(quick_parameters_) / sizeof(quick_parameters_[0]);

ParameterLockUI::ParameterLockUI() 
    : selected_track_(0), selected_step_(0), main_panel_(nullptr),
      step_info_label_(nullptr), parameter_list_(nullptr), lock_count_label_(nullptr) {
}

void ParameterLockUI::setStepSequencer(std::shared_ptr<MIDI::StepSequencer> sequencer) {
    sequencer_ = sequencer;
    
    #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
    TRACE_UI_EVENT("ParameterLockUI", "StepSequencer", "sequencerConnected", "ready");
    #endif
}

lv_obj_t* ParameterLockUI::createParameterLockPanel(lv_obj_t* parent) {
    // Create main panel
    main_panel_ = lv_obj_create(parent);
    lv_obj_set_size(main_panel_, 300, 400);
    lv_obj_set_style_bg_color(main_panel_, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_color(main_panel_, lv_color_hex(0x555555), 0);
    lv_obj_set_style_border_width(main_panel_, 2, 0);
    lv_obj_set_style_radius(main_panel_, 8, 0);
    
    // Title label
    lv_obj_t* title = lv_label_create(main_panel_);
    lv_label_set_text(title, "🔒 Parameter Locks");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(title, 10, 10);
    
    // Step info label
    step_info_label_ = lv_label_create(main_panel_);
    lv_label_set_text(step_info_label_, "Track 1, Step 1");
    lv_obj_set_style_text_color(step_info_label_, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_pos(step_info_label_, 10, 35);
    
    // Lock count label
    lock_count_label_ = lv_label_create(main_panel_);
    lv_label_set_text(lock_count_label_, "Locks: 0");
    lv_obj_set_style_text_color(lock_count_label_, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_pos(lock_count_label_, 200, 35);
    
    // Create scrollable parameter list
    parameter_list_ = lv_obj_create(main_panel_);
    lv_obj_set_size(parameter_list_, 280, 300);
    lv_obj_set_pos(parameter_list_, 10, 60);
    lv_obj_set_style_bg_color(parameter_list_, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(parameter_list_, 1, 0);
    lv_obj_set_style_radius(parameter_list_, 4, 0);
    lv_obj_set_scroll_dir(parameter_list_, LV_DIR_VER);
    
    // Create parameter lock buttons
    for (size_t i = 0; i < num_quick_parameters_; ++i) {
        createParameterLockButton(parameter_list_, quick_parameters_[i]);
    }
    
    // Clear all button
    lv_obj_t* clear_all_btn = lv_btn_create(main_panel_);
    lv_obj_set_size(clear_all_btn, 120, 30);
    lv_obj_set_pos(clear_all_btn, 10, 365);
    lv_obj_set_style_bg_color(clear_all_btn, lv_color_hex(0xFF4444), 0);
    
    lv_obj_t* clear_all_label = lv_label_create(clear_all_btn);
    lv_label_set_text(clear_all_label, "Clear All");
    lv_obj_center(clear_all_label);
    
    lv_obj_add_event_cb(clear_all_btn, onClearAllButtonClicked, LV_EVENT_CLICKED, this);
    
    updateParameterLockDisplay();
    
    return main_panel_;
}

void ParameterLockUI::createParameterLockButton(lv_obj_t* parent, const QuickParameter& param) {
    // Container for parameter controls
    lv_obj_t* param_container = lv_obj_create(parent);
    lv_obj_set_size(param_container, 260, 45);
    lv_obj_set_style_bg_color(param_container, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(param_container, 1, 0);
    lv_obj_set_style_radius(param_container, 4, 0);
    lv_obj_set_style_pad_all(param_container, 5, 0);
    
    // Parameter name label
    lv_obj_t* name_label = lv_label_create(param_container);
    lv_label_set_text(name_label, param.name);
    lv_obj_set_style_text_color(name_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(name_label, 5, 5);
    
    // Lock button
    lv_obj_t* lock_btn = lv_btn_create(param_container);
    lv_obj_set_size(lock_btn, 60, 25);
    lv_obj_set_pos(lock_btn, 120, 5);
    lv_obj_set_style_bg_color(lock_btn, lv_color_hex(0x4CAF50), 0);
    
    lv_obj_t* lock_label = lv_label_create(lock_btn);
    lv_label_set_text(lock_label, "Lock");
    lv_obj_center(lock_label);
    
    // Store parameter info in user data
    lv_obj_set_user_data(lock_btn, (void*)&param);
    lv_obj_add_event_cb(lock_btn, onParameterLockButtonClicked, LV_EVENT_CLICKED, this);
    
    // Clear button
    lv_obj_t* clear_btn = lv_btn_create(param_container);
    lv_obj_set_size(clear_btn, 50, 25);
    lv_obj_set_pos(clear_btn, 190, 5);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(0xF44336), 0);
    
    lv_obj_t* clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, "Clear");
    lv_obj_center(clear_label);
    
    lv_obj_set_user_data(clear_btn, (void*)&param);
    lv_obj_add_event_cb(clear_btn, onClearLockButtonClicked, LV_EVENT_CLICKED, this);
    
    // Value display label
    lv_obj_t* value_label = lv_label_create(param_container);
    lv_label_set_text(value_label, "--");
    lv_obj_set_style_text_color(value_label, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_pos(value_label, 5, 25);
}

void ParameterLockUI::selectStep(int track_id, int step_id) {
    if (track_id < 0 || track_id >= MIDI::StepSequencer::MAX_TRACKS ||
        step_id < 0 || step_id >= MIDI::StepSequencer::MAX_STEPS) {
        return;
    }
    
    selected_track_ = track_id;
    selected_step_ = step_id;
    
    refreshStepInfo();
    updateParameterLockDisplay();
    
    #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
    std::string step_data = "T" + std::to_string(track_id) + "S" + std::to_string(step_id);
    TRACE_UI_EVENT("User", "ParameterLockUI", "stepSelected", step_data.c_str());
    #endif
}

void ParameterLockUI::setParameterLock(ParameterID param_id, float value) {
    if (!sequencer_) return;
    
    sequencer_->setStepParameterLock(selected_track_, selected_step_, param_id, value);
    updateParameterLockDisplay();
    
    std::cout << "[ParameterLockUI] 🔒 Set lock T" << selected_track_ << "S" << selected_step_ 
              << " Param:" << static_cast<int>(param_id) << " = " << value << std::endl;
}

void ParameterLockUI::clearParameterLock(ParameterID param_id) {
    if (!sequencer_) return;
    
    sequencer_->clearStepParameterLock(selected_track_, selected_step_, param_id);
    updateParameterLockDisplay();
}

void ParameterLockUI::clearAllParameterLocks() {
    if (!sequencer_) return;
    
    sequencer_->clearAllStepParameterLocks(selected_track_, selected_step_);
    updateParameterLockDisplay();
    
    std::cout << "[ParameterLockUI] 🗑️ Cleared all locks for T" << selected_track_ 
              << "S" << selected_step_ << std::endl;
}

void ParameterLockUI::updateParameterLockDisplay() {
    if (!sequencer_ || !lock_count_label_) return;
    
    auto locked_params = sequencer_->getStepParameterLocks(selected_track_, selected_step_);
    
    // Update lock count
    std::string count_text = "Locks: " + std::to_string(locked_params.size());
    lv_label_set_text(lock_count_label_, count_text.c_str());
    
    // TODO: Update individual parameter value displays
}

void ParameterLockUI::refreshStepInfo() {
    if (!step_info_label_) return;
    
    std::string info_text = "Track " + std::to_string(selected_track_ + 1) + 
                           ", Step " + std::to_string(selected_step_ + 1);
    lv_label_set_text(step_info_label_, info_text.c_str());
}

// Static callback implementations
void ParameterLockUI::onParameterLockButtonClicked(lv_event_t* e) {
    auto* ui = static_cast<ParameterLockUI*>(lv_event_get_user_data(e));
    auto* btn = static_cast<lv_obj_t*>(lv_event_get_target(e));
    auto* param = static_cast<const QuickParameter*>(lv_obj_get_user_data(btn));
    
    if (ui && param) {
        // Use default value for now - in a full implementation, this would open a value editor
        ui->setParameterLock(param->id, param->default_value);
    }
}

void ParameterLockUI::onClearLockButtonClicked(lv_event_t* e) {
    auto* ui = static_cast<ParameterLockUI*>(lv_event_get_user_data(e));
    auto* btn = static_cast<lv_obj_t*>(lv_event_get_target(e));
    auto* param = static_cast<const QuickParameter*>(lv_obj_get_user_data(btn));
    
    if (ui && param) {
        ui->clearParameterLock(param->id);
    }
}

void ParameterLockUI::onClearAllButtonClicked(lv_event_t* e) {
    auto* ui = static_cast<ParameterLockUI*>(lv_event_get_user_data(e));
    
    if (ui) {
        ui->clearAllParameterLocks();
    }
}

std::string ParameterLockUI::formatParameterValue(float value, ParameterID param_id) const {
    // Simple formatting - could be enhanced per parameter type
    return std::to_string(static_cast<int>(value));
}

} // namespace UI
