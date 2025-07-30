#pragma once

#include <chrono>
#include <deque>
#include <iostream>

// Forward declaration for LVGL
extern "C" {
    void lv_timer_handler(void);
}

namespace MIDI {

/**
 * @brief Adaptive frame rate controller with intelligent skipping
 * 
 * Manages UI frame skipping to maintain MIDI timing while minimizing
 * visual impact on user experience.
 */
class AdaptiveFrameController {
public:
    AdaptiveFrameController();
    
    // Frame rate management
    enum class FrameDecision {
        RENDER_FULL,      // Normal frame rendering
        RENDER_MINIMAL,   // Only critical UI updates
        SKIP_FRAME        // Skip this frame entirely
    };
    
    FrameDecision shouldRenderFrame(std::chrono::microseconds available_time,
                                   std::chrono::microseconds midi_time);
    
    void markFrameComplete(FrameDecision decision, 
                          std::chrono::microseconds actual_time);
    
    // Configuration
    void setTargetFrameRate(int fps) { target_fps_ = fps; }
    void setMinimumFrameRate(int fps) { minimum_fps_ = fps; }
    void setMidiPriority(bool high_priority) { midi_priority_ = high_priority; }
    
    // Statistics
    struct FrameStats {
        float actual_fps = 60.0f;
        float skip_percentage = 0.0f;
        int frames_rendered = 0;
        int frames_skipped = 0;
        float average_frame_time_ms = 16.7f;
        
        void reset() {
            actual_fps = 60.0f;
            skip_percentage = 0.0f;
            frames_rendered = 0;
            frames_skipped = 0;
            average_frame_time_ms = 16.7f;
        }
    };
    
    const FrameStats& getStats() const { return stats_; }
    void resetStats() { stats_.reset(); }
    
    // User experience assessment
    bool isVisualQualityAcceptable() const;
    std::string getPerformanceReport() const;

private:
    // Configuration
    int target_fps_ = 60;
    int minimum_fps_ = 15;  // Below this = unusable
    bool midi_priority_ = true;
    
    // Frame timing history
    std::deque<std::chrono::microseconds> recent_frame_times_;
    std::deque<bool> recent_skip_decisions_;
    static constexpr size_t HISTORY_SIZE = 60; // 1 second of history
    
    // Statistics
    FrameStats stats_;
    std::chrono::steady_clock::time_point last_stats_update_;
    
    // Adaptive algorithms
    std::chrono::microseconds estimateFrameTime() const;
    bool shouldSkipBasedOnHistory() const;
    FrameDecision makeIntelligentDecision(std::chrono::microseconds available_time);
    
    void updateFrameStats();
};

/**
 * @brief Smart UI renderer with multiple quality levels
 * 
 * Provides different rendering modes to fit within available time budgets.
 */
class SmartUIRenderer {
public:
    enum class RenderMode {
        FULL,        // Complete UI update (5-20ms)
        ESSENTIAL,   // Only critical elements (1-5ms)  
        MINIMAL,     // Bare minimum updates (0.1-1ms)
        SKIP         // No rendering
    };
    
    SmartUIRenderer();
    
    // Main rendering function
    bool renderFrame(RenderMode mode, std::chrono::microseconds time_budget);
    
    // Mode selection based on available time
    RenderMode selectRenderMode(std::chrono::microseconds available_time) const;
    
    // Statistics
    struct RenderStats {
        std::chrono::microseconds last_full_render{0};
        std::chrono::microseconds last_essential_render{0};
        std::chrono::microseconds last_minimal_render{0};
        int full_renders = 0;
        int essential_renders = 0;
        int minimal_renders = 0;
        int skipped_renders = 0;
    };
    
    const RenderStats& getStats() const { return stats_; }

private:
    RenderStats stats_;
    
    // Different rendering approaches
    bool renderFullUI(std::chrono::microseconds budget);
    bool renderEssentialUI(std::chrono::microseconds budget);
    bool renderMinimalUI(std::chrono::microseconds budget);
    
    // Timing utilities
    bool checkTimeBudget(std::chrono::steady_clock::time_point start,
                        std::chrono::microseconds budget) const;
};

} // namespace MIDI
