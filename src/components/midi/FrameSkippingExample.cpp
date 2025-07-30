#include "AdaptiveFrameController.h"
#include <iostream>
#include <thread>

namespace MIDI {

/**
 * @brief Example of intelligent frame skipping in action
 */
class ExampleBudgetedLoop {
public:
    ExampleBudgetedLoop() {
        frame_controller_.setTargetFrameRate(60);      // 60fps target
        frame_controller_.setMinimumFrameRate(15);     // 15fps minimum
        frame_controller_.setMidiPriority(true);       // MIDI has priority
    }
    
    void runExample() {
        std::cout << "\n=== Budgeted Loop Example ===" << std::endl;
        std::cout << "Simulating various MIDI/UI load scenarios...\n" << std::endl;
        
        // Scenario 1: Normal operation
        simulateScenario("Normal Load", 
                        std::chrono::microseconds(300),   // MIDI: 300μs
                        std::chrono::milliseconds(8));     // UI: 8ms
        
        // Scenario 2: Heavy MIDI processing
        simulateScenario("Heavy MIDI", 
                        std::chrono::microseconds(2000),  // MIDI: 2ms  
                        std::chrono::milliseconds(5));     // UI: 5ms
        
        // Scenario 3: Heavy UI processing
        simulateScenario("Heavy UI", 
                        std::chrono::microseconds(300),   // MIDI: 300μs
                        std::chrono::milliseconds(25));    // UI: 25ms
        
        // Scenario 4: Both heavy
        simulateScenario("Both Heavy", 
                        std::chrono::microseconds(3000),  // MIDI: 3ms
                        std::chrono::milliseconds(20));    // UI: 20ms
        
        std::cout << "\n" << frame_controller_.getPerformanceReport() << std::endl;
    }

private:
    AdaptiveFrameController frame_controller_;
    SmartUIRenderer ui_renderer_;
    
    void simulateScenario(const std::string& name, 
                         std::chrono::microseconds midi_time,
                         std::chrono::milliseconds ui_time) {
        
        std::cout << "\n--- " << name << " Scenario ---" << std::endl;
        std::cout << "MIDI Time: " << midi_time.count() << "μs, ";
        std::cout << "UI Time: " << ui_time.count() << "ms" << std::endl;
        
        for (int frame = 0; frame < 10; ++frame) {
            simulateFrame(frame, midi_time, ui_time);
        }
    }
    
    void simulateFrame(int frame_num, 
                      std::chrono::microseconds midi_time,
                      std::chrono::milliseconds ui_time) {
        
        auto frame_start = std::chrono::steady_clock::now();
        auto frame_budget = std::chrono::microseconds(16667); // 60fps = 16.67ms
        
        // MIDI processing (always happens)
        std::this_thread::sleep_for(midi_time); // Simulate MIDI work
        
        auto after_midi = std::chrono::steady_clock::now();
        auto midi_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(after_midi - frame_start);
        auto remaining_time = frame_budget - midi_elapsed;
        
        // UI processing decision
        auto decision = frame_controller_.shouldRenderFrame(remaining_time, midi_elapsed);
        
        std::cout << "Frame " << frame_num << ": ";
        
        std::chrono::microseconds actual_ui_time{0};
        
        switch (decision) {
            case AdaptiveFrameController::FrameDecision::RENDER_FULL: {
                auto ui_start = std::chrono::steady_clock::now();
                ui_renderer_.renderFrame(SmartUIRenderer::RenderMode::FULL, remaining_time);
                std::this_thread::sleep_for(ui_time); // Simulate UI work
                auto ui_end = std::chrono::steady_clock::now();
                actual_ui_time = std::chrono::duration_cast<std::chrono::microseconds>(ui_end - ui_start);
                std::cout << "✅ FULL render (" << actual_ui_time.count() << "μs)";
                break;
            }
            
            case AdaptiveFrameController::FrameDecision::RENDER_MINIMAL: {
                auto ui_start = std::chrono::steady_clock::now();
                ui_renderer_.renderFrame(SmartUIRenderer::RenderMode::MINIMAL, remaining_time);
                std::this_thread::sleep_for(ui_time / 4); // Minimal work
                auto ui_end = std::chrono::steady_clock::now();
                actual_ui_time = std::chrono::duration_cast<std::chrono::microseconds>(ui_end - ui_start);
                std::cout << "⚡ MINIMAL render (" << actual_ui_time.count() << "μs)";
                break;
            }
            
            case AdaptiveFrameController::FrameDecision::SKIP_FRAME:
                std::cout << "⏭️ SKIP frame (saved " << ui_time.count() << "ms)";
                break;
        }
        
        frame_controller_.markFrameComplete(decision, actual_ui_time);
        
        // Sleep until next frame
        std::this_thread::sleep_until(frame_start + frame_budget);
    }
};

} // namespace MIDI

// Example usage
void demonstrateFrameSkipping() {
    MIDI::ExampleBudgetedLoop example;
    example.runExample();
}
