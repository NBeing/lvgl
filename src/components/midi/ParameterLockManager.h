#pragma once

#include "components/parameter/ParameterChangeEvent.h"
#include "components/parameter/ParameterManager.h"
#include "components/threading/ThreadSafeSubject.h"
#include <unordered_map>
#include <vector>
#include <memory>

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
#include "debug/RTEventTracer.h"
#endif

namespace MIDI {

// Use the Parameters namespace ParameterID
using ParameterID = Parameters::ParameterID;

/**
 * @brief Manages parameter locks for step sequencer
 * 
 * Handles per-step parameter automation by applying locked parameter
 * values when steps are triggered and restoring previous values.
 */
class ParameterLockManager {
public:
    struct ParameterLockEvent {
        ParameterID parameter_id;
        float locked_value;
        float previous_value;
        int step_id;
        int track_id;
        
        ParameterLockEvent(ParameterID id, float locked, float previous, int step, int track)
            : parameter_id(id), locked_value(locked), previous_value(previous), 
              step_id(step), track_id(track) {}
    };
    
    ParameterLockManager();
    ~ParameterLockManager() = default;
    
    // Core parameter lock functionality
    void applyStepParameterLocks(int track_id, int step_id, 
                                const std::unordered_map<ParameterID, float>& locks);
    void restoreParametersFromStep(int track_id, int step_id);
    void clearAllParameterLocks();
    
    // Parameter lock management
    void setStepParameterLock(int track_id, int step_id, ParameterID param_id, float value);
    void clearStepParameterLock(int track_id, int step_id, ParameterID param_id);
    bool hasStepParameterLock(int track_id, int step_id, ParameterID param_id) const;
    float getStepParameterLock(int track_id, int step_id, ParameterID param_id) const;
    
    // Bulk operations
    void copyStepLocks(int src_track, int src_step, int dest_track, int dest_step);
    void clearStepLocks(int track_id, int step_id);
    std::vector<ParameterID> getLockedParametersForStep(int track_id, int step_id) const;
    
    // Statistics and debugging
    size_t getTotalParameterLocks() const;
    size_t getParameterLocksForTrack(int track_id) const;
    void printParameterLockStatistics() const;
    
    // Integration with ParameterManager
    void setParameterManager(std::shared_ptr<Parameters::ParameterManager> param_manager);
    
private:
    // Storage: [track_id][step_id] -> map<param_id, locked_value>
    std::unordered_map<int, std::unordered_map<int, std::unordered_map<ParameterID, float>>> step_locks_;
    
    // Track parameter values that were overridden (for restoration)
    std::unordered_map<ParameterID, float> saved_parameter_values_;
    
    // Currently active step locks (for restoration when step ends)
    std::unordered_map<int, std::unordered_map<int, std::vector<ParameterID>>> active_step_locks_;
    
    std::shared_ptr<Parameters::ParameterManager> parameter_manager_;
    
    // Helper methods
    std::string getTrackStepKey(int track_id, int step_id) const;
    void saveParameterValue(ParameterID param_id);
    void restoreParameterValue(ParameterID param_id);
    void traceParameterLockEvent(const std::string& action, int track_id, int step_id, 
                                ParameterID param_id, float value) const;
};

} // namespace MIDI
