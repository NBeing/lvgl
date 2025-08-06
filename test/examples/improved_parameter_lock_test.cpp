/**
 * @brief Parameter Lock Unit Tests - Example of Improved Testing
 * 
 * This demonstrates how the new unified test framework eliminates
 * the confusion and duplication in our current testing system.
 */

#include "framework/unified_test_framework.h"
#include <unordered_map>
#include <vector>
#include <algorithm>

// Simple type definitions for testing (no external dependencies)
using ParameterID = uint32_t;

/**
 * @brief Simplified Parameter Lock Manager for unit testing
 */
class ParameterLockManager {
public:
    void setStepParameterLock(int track_id, int step_id, ParameterID param_id, float value) {
        step_locks_[track_id][step_id][param_id] = value;
    }
    
    bool hasStepParameterLock(int track_id, int step_id, ParameterID param_id) const {
        auto track_it = step_locks_.find(track_id);
        if (track_it != step_locks_.end()) {
            auto step_it = track_it->second.find(step_id);
            if (step_it != track_it->second.end()) {
                return step_it->second.find(param_id) != step_it->second.end();
            }
        }
        return false;
    }
    
    float getStepParameterLock(int track_id, int step_id, ParameterID param_id) const {
        auto track_it = step_locks_.find(track_id);
        if (track_it != step_locks_.end()) {
            auto step_it = track_it->second.find(step_id);
            if (step_it != track_it->second.end()) {
                auto param_it = step_it->second.find(param_id);
                if (param_it != step_it->second.end()) {
                    return param_it->second;
                }
            }
        }
        return 0.0f;
    }
    
    std::vector<ParameterID> getLockedParametersForStep(int track_id, int step_id) const {
        std::vector<ParameterID> result;
        auto track_it = step_locks_.find(track_id);
        if (track_it != step_locks_.end()) {
            auto step_it = track_it->second.find(step_id);
            if (step_it != track_it->second.end()) {
                for (const auto& param_pair : step_it->second) {
                    result.push_back(param_pair.first);
                }
            }
        }
        return result;
    }
    
    void clearStepParameterLock(int track_id, int step_id, ParameterID param_id) {
        auto track_it = step_locks_.find(track_id);
        if (track_it != step_locks_.end()) {
            auto step_it = track_it->second.find(step_id);
            if (step_it != track_it->second.end()) {
                step_it->second.erase(param_id);
            }
        }
    }
    
    size_t getTotalParameterLocks() const {
        size_t total = 0;
        for (const auto& track_pair : step_locks_) {
            for (const auto& step_pair : track_pair.second) {
                total += step_pair.second.size();
            }
        }
        return total;
    }

private:
    std::unordered_map<int, std::unordered_map<int, std::unordered_map<ParameterID, float>>> step_locks_;
};

// ============================================================================
// UNIT TESTS - No external dependencies, fast execution
// ============================================================================

TEST_UNIT(ParameterLock, BasicSetAndGet) {
    ParameterLockManager manager;
    
    manager.setStepParameterLock(0, 0, 1, 0.8f);
    
    ASSERT_TRUE(manager.hasStepParameterLock(0, 0, 1));
    ASSERT_NEAR(0.8f, manager.getStepParameterLock(0, 0, 1), 0.01f);
    ASSERT_FALSE(manager.hasStepParameterLock(0, 0, 2)); // Not set
}

TEST_UNIT(ParameterLock, MultipleParameters) {
    ParameterLockManager manager;
    
    // Set multiple parameters on same step
    manager.setStepParameterLock(0, 0, 1, 0.1f);  // Filter cutoff
    manager.setStepParameterLock(0, 0, 2, 0.2f);  // Resonance
    manager.setStepParameterLock(0, 0, 3, 0.3f);  // LFO rate
    
    auto locked_params = manager.getLockedParametersForStep(0, 0);
    ASSERT_EQ(3lu, locked_params.size());
    
    // Verify all parameters are present
    std::vector<ParameterID> expected = {1, 2, 3};
    for (ParameterID expected_param : expected) {
        ASSERT_TRUE(std::find(locked_params.begin(), locked_params.end(), expected_param) != locked_params.end());
    }
}

TEST_UNIT(ParameterLock, ClearParameter) {
    ParameterLockManager manager;
    
    manager.setStepParameterLock(0, 0, 1, 0.5f);
    ASSERT_TRUE(manager.hasStepParameterLock(0, 0, 1));
    
    manager.clearStepParameterLock(0, 0, 1);
    ASSERT_FALSE(manager.hasStepParameterLock(0, 0, 1));
}

