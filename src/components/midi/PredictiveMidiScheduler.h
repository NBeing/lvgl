#pragma once

#include "hardware/MidiHandler.h"
#include <chrono>
#include <memory>
#include <deque>

namespace MIDI {

/**
 * @brief Predictive MIDI scheduling (no extra threads needed)
 * 
 * Learns UI timing patterns and schedules MIDI events to avoid
 * collisions with heavy UI updates. Much simpler than timing thread.
 */
class PredictiveMidiScheduler {
public:
    PredictiveMidiScheduler();
    ~PredictiveMidiScheduler() = default;
    
    // Setup
    void setMidiHandler(std::shared_ptr<MidiHandler> handler) { 
        midi_handler_ = handler; 
    }
    
    // Main processing (call from main loop)
    void processEvents();
    
    // Event scheduling with UI-aware timing
    void scheduleNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, 
                       std::chrono::steady_clock::time_point ideal_time);
    void scheduleNoteOff(uint8_t channel, uint8_t note, 
                        std::chrono::steady_clock::time_point ideal_time);
    
    // UI timing integration
    void markUIFrameStart();
    void markUIFrameEnd();
    void setExpectedUILoad(float cpu_percentage); // 0.0-1.0
    
    // Statistics
    struct PredictionStats {
        float average_ui_frame_ms = 16.7f;
        float max_ui_frame_ms = 16.7f;
        float ui_load_prediction = 0.1f;
        int events_rescheduled = 0;
        int prediction_hits = 0;
        int prediction_misses = 0;
    };
    
    const PredictionStats& getStats() const { return stats_; }
    void resetStats() { stats_ = {}; }

private:
    std::shared_ptr<MidiHandler> midi_handler_;
    
    struct ScheduledEvent {
        enum Type { NOTE_ON, NOTE_OFF };
        Type type;
        uint8_t channel, note, velocity;
        std::chrono::steady_clock::time_point ideal_time;
        std::chrono::steady_clock::time_point actual_time;
        bool rescheduled = false;
    };
    
    std::deque<ScheduledEvent> scheduled_events_;
    
    // UI timing prediction
    std::chrono::steady_clock::time_point ui_frame_start_;
    std::chrono::steady_clock::time_point last_frame_end_;
    std::deque<float> recent_frame_times_; // Rolling window
    static constexpr size_t FRAME_HISTORY_SIZE = 60; // 1 second @ 60Hz
    
    PredictionStats stats_;
    
    // Prediction algorithms
    std::chrono::steady_clock::time_point predictNextUIGap();
    bool isUIBusy(std::chrono::steady_clock::time_point when) const;
    std::chrono::steady_clock::time_point findSafeWindow(
        std::chrono::steady_clock::time_point ideal_time);
    
    // Event processing
    void sendEvent(const ScheduledEvent& event);
    void updateFrameTimeStats(float frame_time_ms);
    
    // Smart scheduling
    std::chrono::steady_clock::time_point adjustForUITiming(
        std::chrono::steady_clock::time_point ideal_time);
};

/**
 * @brief UI-aware main loop (no extra threads!)
 * 
 * Integrates predictive scheduling directly into main loop
 * for precise MIDI timing without thread complexity.
 */
class UIAwareMainLoop {
public:
    UIAwareMainLoop();
    
    void loop();
    void initialize();
    
    // Integration
    void setPredictiveScheduler(std::shared_ptr<PredictiveMidiScheduler> scheduler) {
        predictive_scheduler_ = scheduler;
    }

private:
    std::shared_ptr<PredictiveMidiScheduler> predictive_scheduler_;
    
    // Timing tracking
    std::chrono::steady_clock::time_point loop_start_;
    std::chrono::steady_clock::time_point ui_start_;
    
    // Adaptive processing
    void processTimeCriticalMidi();
    void processAdaptiveUI();
    void updateTimingPredictions();
};

} // namespace MIDI
