/**
 * @brief Event Visualizer System Standalone Test Runner
 * 
 * Tests the complete event flow visualization system:
 * - RT-safe event tracing
 * - Traced callback registration  
 * - Event flow visualization
 * - UI integration
 */

#include <iostream>
#include "test/TestFramework.h"
#include "test/EventVisualizerTests.cpp"

int main() {
    std::cout << "📊 Event Visualizer System Tests" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "Testing the complete event flow visualization system" << std::endl;
    std::cout << "Desktop-only debugging and development tool\n" << std::endl;
    
    // Run the event visualizer test suite
    runEventVisualizerTests();
    
    return 0;
}
