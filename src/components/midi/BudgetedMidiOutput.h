#pragma once

#include "hardware/MidiHandler.h"
#include <chrono>
#include <memory>

namespace MIDI {

/**
 * @brief Simple immediate MIDI output with timing budget
 * 
 * No extra threads, no complex scheduling. Just ensures MIDI
 * processing happens within a strict time budget each loop.
 */
class BudgetedMidiOutput {
public:
    BudgetedMidiOutput(std::shared_ptr<MidiHandler> midi_handler);
    
    // Main processing with time budget
    void processWithBudget(std::chrono::microseconds max_time);
    
    // Event scheduling (immediate send if budget allows)
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    void sendNoteOff(uint8_t channel, uint8_t note);
    
    // Budget management
    void startLoopTiming();
    bool hasBudgetRemaining(std::chrono::microseconds required) const;
    std::chrono::microseconds getRemainingBudget() const;
    
    // Statistics
    struct BudgetStats {
        int loops_processed = 0;
        int budget_exceeded = 0;
        float average_usage_percent = 0.0f;
        float max_usage_percent = 0.0f;
    };
    
    const BudgetStats& getStats() const { return stats_; }

private:
    std::shared_ptr<MidiHandler> midi_handler_;
    
    // Timing budget
    std::chrono::steady_clock::time_point loop_start_;
    std::chrono::microseconds budget_limit_{500}; // 500μs budget per loop
    
    BudgetStats stats_;
    
    void updateBudgetStats(std::chrono::microseconds used);
};

/**
 * @brief Ultra-simple main loop with MIDI budget
 * 
 * Guarantees MIDI timing by enforcing strict time budgets.
 * Much simpler than threading but still effective.
 */
class BudgetedMainLoop {
public:
    void loop() {
        auto loop_start = std::chrono::steady_clock::now();
        
        // PHASE 1: MIDI processing (strict 500μs budget)
        budgeted_midi_->startLoopTiming();
        budgeted_midi_->processWithBudget(std::chrono::microseconds(500));
        
        // PHASE 2: UI processing (remaining time)
        auto midi_end = std::chrono::steady_clock::now();
        auto midi_time = std::chrono::duration_cast<std::chrono::microseconds>(midi_end - loop_start);
        
        if (midi_time < std::chrono::microseconds(1000)) { // Leave 1ms total budget
            auto ui_budget = std::chrono::microseconds(1000) - midi_time;
            processUIWithBudget(ui_budget);
        }
        
        // Sleep to maintain loop rate
        Threading::TaskManager::sleep(16); // 60Hz
    }

private:
    std::shared_ptr<BudgetedMidiOutput> budgeted_midi_;
    
    void processUIWithBudget(std::chrono::microseconds budget);
};

} // namespace MIDI
