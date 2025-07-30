#include "AdaptiveUIController.h"
#include <iostream>
#include <algorithm>

namespace MIDI {

AdaptiveUIController::AdaptiveUIController() 
    : current_activity_(MidiActivity::IDLE),
      last_ui_update_(std::chrono::steady_clock::now()),
      ui_frame_start_(std::chrono::steady_clock::now())
{
    // Constructor body is empty - all initialization in member initializer list
}

void AdaptiveUIController::setMidiActivity(MidiActivity activity) {
    if (current_activity_.load() != activity) {
        current_activity_.store(activity);
        
        std::cout << "[AdaptiveUI] MIDI activity changed to: " << getActivityName(activity) << std::endl;
        std::cout << "[AdaptiveUI] UI interval: " << getIntervalForActivity(activity).count() << "ms" << std::endl;
    }
}

bool AdaptiveUIController::shouldProcessUI() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_ui_update_);
    auto required_interval = getIntervalForActivity(current_activity_.load());
    
    return elapsed >= required_interval;
}

std::chrono::milliseconds AdaptiveUIController::getCurrentUIInterval() const {
    return getIntervalForActivity(current_activity_.load());
}

void AdaptiveUIController::markUIFrameComplete() {
    auto now = std::chrono::steady_clock::now();
    auto frame_time = std::chrono::duration_cast<std::chrono::nanoseconds>(now - last_ui_update_);
    
    last_ui_update_ = now;
    
    // Update statistics
    updateUIStats(frame_time);
    stats_.frames_processed++;
}

std::chrono::milliseconds AdaptiveUIController::getIntervalForActivity(MidiActivity activity) const {
    switch (activity) {
        case MidiActivity::IDLE:
            return ui_interval_idle_;
            
        case MidiActivity::SEQUENCING:
            return ui_interval_sequencing_;
            
        case MidiActivity::RECORDING:
            return ui_interval_recording_;
            
        case MidiActivity::CRITICAL:
            return ui_interval_critical_;
            
        default:
            return ui_interval_idle_;
    }
}

void AdaptiveUIController::updateUIStats(std::chrono::nanoseconds frame_time) {
    float frame_time_ms = std::chrono::duration_cast<std::chrono::microseconds>(frame_time).count() / 1000.0f;
    
    if (stats_.frames_processed == 0) {
        stats_.average_frame_time_ms = frame_time_ms;
    } else {
        stats_.average_frame_time_ms = (stats_.average_frame_time_ms * stats_.frames_processed + frame_time_ms) / (stats_.frames_processed + 1);
    }
    
    stats_.max_frame_time_ms = std::max(stats_.max_frame_time_ms, frame_time_ms);
}

void AdaptiveUIController::setUIIntervalForActivity(MidiActivity activity, std::chrono::milliseconds interval) {
    switch (activity) {
        case MidiActivity::IDLE:
            ui_interval_idle_ = interval;
            break;
            
        case MidiActivity::SEQUENCING:
            ui_interval_sequencing_ = interval;
            break;
            
        case MidiActivity::RECORDING:
            ui_interval_recording_ = interval;
            break;
            
        case MidiActivity::CRITICAL:
            ui_interval_critical_ = interval;
            break;
    }
    
    std::cout << "[AdaptiveUI] Updated interval for " << getActivityName(activity) 
              << " to " << interval.count() << "ms" << std::endl;
}

const char* AdaptiveUIController::getActivityName(MidiActivity activity) const {
    switch (activity) {
        case MidiActivity::IDLE: return "IDLE";
        case MidiActivity::SEQUENCING: return "SEQUENCING";
        case MidiActivity::RECORDING: return "RECORDING";
        case MidiActivity::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

} // namespace MIDI
