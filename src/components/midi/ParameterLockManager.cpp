#include "ParameterLockManager.h"
#include <iostream>
#include <algorithm>

namespace MIDI {

ParameterLockManager::ParameterLockManager() {
    // Initialize empty
}

void ParameterLockManager::setParameterManager(std::shared_ptr<Parameters::ParameterManager> param_manager) {
    parameter_manager_ = param_manager;
}

void ParameterLockManager::applyStepParameterLocks(int track_id, int step_id, 
                                                  const std::unordered_map<ParameterID, float>& locks) {
    if (!parameter_manager_ || locks.empty()) {
        return;
    }
    
    #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
    traceParameterLockEvent("applyStepLocks", track_id, step_id, 0, locks.size());
    #endif
    
    // Clear any existing active locks for this track/step
    restoreParametersFromStep(track_id, step_id);
    
    // Apply new parameter locks
    std::vector<ParameterID> applied_locks;
    
    for (const auto& [param_id, locked_value] : locks) {
        // Save current parameter value before overriding
        saveParameterValue(param_id);
        
        // Apply the locked value using setParameter
        parameter_manager_->setParameter(param_id, locked_value, Parameters::ParameterSource::AUTOMATION);
        
        applied_locks.push_back(param_id);
        
        #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
        traceParameterLockEvent("lockApplied", track_id, step_id, param_id, locked_value);
        #endif
        
        std::cout << "[ParameterLock] 🔒 Applied lock T" << track_id << "S" << step_id 
                  << " Param:" << static_cast<int>(param_id) << " = " << locked_value << std::endl;
    }
    
    // Track which parameters are locked for this step (for later restoration)
    active_step_locks_[track_id][step_id] = applied_locks;
}

void ParameterLockManager::restoreParametersFromStep(int track_id, int step_id) {
    auto track_it = active_step_locks_.find(track_id);
    if (track_it == active_step_locks_.end()) {
        return;
    }
    
    auto step_it = track_it->second.find(step_id);
    if (step_it == track_it->second.end()) {
        return;
    }
    
    #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
    traceParameterLockEvent("restoreStepLocks", track_id, step_id, 0, step_it->second.size());
    #endif
    
    // Restore all parameters that were locked for this step
    for (ParameterID param_id : step_it->second) {
        restoreParameterValue(param_id);
        
        #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
        auto saved_it = saved_parameter_values_.find(param_id);
        if (saved_it != saved_parameter_values_.end()) {
            traceParameterLockEvent("lockRestored", track_id, step_id, param_id, saved_it->second);
        }
        #endif
    }
    
    // Clear the active locks for this step
    step_it->second.clear();
    
    std::cout << "[ParameterLock] 🔓 Restored parameters for T" << track_id << "S" << step_id << std::endl;
}

void ParameterLockManager::setStepParameterLock(int track_id, int step_id, ParameterID param_id, float value) {
    step_locks_[track_id][step_id][param_id] = value;
    
    #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
    traceParameterLockEvent("setLock", track_id, step_id, param_id, value);
    #endif
    
    std::cout << "[ParameterLock] 📝 Set lock T" << track_id << "S" << step_id 
              << " Param:" << static_cast<int>(param_id) << " = " << value << std::endl;
}

void ParameterLockManager::clearStepParameterLock(int track_id, int step_id, ParameterID param_id) {
    auto track_it = step_locks_.find(track_id);
    if (track_it != step_locks_.end()) {
        auto step_it = track_it->second.find(step_id);
        if (step_it != track_it->second.end()) {
            step_it->second.erase(param_id);
            
            #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
            traceParameterLockEvent("clearLock", track_id, step_id, param_id, 0);
            #endif
            
            std::cout << "[ParameterLock] 🗑️ Cleared lock T" << track_id << "S" << step_id 
                      << " Param:" << static_cast<int>(param_id) << std::endl;
        }
    }
}

bool ParameterLockManager::hasStepParameterLock(int track_id, int step_id, ParameterID param_id) const {
    auto track_it = step_locks_.find(track_id);
    if (track_it != step_locks_.end()) {
        auto step_it = track_it->second.find(step_id);
        if (step_it != track_it->second.end()) {
            return step_it->second.find(param_id) != step_it->second.end();
        }
    }
    return false;
}

float ParameterLockManager::getStepParameterLock(int track_id, int step_id, ParameterID param_id) const {
    auto track_it = step_locks_.find(track_id);
    if (track_it != step_locks_.end()) {
        auto step_it = track_it->second.find(step_id);
        if (step_it != track_it->second.end()) {
            auto param_it = step_it->second.find(param_id);
            if (param_it != step_it->second.end()) {
                return param_it->second;
            }
        }
    }
    return 0.0f;
}

void ParameterLockManager::copyStepLocks(int src_track, int src_step, int dest_track, int dest_step) {
    auto src_track_it = step_locks_.find(src_track);
    if (src_track_it != step_locks_.end()) {
        auto src_step_it = src_track_it->second.find(src_step);
        if (src_step_it != src_track_it->second.end()) {
            // Copy all parameter locks from source to destination
            step_locks_[dest_track][dest_step] = src_step_it->second;
            
            std::cout << "[ParameterLock] 📋 Copied " << src_step_it->second.size() 
                      << " locks from T" << src_track << "S" << src_step 
                      << " to T" << dest_track << "S" << dest_step << std::endl;
        }
    }
}

void ParameterLockManager::clearStepLocks(int track_id, int step_id) {
    auto track_it = step_locks_.find(track_id);
    if (track_it != step_locks_.end()) {
        auto step_it = track_it->second.find(step_id);
        if (step_it != track_it->second.end()) {
            size_t cleared_count = step_it->second.size();
            step_it->second.clear();
            
            std::cout << "[ParameterLock] 🧹 Cleared " << cleared_count 
                      << " locks from T" << track_id << "S" << step_id << std::endl;
        }
    }
}

std::vector<ParameterID> ParameterLockManager::getLockedParametersForStep(int track_id, int step_id) const {
    std::vector<ParameterID> locked_params;
    
    auto track_it = step_locks_.find(track_id);
    if (track_it != step_locks_.end()) {
        auto step_it = track_it->second.find(step_id);
        if (step_it != track_it->second.end()) {
            for (const auto& [param_id, value] : step_it->second) {
                locked_params.push_back(param_id);
            }
        }
    }
    
    return locked_params;
}

size_t ParameterLockManager::getTotalParameterLocks() const {
    size_t total = 0;
    for (const auto& [track_id, track_locks] : step_locks_) {
        for (const auto& [step_id, step_locks] : track_locks) {
            total += step_locks.size();
        }
    }
    return total;
}

size_t ParameterLockManager::getParameterLocksForTrack(int track_id) const {
    size_t track_total = 0;
    auto track_it = step_locks_.find(track_id);
    if (track_it != step_locks_.end()) {
        for (const auto& [step_id, step_locks] : track_it->second) {
            track_total += step_locks.size();
        }
    }
    return track_total;
}

void ParameterLockManager::clearAllParameterLocks() {
    size_t total_cleared = getTotalParameterLocks();
    step_locks_.clear();
    active_step_locks_.clear();
    saved_parameter_values_.clear();
    
    std::cout << "[ParameterLock] 🗑️ Cleared ALL parameter locks (" << total_cleared << " total)" << std::endl;
}

void ParameterLockManager::printParameterLockStatistics() const {
    std::cout << "\n[ParameterLock] 📊 Statistics:" << std::endl;
    std::cout << "  Total locks: " << getTotalParameterLocks() << std::endl;
    
    for (const auto& [track_id, track_locks] : step_locks_) {
        size_t track_total = getParameterLocksForTrack(track_id);
        std::cout << "  Track " << track_id << ": " << track_total << " locks" << std::endl;
    }
}

// Private helper methods
void ParameterLockManager::saveParameterValue(ParameterID param_id) {
    if (parameter_manager_) {
        saved_parameter_values_[param_id] = parameter_manager_->getParameterNormalized(param_id);
    }
}

void ParameterLockManager::restoreParameterValue(ParameterID param_id) {
    auto it = saved_parameter_values_.find(param_id);
    if (it != saved_parameter_values_.end() && parameter_manager_) {
        parameter_manager_->setParameter(param_id, it->second, Parameters::ParameterSource::AUTOMATION);
        saved_parameter_values_.erase(it);
    }
}

void ParameterLockManager::traceParameterLockEvent(const std::string& action, int track_id, int step_id, 
                                                  ParameterID param_id, float value) const {
    #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
    std::string event_data = "T" + std::to_string(track_id) + "S" + std::to_string(step_id);
    if (param_id != 0) {
        event_data += " P" + std::to_string(static_cast<int>(param_id)) + ":" + std::to_string(value);
    } else {
        event_data += " Count:" + std::to_string(static_cast<int>(value));
    }
    TRACE_PARAMETER_EVENT("ParameterLockManager", "StepSequencer", action.c_str(), event_data.c_str());
    #endif
}

} // namespace MIDI
