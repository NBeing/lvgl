#include "AdaptiveFrameController.h"
#include <algorithm>
#include <numeric>
#include <sstream>

namespace MIDI {

AdaptiveFrameController::AdaptiveFrameController() {
    // Note: deque doesn't have reserve(), but that's okay
    // It will allocate as needed
    resetStats();
}

AdaptiveFrameController::FrameDecision AdaptiveFrameController::shouldRenderFrame(
    std::chrono::microseconds available_time,
    std::chrono::microseconds midi_time) {
    
    // Calculate frame budget based on target FPS
    auto frame_budget = std::chrono::microseconds(1000000 / target_fps_); // 16.67ms @ 60fps
    
    std::cout << "[FrameController] Available: " << available_time.count() 
              << "μs, MIDI took: " << midi_time.count() << "μs" << std::endl;
    
    // Decision tree based on available time
    if (available_time >= std::chrono::milliseconds(10)) {
        // Plenty of time - full render
        return FrameDecision::RENDER_FULL;
    } else if (available_time >= std::chrono::milliseconds(2)) {
        // Limited time - minimal render
        return FrameDecision::RENDER_MINIMAL;
    } else if (available_time >= std::chrono::microseconds(500)) {
        // Very limited time - only if we haven't skipped recently
        if (shouldSkipBasedOnHistory()) {
            return FrameDecision::SKIP_FRAME;
        } else {
            return FrameDecision::RENDER_MINIMAL;
        }
    } else {
        // No time - must skip
        std::cout << "⏭️ [FrameController] FORCED SKIP - only " 
                  << available_time.count() << "μs available" << std::endl;
        return FrameDecision::SKIP_FRAME;
    }
}

void AdaptiveFrameController::markFrameComplete(FrameDecision decision, 
                                               std::chrono::microseconds actual_time) {
    // Update history
    recent_frame_times_.push_back(actual_time);
    recent_skip_decisions_.push_back(decision == FrameDecision::SKIP_FRAME);
    
    if (recent_frame_times_.size() > HISTORY_SIZE) {
        recent_frame_times_.pop_front();
        recent_skip_decisions_.pop_front();
    }
    
    // Update statistics
    if (decision == FrameDecision::SKIP_FRAME) {
        stats_.frames_skipped++;
        std::cout << "⏭️ Frame skipped (total skipped: " << stats_.frames_skipped << ")" << std::endl;
    } else {
        stats_.frames_rendered++;
        std::cout << "✅ Frame rendered in " << actual_time.count() << "μs" << std::endl;
    }
    
    updateFrameStats();
}

bool AdaptiveFrameController::shouldSkipBasedOnHistory() const {
    if (recent_skip_decisions_.size() < 10) return false;
    
    // Count recent skips
    int recent_skips = std::count(recent_skip_decisions_.end() - 10, 
                                 recent_skip_decisions_.end(), true);
    
    // Don't skip if we've already skipped 3+ of the last 10 frames
    return recent_skips < 3;
}

void AdaptiveFrameController::updateFrameStats() {
    if (recent_frame_times_.empty()) return;
    
    // Calculate actual FPS
    auto total_frames = stats_.frames_rendered + stats_.frames_skipped;
    if (total_frames > 0) {
        auto skip_ratio = static_cast<float>(stats_.frames_skipped) / total_frames;
        stats_.skip_percentage = skip_ratio * 100.0f;
        
        // Estimate actual FPS accounting for skips
        stats_.actual_fps = target_fps_ * (1.0f - skip_ratio);
    }
    
    // Calculate average frame time (for rendered frames only)
    if (!recent_frame_times_.empty()) {
        auto avg_time_us = std::accumulate(recent_frame_times_.begin(), 
                                          recent_frame_times_.end(), 
                                          std::chrono::microseconds{0}).count() / recent_frame_times_.size();
        stats_.average_frame_time_ms = avg_time_us / 1000.0f;
    }
}

bool AdaptiveFrameController::isVisualQualityAcceptable() const {
    return stats_.actual_fps >= minimum_fps_ && stats_.skip_percentage < 30.0f;
}

std::string AdaptiveFrameController::getPerformanceReport() const {
    std::ostringstream oss;
    oss << "Frame Performance Report:\n";
    oss << "  Target FPS: " << target_fps_ << " | Actual FPS: " << stats_.actual_fps << "\n";
    oss << "  Frames Rendered: " << stats_.frames_rendered << " | Skipped: " << stats_.frames_skipped << "\n";
    oss << "  Skip Percentage: " << stats_.skip_percentage << "%\n";
    oss << "  Average Frame Time: " << stats_.average_frame_time_ms << "ms\n";
    oss << "  Visual Quality: " << (isVisualQualityAcceptable() ? "✅ Acceptable" : "❌ Poor");
    return oss.str();
}

// SmartUIRenderer Implementation
SmartUIRenderer::SmartUIRenderer() {
    // Initialize stats
}

SmartUIRenderer::RenderMode SmartUIRenderer::selectRenderMode(
    std::chrono::microseconds available_time) const {
    
    if (available_time >= std::chrono::milliseconds(10)) {
        return RenderMode::FULL;
    } else if (available_time >= std::chrono::milliseconds(2)) {
        return RenderMode::ESSENTIAL;
    } else if (available_time >= std::chrono::microseconds(500)) {
        return RenderMode::MINIMAL;
    } else {
        return RenderMode::SKIP;
    }
}

bool SmartUIRenderer::renderFrame(RenderMode mode, std::chrono::microseconds time_budget) {
    auto start_time = std::chrono::steady_clock::now();
    bool success = false;
    
    switch (mode) {
        case RenderMode::FULL:
            success = renderFullUI(time_budget);
            if (success) stats_.full_renders++;
            break;
            
        case RenderMode::ESSENTIAL:
            success = renderEssentialUI(time_budget);
            if (success) stats_.essential_renders++;
            break;
            
        case RenderMode::MINIMAL:
            success = renderMinimalUI(time_budget);
            if (success) stats_.minimal_renders++;
            break;
            
        case RenderMode::SKIP:
            stats_.skipped_renders++;
            return true; // "Success" - we successfully skipped
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "[UIRenderer] Mode: " << static_cast<int>(mode) 
              << ", Time: " << elapsed.count() << "μs, Success: " << success << std::endl;
    
    return success;
}

bool SmartUIRenderer::renderFullUI(std::chrono::microseconds budget) {
    auto start = std::chrono::steady_clock::now();
    
    // Full LVGL update
    lv_timer_handler();
    
    // Update all UI components
    // (This would call your window manager, tabs, etc.)
    
    return checkTimeBudget(start, budget);
}

bool SmartUIRenderer::renderEssentialUI(std::chrono::microseconds budget) {
    auto start = std::chrono::steady_clock::now();
    
    // Only update critical UI elements
    // - Clock display
    // - Transport controls  
    // - Critical status indicators
    lv_timer_handler(); // Still need LVGL, but limit scope
    
    return checkTimeBudget(start, budget);
}

bool SmartUIRenderer::renderMinimalUI(std::chrono::microseconds budget) {
    auto start = std::chrono::steady_clock::now();
    
    // Absolute minimum updates
    // - Just LVGL core (no custom updates)
    lv_timer_handler();
    
    return checkTimeBudget(start, budget);
}

bool SmartUIRenderer::checkTimeBudget(std::chrono::steady_clock::time_point start,
                                     std::chrono::microseconds budget) const {
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start);
    return elapsed <= budget;
}

} // namespace MIDI
