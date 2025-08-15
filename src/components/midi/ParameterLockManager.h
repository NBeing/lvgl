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
 * @brief Manages parameter locks for step sequencer (Digitakt-style)
 * 
 * This class provides the core functionality for parameter locks in a step sequencer,
 * allowing each step to override any number of parameters with custom values.
 * When a step is triggered, its parameter locks are applied, and when the step ends,
 * the original parameter values are restored.
 * 
 * **Core Concept:**
 * - Each step can "lock" parameters to specific values
 * - When step plays: apply all locked parameters
 * - When step ends: restore original parameter values
 * - Multiple tracks with independent parameter locks
 * 
 * **Thread Safety:**
 * - Uses ParameterManager for thread-safe parameter updates
 * - Lock storage is protected for concurrent access
 * - RT-safe parameter application during sequencer playback
 * 
 * **Dependencies:**
 * - `ParameterManager`: For thread-safe parameter updates
 * - `ParameterChangeEvent`: For parameter event structures
 * - `ThreadSafeSubject`: For event notifications
 * 
 * **Usage Example:**
 * ```cpp
 * ParameterLockManager lockMgr;
 * lockMgr.setStepParameterLock(track=0, step=3, ParameterID::FILTER_CUTOFF, 0.8f);
 * lockMgr.applyStepParameterLocks(track=0, step=3, locks);  // On step trigger
 * lockMgr.restoreParametersFromStep(track=0, step=3);      // On step end
 * ```
 * 
 * @see ParameterManager For parameter value management
 * @see StepSequencer For sequencer integration
 */
class ParameterLockManager {
public:
    /**
     * @brief Event data for parameter lock operations
     * 
     * Contains all information about a parameter lock being applied or removed,
     * useful for debugging and event tracing.
     */
    struct ParameterLockEvent {
        ParameterID parameter_id;    ///< Which parameter is being locked
        float locked_value;          ///< The locked value being applied
        float previous_value;        ///< Original value before lock
        int step_id;                ///< Step number (0-based)
        int track_id;               ///< Track number (0-based)
        
        /**
         * @brief Construct a parameter lock event
         * @param id Parameter identifier
         * @param locked Value to lock parameter to
         * @param previous Original parameter value
         * @param step Step number where lock is applied
         * @param track Track number containing the step
         */
        ParameterLockEvent(ParameterID id, float locked, float previous, int step, int track)
            : parameter_id(id), locked_value(locked), previous_value(previous), 
              step_id(step), track_id(track) {}
    };
    
    /**
     * @brief Constructor - initializes empty parameter lock manager
     * @note Must call setParameterManager() before using lock functionality
     */
    ParameterLockManager();
    
    /**
     * @brief Destructor - cleans up all parameter locks
     */
    /**
     * @brief Destructor - cleans up all parameter locks
     */
    ~ParameterLockManager() = default;
    
    // =============================================================================
    // CORE PARAMETER LOCK FUNCTIONALITY
    // =============================================================================
    
    /**
     * @brief Apply all parameter locks for a specific step (called when step triggers)
     * @param track_id Track number (0-based)
     * @param step_id Step number (0-based)
     * @param locks Map of parameter IDs to locked values
     * @note Thread-safe, can be called from sequencer thread
     * @note Saves original values for later restoration
     * @see restoreParametersFromStep To restore original values
     */
    void applyStepParameterLocks(int track_id, int step_id, 
                                const std::unordered_map<ParameterID, float>& locks);
    
    /**
     * @brief Restore original parameter values for a step (called when step ends)
     * @param track_id Track number (0-based)
     * @param step_id Step number (0-based)
     * @note Thread-safe, restores values saved by applyStepParameterLocks
     * @note Safe to call even if no locks were applied
     */
    void restoreParametersFromStep(int track_id, int step_id);
    
    /**
     * @brief Clear all parameter locks from all tracks and steps
     * @note Thread-safe, useful for pattern reset or initialization
     * @note Does not restore currently applied locks - call from stopped state
     */
    void clearAllParameterLocks();
    
    // =============================================================================
    // PARAMETER LOCK MANAGEMENT (Individual Operations)
    // =============================================================================
    
    /**
     * @brief Set a parameter lock for a specific step
     * @param track_id Track number (0-based)
     * @param step_id Step number (0-based)
     * @param param_id Parameter to lock
     * @param value Normalized value to lock parameter to (0.0-1.0)
     * @note Thread-safe, can be called while sequencer is running
     * @note Overwrites any existing lock for this parameter on this step
     */
    void setStepParameterLock(int track_id, int step_id, ParameterID param_id, float value);
    
    /**
     * @brief Remove a parameter lock from a specific step
     * @param track_id Track number (0-based)
     * @param step_id Step number (0-based)
     * @param param_id Parameter to unlock
     * @note Thread-safe, safe to call even if no lock exists
     */
    void clearStepParameterLock(int track_id, int step_id, ParameterID param_id);
    
    /**
     * @brief Check if a parameter is locked on a specific step
     * @param track_id Track number (0-based)
     * @param step_id Step number (0-based)
     * @param param_id Parameter to check
     * @return true if parameter has a lock on this step
     * @note Thread-safe query operation
     */
    bool hasStepParameterLock(int track_id, int step_id, ParameterID param_id) const;
    
