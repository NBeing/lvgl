/**
 * @brief Simple test runner for BidirectionalBridgeTests
 */

#include "test/BidirectionalBridgeTests.cpp"

int main() {
    std::cout << "🎹 Running Bidirectional MIDI-Parameter Bridge Tests\n" << std::endl;
    
    try {
        runBidirectionalBridgeTests();
        std::cout << "\n✅ All bridge tests completed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "\n❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}
