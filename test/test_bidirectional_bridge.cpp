/**
 * @brief Bidirectional MIDI-Parameter Bridge Standalone Test Runner
 * 
 * Tests the complete bidirectional system that bridges MIDI and parameters
 * using the RT-Safe Event Distributor as its foundation
 */

#include <iostream>
#include "test/TestFramework.h"
#include "test/BidirectionalBridgeTests.cpp"

int main() {
    std::cout << "🎹 Bidirectional MIDI-Parameter Bridge Tests" << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "Testing the complete bidirectional MIDI ↔ Parameter system" << std::endl;
    std::cout << "Built on the RT-Safe Event Distributor foundation\n" << std::endl;
    
    // Run the bridge test suite
    runBidirectionalBridgeTests();
    
    return 0;
}