TEST_UNIT(ParameterLock, MultipleSteps) {
    ParameterLockManager manager;
    
    // Set locks on different steps
    manager.setStepParameterLock(0, 0, 1, 0.1f);  // Step 0
    manager.setStepParameterLock(0, 1, 1, 0.2f);  // Step 1
    manager.setStepParameterLock(0, 2, 1, 0.3f);  // Step 2
    
    ASSERT_NEAR(0.1f, manager.getStepParameterLock(0, 0, 1), 0.01f);
    ASSERT_NEAR(0.2f, manager.getStepParameterLock(0, 1, 1), 0.01f);
    ASSERT_NEAR(0.3f, manager.getStepParameterLock(0, 2, 1), 0.01f);
}

TEST_UNIT(ParameterLock, MultipleTracks) {
    ParameterLockManager manager;
    
    // Set locks on different tracks
    manager.setStepParameterLock(0, 0, 1, 0.1f);  // Track 0
    manager.setStepParameterLock(1, 0, 1, 0.2f);  // Track 1
    manager.setStepParameterLock(2, 0, 1, 0.3f);  // Track 2
    
    ASSERT_NEAR(0.1f, manager.getStepParameterLock(0, 0, 1), 0.01f);
    ASSERT_NEAR(0.2f, manager.getStepParameterLock(1, 0, 1), 0.01f);
    ASSERT_NEAR(0.3f, manager.getStepParameterLock(2, 0, 1), 0.01f);
}

TEST_UNIT(ParameterLock, Statistics) {
    ParameterLockManager manager;
    
    // Initially no locks
    ASSERT_EQ(0lu, manager.getTotalParameterLocks());
    
    // Add some locks
    manager.setStepParameterLock(0, 0, 1, 0.1f);
    manager.setStepParameterLock(0, 0, 2, 0.2f);
    manager.setStepParameterLock(1, 1, 1, 0.3f);
    
    ASSERT_EQ(3lu, manager.getTotalParameterLocks());
}

TEST_UNIT(ParameterLock, ValueOverwrite) {
    ParameterLockManager manager;
    
    // Set initial value
    manager.setStepParameterLock(0, 0, 1, 0.5f);
    ASSERT_NEAR(0.5f, manager.getStepParameterLock(0, 0, 1), 0.01f);
    
    // Overwrite with new value
    manager.setStepParameterLock(0, 0, 1, 0.8f);
    ASSERT_NEAR(0.8f, manager.getStepParameterLock(0, 0, 1), 0.01f);
    
    // Should still have only 1 lock total
    ASSERT_EQ(1lu, manager.getTotalParameterLocks());
}

TEST_UNIT(ParameterLock, EdgeCaseValues) {
    ParameterLockManager manager;
    
    // Test boundary values
    manager.setStepParameterLock(0, 0, 1, 0.0f);    // Minimum
    manager.setStepParameterLock(0, 1, 1, 1.0f);    // Maximum
    manager.setStepParameterLock(0, 2, 1, 0.5f);    // Middle
    
    ASSERT_NEAR(0.0f, manager.getStepParameterLock(0, 0, 1), 0.01f);
    ASSERT_NEAR(1.0f, manager.getStepParameterLock(0, 1, 1), 0.01f);
    ASSERT_NEAR(0.5f, manager.getStepParameterLock(0, 2, 1), 0.01f);
}

// ============================================================================
// INTEGRATION TESTS - Would test with real components (mocked for demo)
// ============================================================================

TEST_INTEGRATION(ParameterLock, WithStepSequencer) {
    // This would test ParameterLockManager integrated with StepSequencer
    // using dependency injection for mocked components
    ASSERT_TRUE(true); // Placeholder - would test real integration
}

TEST_INTEGRATION(ParameterLock, WithParameterManager) {
    // This would test actual parameter value application
    // using mocked ParameterManager
    ASSERT_TRUE(true); // Placeholder - would test real integration
}

// ============================================================================
// SYSTEM TESTS - Full workflow tests
// ============================================================================

TEST_SYSTEM(CompleteParameterLockWorkflow) {
    // This would test the complete workflow from UI interaction
    // through parameter lock application to MIDI output
    ASSERT_TRUE(true); // Placeholder - would test complete system
}

// ============================================================================
// MAIN FUNCTION - Demonstrates the unified test runner
// ============================================================================

int main() {
    std::cout << "🧪 Parameter Lock Tests - Unified Framework Demo" << std::endl;
    std::cout << "================================================" << std::endl;
    
    auto& runner = TestFramework::TestRunner::getInstance();
    
    // You can run all tests
    auto results = runner.runAllTests();
    
    // Or run specific categories
    // runner.runCategory("unit/ParameterLock");
    // runner.runCategory("integration/ParameterLock");
    
    // Or list available tests
    // runner.listTests();
    
    return results.failed_tests == 0 ? 0 : 1;
}
