/**
 * @brief RT-Safe UI Control Integration Test Runner
 * 
 * Tests the complete UI control integration system that connects
 * UI elements (dials, sliders) to the RT-safe parameter system
 */

#include <iostream>
#include "test/TestFramework.h"
#include "test/RTSafeUIControlIntegrationTests.cpp"

int main() {
    std::cout << "🎛️  RT-Safe UI Control Integration Tests" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Testing the complete UI ↔ Parameter integration system" << std::endl;
    std::cout << "Built on the RT-Safe Event Distributor and MIDI Bridge\n" << std::endl;
    
    // Run the UI control integration test suite
    runRTSafeUIControlIntegrationTests();
    
    return 0;
}