    /**
     * @brief Get the locked value for a parameter on a specific step
     * @param track_id Track number (0-based)
     * @param step_id Step number (0-based)
     * @param param_id Parameter to query
     * @return Locked value (0.0-1.0), or 0.0 if no lock exists
     * @note Thread-safe query operation
     */
    float getStepParameterLock(int track_id, int step_id, ParameterID param_id) const;
    
    // =============================================================================
    // BULK OPERATIONS (Digitakt-style editing features)
    // =============================================================================
    
    /**
     * @brief Copy all parameter locks from one step to another
     * @param src_track Source track number
     * @param src_step Source step number
     * @param dest_track Destination track number
     * @param dest_step Destination step number
     * @note Thread-safe, useful for pattern editing workflows
     * @note Overwrites any existing locks on destination step
     */
    void copyStepLocks(int src_track, int src_step, int dest_track, int dest_step);
    
    /**
     * @brief Clear all parameter locks from a specific step
     * @param track_id Track number (0-based)
     * @param step_id Step number (0-based)
     * @note Thread-safe, removes all locks but doesn't affect currently playing step
     */
    void clearStepLocks(int track_id, int step_id);
    
    /**
     * @brief Get list of all parameters that have locks on a specific step
     * @param track_id Track number (0-based)
     * @param step_id Step number (0-based)
     * @return Vector of parameter IDs that are locked on this step
     * @note Thread-safe, useful for UI display of locked parameters
     */
    std::vector<ParameterID> getLockedParametersForStep(int track_id, int step_id) const;
    
    // =============================================================================
    // STATISTICS AND DEBUGGING
    // =============================================================================
    
    /**
     * @brief Get total number of parameter locks across all tracks and steps
     * @return Total count of parameter locks
     * @note Thread-safe, useful for memory usage monitoring
     */
    size_t getTotalParameterLocks() const;
    
    /**
     * @brief Get number of parameter locks for a specific track
     * @param track_id Track number to query
     * @return Count of parameter locks on this track
     * @note Thread-safe
     */
    size_t getParameterLocksForTrack(int track_id) const;
    
    /**
     * @brief Print debugging information about parameter locks
     * @note Outputs to console, useful for development and debugging
     * @note Thread-safe
     */
    void printParameterLockStatistics() const;
    
    // =============================================================================
    // SYSTEM INTEGRATION
    // =============================================================================
    
    /**
     * @brief Set the parameter manager for parameter updates
     * @param param_manager Shared pointer to parameter manager
     * @note Must be called before using any lock functionality
     * @note Thread-safe
     */
    /**
     * @brief Set the parameter manager for parameter updates
     * @param param_manager Shared pointer to parameter manager
     * @note Must be called before using any lock functionality
     * @note Thread-safe
     */
    void setParameterManager(std::shared_ptr<Parameters::ParameterManager> param_manager);
    
private:
    // =============================================================================
    // INTERNAL DATA STORAGE
    // =============================================================================
    
    /**
     * @brief Parameter lock storage structure
     * @note Storage: [track_id][step_id] -> map<param_id, locked_value>
     * @note Allows efficient lookup by track and step
     */
    std::unordered_map<int, std::unordered_map<int, std::unordered_map<ParameterID, float>>> step_locks_;
    
    /**
     * @brief Backup of parameter values before locks were applied
     * @note Used to restore original values when step ends
     * @note Key: ParameterID, Value: Original parameter value
     */
    std::unordered_map<ParameterID, float> saved_parameter_values_;
    
    /**
     * @brief Track which parameters are currently locked by which steps
     * @note Used for cleanup when restoring parameters
     * @note [track_id][step_id] -> vector of currently locked parameter IDs
     */
    std::unordered_map<int, std::unordered_map<int, std::vector<ParameterID>>> active_step_locks_;
    
    /**
     * @brief Reference to parameter manager for applying parameter changes
     * @note Used for thread-safe parameter updates
     */
    std::shared_ptr<Parameters::ParameterManager> parameter_manager_;
    
    // =============================================================================
    // INTERNAL HELPER METHODS
    // =============================================================================
    
    /**
     * @brief Generate string key for track/step combination
     * @param track_id Track number
     * @param step_id Step number
     * @return String key for debugging and logging
     */
    std::string getTrackStepKey(int track_id, int step_id) const;
    
    /**
     * @brief Save current parameter value before applying lock
     * @param param_id Parameter to save
     * @note Called internally before applying locks
     */
    void saveParameterValue(ParameterID param_id);
    
    /**
     * @brief Restore previously saved parameter value
     * @param param_id Parameter to restore
     * @note Called internally when restoring from locks
     */
    void restoreParameterValue(ParameterID param_id);
    
    /**
     * @brief Log parameter lock events for debugging
     * @param action Description of action being performed
     * @param track_id Track number
     * @param step_id Step number
     * @param param_id Parameter being affected
     * @param value Parameter value
     * @note Only active in debug builds with event tracing enabled
     */
    void traceParameterLockEvent(const std::string& action, int track_id, int step_id, 
                                ParameterID param_id, float value) const;
};

} // namespace MIDI
