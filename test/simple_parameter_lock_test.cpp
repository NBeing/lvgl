#include "TestFramework.h"
#include "components/midi/ParameterLockManager.h"
#include "components/midi/StepSequencer.h"
#include <memory>
#include <iostream>
#include <cassert>

using namespace MIDI;

/**
 * @brief Simplified Parameter Lock System Tests
 * Tests core functionality without complex dependencies
 */
class SimpleParameterLockTests {
public:
    static void runAllTests() {
        std::cout << "🔒 Running Simple Parameter Lock Tests..." << std::endl;
        
        testParameterLockManagerBasics();
        testStepSequencerParameterLocks();
        testParameterLockStatistics();
        
        std::cout << "✅ All simple parameter lock tests passed!" << std::endl;
    }
    
private:
    static void testParameterLockManagerBasics() {
        std::cout << "\n🧪 Testing ParameterLockManager basics..." << std::endl;
        
        auto lock_manager = std::make_unique<ParameterLockManager>();
        
        // Test setting parameter locks without ParameterManager
        ParameterID param1 = 1;
        ParameterID param2 = 2;
        
        lock_manager->setStepParameterLock(0, 0, param1, 0.8f);
        lock_manager->setStepParameterLock(0, 1, param2, 0.6f);
        lock_manager->setStepParameterLock(1, 0, param1, 0.9f);
        
        // Verify locks were set
        assert(lock_manager->hasStepParameterLock(0, 0, param1));
        assert(lock_manager->hasStepParameterLock(0, 1, param2));
        assert(lock_manager->hasStepParameterLock(1, 0, param1));
        assert(!lock_manager->hasStepParameterLock(0, 0, param2)); // Not set
        
        // Verify values
        assert(std::abs(lock_manager->getStepParameterLock(0, 0, param1) - 0.8f) < 0.01f);
        assert(std::abs(lock_manager->getStepParameterLock(0, 1, param2) - 0.6f) < 0.01f);
        assert(std::abs(lock_manager->getStepParameterLock(1, 0, param1) - 0.9f) < 0.01f);
        
        // Test clearing locks
        lock_manager->clearStepParameterLock(0, 0, param1);
        assert(!lock_manager->hasStepParameterLock(0, 0, param1));
        
        std::cout << "✅ ParameterLockManager basics test passed!" << std::endl;
    }
    
    static void testStepSequencerParameterLocks() {
        std::cout << "\n🎵 Testing StepSequencer parameter lock API..." << std::endl;
        
        auto sequencer = std::make_unique<StepSequencer>();
        
        // Test parameter lock API
        ParameterID filter_cutoff = 1;
        ParameterID resonance = 2;
        
        sequencer->setStepParameterLock(0, 0, filter_cutoff, 0.9f);
        sequencer->setStepParameterLock(0, 0, resonance, 0.7f);
        sequencer->setStepParameterLock(1, 1, filter_cutoff, 0.5f);
        
        // Verify locks
        assert(sequencer->hasStepParameterLock(0, 0, filter_cutoff));
        assert(sequencer->hasStepParameterLock(0, 0, resonance));
        assert(sequencer->hasStepParameterLock(1, 1, filter_cutoff));
        assert(!sequencer->hasStepParameterLock(1, 1, resonance)); // Not set
        
        // Test values
        assert(std::abs(sequencer->getStepParameterLock(0, 0, filter_cutoff) - 0.9f) < 0.01f);
        assert(std::abs(sequencer->getStepParameterLock(0, 0, resonance) - 0.7f) < 0.01f);
        
        // Test getting all locks for a step
        auto locks = sequencer->getStepParameterLocks(0, 0);
        assert(locks.size() == 2);
        
        // Test clearing all locks for a step
        sequencer->clearAllStepParameterLocks(0, 0);
        assert(!sequencer->hasStepParameterLock(0, 0, filter_cutoff));
        assert(!sequencer->hasStepParameterLock(0, 0, resonance));
        
        // Test copy operations
        sequencer->setStepParameterLock(0, 1, filter_cutoff, 0.3f);
        sequencer->copyStepParameterLocks(0, 1, 1, 2);
        assert(sequencer->hasStepParameterLock(1, 2, filter_cutoff));
        assert(std::abs(sequencer->getStepParameterLock(1, 2, filter_cutoff) - 0.3f) < 0.01f);
        
        std::cout << "✅ StepSequencer parameter lock API test passed!" << std::endl;
    }
    
    static void testParameterLockStatistics() {
        std::cout << "\n📊 Testing parameter lock statistics..." << std::endl;
        
        auto lock_manager = std::make_unique<ParameterLockManager>();
        
        // Initially no locks
        assert(lock_manager->getTotalParameterLocks() == 0);
        
        // Add some locks
        for (int track = 0; track < 4; ++track) {
            for (int step = 0; step < 8; ++step) {
                lock_manager->setStepParameterLock(track, step, 1, 0.5f + track * 0.1f);
                if (step % 2 == 0) {
                    lock_manager->setStepParameterLock(track, step, 2, 0.3f + step * 0.05f);
                }
            }
        }
        
        // Check total locks (4 tracks * 8 steps * 1 param + 4 tracks * 4 steps * 1 param)
        size_t expected_total = (4 * 8 * 1) + (4 * 4 * 1);  // 32 + 16 = 48
        assert(lock_manager->getTotalParameterLocks() == expected_total);
        
        // Test clearing all locks
        lock_manager->clearAllParameterLocks();
        assert(lock_manager->getTotalParameterLocks() == 0);
        
        std::cout << "✅ Parameter lock statistics test passed!" << std::endl;
    }
};

/**
 * @brief Main test runner
 */
int main() {
    std::cout << "🚀 Starting Parameter Lock System Tests" << std::endl;
    std::cout << "=======================================" << std::endl;
    
    try {
        SimpleParameterLockTests::runAllTests();
        
        std::cout << "\n🎉 All parameter lock tests completed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "\n❌ Test failed with unknown exception!" << std::endl;
        return 1;
    }
}
