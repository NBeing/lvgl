#include "components/input/InputTest.h"
#include "components/input/SynthInputHandler.h"
#include <iostream>

/**
 * @brief Simple test program for the Input System
 */
int testInputSystem() {
    std::cout << "=== Input System Test Program ===" << std::endl;
    
    try {
        // Run comprehensive tests
        bool tests_passed = Input::Test::TestRunner::runAllTests();
        
        if (tests_passed) {
            std::cout << "✅ All Input System tests PASSED!" << std::endl;
            return 0;
        } else {
            std::cout << "❌ Some Input System tests FAILED!" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ Test exception: " << e.what() << std::endl;
        return 1;
    }
}

// Integration example
void inputSystemExample() {
    std::cout << "\n=== Input System Integration Example ===" << std::endl;
    
    // Initialize the input system
    auto& input_manager = Input::Manager::getInstance();
    input_manager.initialize();
    
    // Create synth input handler
    SynthInputHandler synth_handler;
    synth_handler.initialize();
    
    std::cout << "Input system initialized with default mappings:" << std::endl;
    std::cout << "  SPACE     --> Play/Pause transport" << std::endl;
    std::cout << "  ESC       --> Stop transport" << std::endl;
    std::cout << "  ENTER     --> Continue transport" << std::endl;
    std::cout << "  +/-       --> Increase/Decrease BPM" << std::endl;
    std::cout << "  TAB       --> Next tab" << std::endl;
    std::cout << "  SHIFT+TAB --> Previous tab" << std::endl;
    std::cout << "  F1        --> Help" << std::endl;
    
    // Simulate some key presses
    std::cout << "\nSimulating key presses..." << std::endl;
    
    // Play/pause
    auto play_event = Input::Test::EventBuilder()
        .type(Input::EventType::KEY_PRESS)
        .key(Input::KeyCode::SPACE)
        .build();
    input_manager.injectEvent(play_event);
    
    // Increase BPM
    auto bpm_event = Input::Test::EventBuilder()
        .type(Input::EventType::KEY_PRESS)
        .key(Input::KeyCode::PLUS)
        .build();
    input_manager.injectEvent(bpm_event);
    
    std::cout << "Key simulation complete!" << std::endl;
    
    // Cleanup
    synth_handler.shutdown();
    input_manager.shutdown();
}
