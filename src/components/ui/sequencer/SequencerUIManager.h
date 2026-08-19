#pragma once

#include "components/parameter/ParameterManager.h"
#include <memory>

namespace UI {

/**
 * @brief Mock sequencer UI manager for demonstration
 * 
 * In a real implementation, this would be the main UI controller
 * that manages the sequencer interface, track focus, pattern switching, etc.
 */
class SequencerUIManager {
private:
    int current_track_{0};
    int current_step_{0};
    int current_pattern_{0};
    
public:
    SequencerUIManager(void* parent, 
                      std::shared_ptr<class StepSequencer> sequencer,
                      std::shared_ptr<class ParameterLockManager> lock_manager,
                      std::shared_ptr<Parameters::ParameterManager> param_manager) {
        // Constructor for compatibility with example
        (void)parent; (void)sequencer; (void)lock_manager; (void)param_manager;
    }
    
    /**
     * @brief Set UI focus to specific track and step
     */
    void setFocus(int track, int step) {
        current_track_ = track;
        current_step_ = step;
        // In real implementation, would update UI highlighting
    }
    
    /**
     * @brief Switch to different pattern
     */
    void switchToPattern(int pattern) {
        current_pattern_ = pattern;
        // In real implementation, would change UI to show different pattern
    }
    
    /**
     * @brief Get current track focus
     */
    int getCurrentTrack() const { return current_track_; }
    
    /**
     * @brief Get current step focus
     */
    int getCurrentStep() const { return current_step_; }
    
    /**
     * @brief Get current pattern
     */
    int getCurrentPattern() const { return current_pattern_; }
    
    /**
     * @brief Quick save current state
     */
    void quickSave() {
        // Implementation would save current sequencer state
    }
    
    /**
     * @brief Quick load last saved state
     */
    void quickLoad() {
        // Implementation would load last saved state
    }
    
    /**
     * @brief Set zoom level
     */
    void setZoomLevel(float zoom) {
        // Implementation would adjust UI zoom
        (void)zoom;
    }
};

} // namespace UI
