#pragma once

#include "ParameterChangeEvent.h"

namespace Parameters {

/**
 * @brief Observer interface for parameter changes
 * 
 * Components that need to respond to parameter changes implement this interface.
 * Different observers handle different aspects (audio, UI, MIDI output, etc.)
 */
class ParameterObserver {
public:
    virtual ~ParameterObserver() = default;
    
    /**
     * @brief Called when a parameter changes
     * @param event The parameter change event
     * @note Implementation must be thread-safe and RT-safe if marked as RT observer
     */
    virtual void onParameterChanged(const ParameterChangeEvent& event) = 0;
    
    /**
     * @brief Check if this observer should be called from RT thread
     * @return true if observer is RT-safe and should be called immediately
     */
    virtual bool isRTSafe() const { return false; }
    
    /**
     * @brief Get parameters this observer is interested in
     * @return Vector of parameter IDs, or empty for all parameters
     */
    virtual std::vector<ParameterID> getInterestedParameters() const { return {}; }
};

/**
 * @brief RT-safe parameter observer for audio processing
 * 
 * These observers are called directly from the RT thread for immediate
 * response to parameter changes that affect audio.
 */
class RTParameterObserver : public ParameterObserver {
public:
    bool isRTSafe() const override { return true; }
    
    /**
     * @brief RT-safe parameter change handler
     * @param event Parameter change event
     * @note Must be RT-safe: no allocations, no blocking, minimal processing
     */
    void onParameterChanged(const ParameterChangeEvent& event) override = 0;
};

/**
 * @brief UI parameter observer for display updates
 * 
 * These observers are called from the UI thread (via event queue)
 * for updating visual elements and non-time-critical operations.
 */
class UIParameterObserver : public ParameterObserver {
public:
    bool isRTSafe() const override { return false; }
    
    /**
     * @brief UI parameter change handler
     * @param event Parameter change event
     * @note Called from UI thread - safe to update LVGL widgets
     */
    void onParameterChanged(const ParameterChangeEvent& event) override = 0;
};

} // namespace Parameters
