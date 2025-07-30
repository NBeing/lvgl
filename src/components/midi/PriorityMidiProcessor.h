#pragma once

#include "components/midi/ThreadedMidiClockManager.h"
#include "components/midi/StepSequencer.h"
#include "components/midi/TimestampedMidiOutput.h"
#include <chrono>

namespace MIDI {

/**
 * @brief High-priority MIDI processing manager
 * 
 * Ensures time-critical MIDI operations are processed with minimal latency
 * before UI updates can interfere.
 */
class PriorityMidiProcessor {
public:
    PriorityMidiProcessor();
    ~PriorityMidiProcessor() = default;
    
    // Main processing function - call FIRST in main loop
    void processTimeCritical();
    
    // Statistics and monitoring
    struct ProcessingStats {
        float average_processing_time_us = 0.0f;
        float max_processing_time_us = 0.0f;
        int processed_cycles = 0;
        int missed_deadlines = 0;  // When processing took > 1ms
        
        void reset() {
            average_processing_time_us = 0.0f;
            max_processing_time_us = 0.0f;
            processed_cycles = 0;
            missed_deadlines = 0;
        }
    };
    
    const ProcessingStats& getStats() const { return stats_; }
    void resetStats() { stats_.reset(); }
    
    // Configuration
    void setMaxProcessingTime(std::chrono::microseconds max_time) { 
        max_processing_time_ = max_time; 
    }
    
    // Integration points
    void setStepSequencer(std::shared_ptr<StepSequencer> sequencer) { 
        step_sequencer_ = sequencer; 
    }
    void setTimestampedOutput(std::shared_ptr<TimestampedMidiOutput> output) { 
        timestamped_output_ = output; 
    }

private:
    // Time-critical processing components
    std::shared_ptr<StepSequencer> step_sequencer_;
    std::shared_ptr<TimestampedMidiOutput> timestamped_output_;
    
    // Timing control
    std::chrono::microseconds max_processing_time_{1000}; // 1ms deadline
    std::chrono::steady_clock::time_point last_process_time_;
    
    // Statistics
    ProcessingStats stats_;
    
    // Internal processing
    void updateProcessingStats(std::chrono::nanoseconds processing_time);
    bool checkDeadline(std::chrono::steady_clock::time_point start_time);
};

/**
 * @brief Modified main loop with priority processing
 */
class TimingCorrectMainLoop {
public:
    TimingCorrectMainLoop();
    
    // Main loop implementation
    void loop();
    
    // Setup
    void initialize();
    
private:
    PriorityMidiProcessor priority_processor_;
    
    // Timing measurements
    struct LoopStats {
        float midi_processing_time_us = 0.0f;
        float ui_processing_time_us = 0.0f;
        float total_loop_time_us = 0.0f;
        int loop_count = 0;
    };
    
    LoopStats loop_stats_;
    std::chrono::steady_clock::time_point last_stats_time_;
    
    void updateLoopStats();
    void printPerformanceReport();
};

} // namespace MIDI
