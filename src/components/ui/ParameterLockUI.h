#pragma once

#include "components/midi/StepSequencer.h"
#include "components/parameter/ParameterChangeEvent.h"
#include <lvgl.h>
#include <memory>

namespace UI {

// Use the Parameters namespace ParameterID
using ParameterID = Parameters::ParameterID;

/**
 * @brief UI helper for setting parameter locks on sequencer steps
 * 
 * Provides a simple interface for users to set parameter locks
 * on specific steps in the step sequencer.
 */
class ParameterLockUI {
public:
    ParameterLockUI();
    ~ParameterLockUI() = default;
    
    // Initialize with sequencer reference
    void setStepSequencer(std::shared_ptr<MIDI::StepSequencer> sequencer);
    
    // UI creation
    lv_obj_t* createParameterLockPanel(lv_obj_t* parent);
    
    // Step selection
    void selectStep(int track_id, int step_id);
    int getSelectedTrack() const { return selected_track_; }
    int getSelectedStep() const { return selected_step_; }
    
    // Parameter lock operations
    void setParameterLock(ParameterID param_id, float value);
    void clearParameterLock(ParameterID param_id);
    void clearAllParameterLocks();
    
    // UI updates
    void updateParameterLockDisplay();
    void refreshStepInfo();
    
    // Quick preset parameter locks
    void applyFilterSweepLock(float cutoff_start, float cutoff_end, int num_steps);
    void applyResonanceBuildLock(float res_start, float res_end, int num_steps);
    void applyEnvelopeVariationLock(int num_steps);
    
private:
    std::shared_ptr<MIDI::StepSequencer> sequencer_;
    
    // Current selection
    int selected_track_;
    int selected_step_;
    
    // UI elements
    lv_obj_t* main_panel_;
    lv_obj_t* step_info_label_;
    lv_obj_t* parameter_list_;
    lv_obj_t* lock_count_label_;
    
    // Common parameters for quick access
    struct QuickParameter {
        ParameterID id;
        const char* name;
        float min_value;
        float max_value;
        float default_value;
    };
    
    static const QuickParameter quick_parameters_[];
    static const size_t num_quick_parameters_;
    
    // UI callbacks
    static void onParameterLockButtonClicked(lv_event_t* e);
    static void onClearLockButtonClicked(lv_event_t* e);
    static void onClearAllButtonClicked(lv_event_t* e);
    
    // Helper methods
    void createParameterLockButton(lv_obj_t* parent, const QuickParameter& param);
    void updateLockCountDisplay();
    std::string formatParameterValue(float value, ParameterID param_id) const;
};

} // namespace UI
