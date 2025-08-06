/**
 * @brief Event Visualizer Test Runner
 * 
 * Compiles and runs the event visualizer tests to verify:
 * - RT-safe event tracing works correctly
 * - Traced callback macros function properly
 * - High-frequency event handling performs well
 * - Thread safety under concurrent load
 * - Event flow visualization integration
 */

#include "TestFramework.h"
#include "SimpleRTEventTracerTests.cpp"
#include <iostream>

int main() {
    std::cout << "🎵 Event Visualizer Test Suite" << std::endl;
    std::cout << "===============================" << std::endl;
    
    #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
    std::cout << "✅ Desktop build with event visualizer enabled" << std::endl;
    std::cout << "🔍 Testing RT-safe event tracing and visualization system..." << std::endl;
    #else
    std::cout << "⚠️ Event visualizer disabled (ESP32 build or visualizer not enabled)" << std::endl;
    std::cout << "💡 To run full tests, compile with: -DDESKTOP_BUILD=ON -DENABLE_EVENT_VISUALIZER=ON" << std::endl;
    #endif
    
    std::cout << std::endl;
    
    // Run simple event tracer tests (no LVGL dependencies)
    runSimpleRTEventTracerTests();
    
    std::cout << std::endl;
    std::cout << "🎯 Event Visualizer Test Suite Complete!" << std::endl;
    
    return 0;
}
