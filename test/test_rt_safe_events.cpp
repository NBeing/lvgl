/**
 * @brief RT-Safe Event Distributor Standalone Test Runner
 * 
 * Tests the RT-safe observer notification system independently
 * without requiring the full LVGL synthesizer application
 */

#include <iostream>
#include "test/TestFramework.h"
#include "test/RTSafeEventDistributorTestsSimple.cpp"

int main() {
    std::cout << "🧵 RT-Safe Event Distributor Tests" << std::endl;
    std::cout << "===================================" << std::endl;
    
    // Run the simplified test suite
    runRTSafeEventDistributorTests();
    
    return 0;
}
