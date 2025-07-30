#pragma once

#include <chrono>
#include <atomic>
#include <memory>

namespace MIDI {

// Forward declarations
class MidiTimingThread;
class AdaptiveUIController;

/**
 * @brief Adaptive UI timing controller
 * 
 * Automatically reduces UI update frequency when MIDI timing precision
 * is critical, preventing long UI updates from affecting MIDI timing.
 */
class AdaptiveUIController {
public:
    enum class MidiActivity {
        IDLE,           // No time-critical MIDI activity
        SEQUENCING,     // Step sequencer is running
        RECORDING,      // Recording MIDI input
        CRITICAL        // Ultra-precise timing required
    };
    
    AdaptiveUIController();
    ~AdaptiveUIController() = default;
    
    // Main control functions
    void setMidiActivity(MidiActivity activity);
    MidiActivity getMidiActivity() const { return current_activity_.load(); }
    
    // UI timing control
    bool shouldProcessUI() const;
    std::chrono::milliseconds getCurrentUIInterval() const;
    void markUIFrameComplete();
    
    // Performance monitoring
    struct UIStats {
        float average_frame_time_ms = 0.0f;
        float max_frame_time_ms = 0.0f;
        int frames_processed = 0;
        int frames_skipped = 0;
        
        void reset() {
            average_frame_time_ms = 0.0f;
            max_frame_time_ms = 0.0f;
            frames_processed = 0;
            frames_skipped = 0;
        }
    };
    
    const UIStats& getStats() const { return stats_; }
    void resetStats() { stats_.reset(); }
    
    // Utility methods
    const char* getActivityName(MidiActivity activity) const;
    
    // Configuration
    void setUIIntervalForActivity(MidiActivity activity, std::chrono::milliseconds interval);
    void setMaxUIFrameTime(std::chrono::milliseconds max_time) { 
        max_ui_frame_time_ = max_time; 
    }

private:
    // Current state
    std::atomic<MidiActivity> current_activity_{MidiActivity::IDLE};
    
    // Timing intervals for different activities
    std::chrono::milliseconds ui_interval_idle_{16};      // 60Hz when idle
    std::chrono::milliseconds ui_interval_sequencing_{33}; // 30Hz when sequencing
    std::chrono::milliseconds ui_interval_recording_{50};  // 20Hz when recording
    std::chrono::milliseconds ui_interval_critical_{100}; // 10Hz when critical
    
    // Frame timing tracking
    std::chrono::steady_clock::time_point last_ui_update_;
    std::chrono::steady_clock::time_point ui_frame_start_;
    std::chrono::milliseconds max_ui_frame_time_{5}; // Interrupt long frames
    
    // Statistics
    UIStats stats_;
    
    // Internal helpers
    std::chrono::milliseconds getIntervalForActivity(MidiActivity activity) const;
    void updateUIStats(std::chrono::nanoseconds frame_time);
};

/**
 * @brief Smart main loop with adaptive UI timing
 * 
 * Combines high-frequency MIDI timing thread with adaptive UI updates
 * to ensure perfect MIDI timing regardless of UI complexity.
 */
class SmartMainLoop {
public:
    SmartMainLoop();
    ~SmartMainLoop() = default;
    
    // Main loop - call this continuously
    void loop();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Integration points
    void setMidiTimingThread(std::shared_ptr<MidiTimingThread> timing_thread) {
        midi_timing_thread_ = timing_thread;
    }
    
    void setUIController(std::shared_ptr<AdaptiveUIController> ui_controller) {
        ui_controller_ = ui_controller;
    }
    
    // Activity control
    void setMidiActivity(AdaptiveUIController::MidiActivity activity);
    
    // Statistics
    struct LoopStats {
        float total_loop_time_us = 0.0f;
        float midi_processing_time_us = 0.0f;
        float ui_processing_time_us = 0.0f;
        int total_loops = 0;
        int ui_updates = 0;
        int ui_skips = 0;
        
        void reset() {
            total_loop_time_us = 0.0f;
            midi_processing_time_us = 0.0f;
            ui_processing_time_us = 0.0f;
            total_loops = 0;
            ui_updates = 0;
            ui_skips = 0;
        }
    };
    
    const LoopStats& getStats() const { return loop_stats_; }
    void resetStats() { loop_stats_.reset(); }

private:
    // Core components
    std::shared_ptr<MidiTimingThread> midi_timing_thread_;
    std::shared_ptr<AdaptiveUIController> ui_controller_;
    
    // State
    bool initialized_ = false;
    
    // Statistics
    LoopStats loop_stats_;
    std::chrono::steady_clock::time_point last_stats_print_;
    
    // Internal processing
    void processMidiEvents();
    void processUIUpdates();
    void updateLoopStats(std::chrono::nanoseconds total_time, 
                        std::chrono::nanoseconds midi_time, 
                        std::chrono::nanoseconds ui_time);
    void printPeriodicStats();
};

} // namespace MIDI
