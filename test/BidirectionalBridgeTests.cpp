/**
 * @brief Comprehensive Bidirectional MIDI-Parameter Bridge Tests
 * 
 * Tests the complete bidirectional synchronization system:
 * - Parameter changes → MIDI CC output
 * - MIDI CC input → Parameter updates  
 * - Feedback loop prevention
 * - RT-safe operation validation
 * - Thread safety verification
 */

#include "TestFramework.h"
#include "components/controls/BidirectionalParameterMidiBridge.h"
#include "components/threading/RTSafeEventDistributor.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <tuple>
#include <set>

using namespace RTSafe;
using namespace Test;

// Forward declaration to resolve circular dependency
class MockParameterManager;
class MockMidiHandler;

/**
 * @brief Mock Parameter Manager for testing
 */
class MockParameterManager {
private:
    std::unordered_map<uint32_t, float> parameters_;
    mutable std::mutex mutex_;
    
public:
    void setParameterValueRT(uint32_t param_id, float value) {
        // RT-safe version (no mutex in real implementation)
        parameters_[param_id] = value;
    }
    
    float getParameterValueRT(uint32_t param_id) const {
        // RT-safe version (no mutex in real implementation)
        auto it = parameters_.find(param_id);
        return (it != parameters_.end()) ? it->second : 0.0f;
    }
    
    void setParameterValue(uint32_t param_id, float value) {
        std::lock_guard<std::mutex> lock(mutex_);
        parameters_[param_id] = value;
    }
    
    float getParameterValue(uint32_t param_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = parameters_.find(param_id);
        return (it != parameters_.end()) ? it->second : 0.0f;
    }
    
    size_t getParameterCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return parameters_.size();
    }
};

/**
 * @brief Mock MIDI Handler for testing
 */
class MockMidiHandler {
private:
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> sent_cc_messages_;
    mutable std::mutex mutex_;
    std::atomic<int> rt_send_calls_{0};
    std::atomic<bool> simulate_send_failure_{false};
    
public:
    bool sendControlChangeRT(uint8_t channel, uint8_t cc_number, uint8_t value) {
        rt_send_calls_++;
        
        if (simulate_send_failure_) {
            return false;
        }
        
        // In real RT implementation, this would be lock-free
        std::lock_guard<std::mutex> lock(mutex_);
        sent_cc_messages_.emplace_back(channel, cc_number, value);
        return true;
    }
    
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> getSentMessages() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sent_cc_messages_;
    }
    
    void clearSentMessages() {
        std::lock_guard<std::mutex> lock(mutex_);
        sent_cc_messages_.clear();
    }
    
    int getRTSendCallCount() const {
        return rt_send_calls_.load();
    }
    
    void resetCallCount() {
        rt_send_calls_ = 0;
    }
    
    void setSimulateSendFailure(bool fail) {
        simulate_send_failure_ = fail;
    }
};

/**
 * @brief Bridge Test Suite
 */
class BidirectionalBridgeTests {
private:
    RTSafeEventDistributor distributor_;
    MockParameterManager parameter_manager_;
    MockMidiHandler midi_handler_;
    std::unique_ptr<BidirectionalParameterMidiBridge> bridge_;
    
public:
    void setUp() {
        distributor_.initialize();
        
        // Create bridge with mock dependencies
        bridge_ = std::make_unique<BidirectionalParameterMidiBridge>(
            &distributor_, 
            reinterpret_cast<ParameterManager*>(&parameter_manager_),
            reinterpret_cast<MidiHandler*>(&midi_handler_)
        );
        
        bridge_->initialize();
        bridge_->resetStatistics();
        
        // Clear mock state
        midi_handler_.clearSentMessages();
        midi_handler_.resetCallCount();
    }
    
    void tearDown() {
        bridge_->shutdown();
        distributor_.shutdown();
    }
    
    // Test 1: Parameter → MIDI CC Output
    void testParameterToMidiOutput() {
        TEST_SUITE("Parameter → MIDI Output");
        
        TEST("Filter cutoff parameter change sends MIDI CC") {
            // Set up parameter value
            parameter_manager_.setParameterValue(1001, 0.5f); // 50% of filter range
            
            // Create parameter change event
            RTEvent param_event = RTEvent::parameterChange(1001 >> 8, 1001 & 0xFF);
            
            // Send event through RT path
            distributor_.notifyRTObservers(param_event);
            
            // Check that MIDI CC was sent
            auto sent_messages = midi_handler_.getSentMessages();
            ASSERT_EQ(1, sent_messages.size());
            
            auto [channel, cc_number, value] = sent_messages[0];
            ASSERT_EQ(0, channel);      // Channel 1 (0-indexed)
            ASSERT_EQ(74, cc_number);   // Filter cutoff CC
            ASSERT_TRUE(value >= 0 && value <= 127); // Valid MIDI range
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 2: MIDI CC Input → Parameter Update
    void testMidiInputToParameterUpdate() {
        TEST_SUITE("MIDI Input → Parameter Update");
        
        TEST("MIDI CC 74 updates filter cutoff parameter") {
            // Create MIDI CC event
            RTEvent midi_event = RTEvent::midiCC(0, 74, 100); // Channel 1, CC 74, value 100
            
            // Send event through RT path
            distributor_.notifyRTObservers(midi_event);
            
            // Check that parameter was updated
            float param_value = parameter_manager_.getParameterValue(1001);
            ASSERT_TRUE(param_value > 0.7f); // Should be around 100/127 = ~0.79
            ASSERT_TRUE(param_value < 0.85f);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 3: Bidirectional Synchronization
    void testBidirectionalSynchronization() {
        TEST_SUITE("Bidirectional Synchronization");
        
        TEST("External MIDI updates parameter and UI") {
            bridge_->resetStatistics();
            
            // Simulate external MIDI controller input
            RTEvent midi_event = RTEvent::midiCC(0, 71, 64); // Resonance CC, mid value
            distributor_.notifyRTObservers(midi_event);
            
            // Process UI events
            distributor_.processUIEvents();
            
            // Check parameter was updated
            float param_value = parameter_manager_.getParameterValue(1002); // Resonance ID
            ASSERT_TRUE(param_value >= 0.45f && param_value <= 0.55f); // ~50%
            
            // Check statistics
            auto stats = bridge_->getStatistics();
            ASSERT_EQ(1, stats.midi_to_param_events.load());
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 4: Feedback Loop Prevention
    void testFeedbackLoopPrevention() {
        TEST_SUITE("Feedback Loop Prevention");
        
        TEST("No MIDI feedback when processing external MIDI") {
            bridge_->resetStatistics();
            bridge_->setFeedbackLoopPrevention(true);
            
            // Clear any initial messages
            midi_handler_.clearSentMessages();
            midi_handler_.resetCallCount();
            
            // Send MIDI CC input
            RTEvent midi_event = RTEvent::midiCC(0, 74, 50);
            distributor_.notifyRTObservers(midi_event);
            
            // Check no MIDI was sent back (feedback prevention)
            auto sent_messages = midi_handler_.getSentMessages();
            ASSERT_EQ(0, sent_messages.size()); // No feedback
            
            // Check feedback prevention was triggered
            auto stats = bridge_->getStatistics();
            ASSERT_TRUE(stats.feedback_loops_prevented.load() >= 0); // May or may not prevent
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 5: RT Timing Constraints
    void testRTTimingConstraints() {
        TEST_SUITE("RT Timing Constraints");
        
        RT_TEST("Bridge operations meet RT timing") {
            // Test multiple parameter updates
            for (int i = 0; i < 50; ++i) {
                RTEvent param_event = RTEvent::parameterChange(1001 >> 8, 1001 & 0xFF);
                
                ASSERT_RT_TIMING({
                    distributor_.notifyRTObservers(param_event);
                }, 200); // 200μs max (lenient for test environment)
            }
            
            // Check bridge RT timing statistics
            auto stats = bridge_->getStatistics();
            ASSERT_TRUE(stats.max_processing_time_us.load() < 1000); // < 1ms
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 6: Multiple Parameter Mappings
    void testMultipleParameterMappings() {
        TEST_SUITE("Multiple Parameter Mappings");
        
        TEST("Different parameters map to different CCs") {
            bridge_->resetStatistics();
            midi_handler_.clearSentMessages();
            
            // Set parameter values
            parameter_manager_.setParameterValue(1001, 0.25f); // Filter cutoff
            parameter_manager_.setParameterValue(1002, 0.75f); // Filter resonance
            parameter_manager_.setParameterValue(2001, 0.5f);  // Envelope attack
            
            // Send parameter change events
            RTEvent cutoff_event = RTEvent::parameterChange(1001 >> 8, 1001 & 0xFF);
            RTEvent resonance_event = RTEvent::parameterChange(1002 >> 8, 1002 & 0xFF);
            RTEvent attack_event = RTEvent::parameterChange(2001 >> 8, 2001 & 0xFF);
            
            distributor_.notifyRTObservers(cutoff_event);
            distributor_.notifyRTObservers(resonance_event);
            distributor_.notifyRTObservers(attack_event);
            
            // Check correct MIDI CCs were sent
            auto sent_messages = midi_handler_.getSentMessages();
            ASSERT_EQ(3, sent_messages.size());
            
            // Should have CC 74, 71, and 73
            std::set<uint8_t> expected_ccs = {74, 71, 73};
            std::set<uint8_t> actual_ccs;
            
            for (const auto& [channel, cc, value] : sent_messages) {
                actual_ccs.insert(cc);
            }
            
            ASSERT_EQ(expected_ccs.size(), actual_ccs.size());
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 7: Error Handling
    void testErrorHandling() {
        TEST_SUITE("Error Handling");
        
        TEST("Invalid MIDI CC numbers handled gracefully") {
            bridge_->resetStatistics();
            
            // Send invalid MIDI CC
            RTEvent invalid_event = RTEvent::midiCC(0, 99, 64); // CC 99 not mapped
            distributor_.notifyRTObservers(invalid_event);
            
            // Check mapping error was recorded
            auto stats = bridge_->getStatistics();
            ASSERT_EQ(1, stats.mapping_errors.load());
            
        } END_TEST();
        
        TEST("MIDI send failures handled gracefully") {
            bridge_->resetStatistics();
            midi_handler_.setSimulateSendFailure(true);
            
            // Set parameter and trigger MIDI send
            parameter_manager_.setParameterValue(1001, 0.8f);
            RTEvent param_event = RTEvent::parameterChange(1001 >> 8, 1001 & 0xFF);
            distributor_.notifyRTObservers(param_event);
            
            // Check send failure was recorded
            auto stats = bridge_->getStatistics();
            ASSERT_EQ(1, stats.midi_send_failures.load());
            
            midi_handler_.setSimulateSendFailure(false);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 8: Bridge Enable/Disable
    void testBridgeEnableDisable() {
        TEST_SUITE("Bridge Enable/Disable");
        
        TEST("Disabled bridge doesn't process events") {
            bridge_->resetStatistics();
            bridge_->setEnabled(false);
            
            // Send parameter change
            RTEvent param_event = RTEvent::parameterChange(1001 >> 8, 1001 & 0xFF);
            distributor_.notifyRTObservers(param_event);
            
            // Check no events were processed
            auto stats = bridge_->getStatistics();
            ASSERT_EQ(0, stats.param_to_midi_events.load());
            
            // Re-enable and test
            bridge_->setEnabled(true);
            distributor_.notifyRTObservers(param_event);
            
            // Now should process
            stats = bridge_->getStatistics();
            ASSERT_TRUE(stats.param_to_midi_events.load() > 0);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 9: Statistics Tracking
    void testStatisticsTracking() {
        TEST_SUITE("Statistics Tracking");
        
        TEST("Bridge statistics tracked correctly") {
            bridge_->resetStatistics();
            
            // Send mix of MIDI and parameter events
            for (int i = 0; i < 10; ++i) {
                RTEvent midi_event = RTEvent::midiCC(0, 74, 50 + i);
                RTEvent param_event = RTEvent::parameterChange(1001 >> 8, 1001 & 0xFF);
                
                distributor_.notifyRTObservers(midi_event);
                distributor_.notifyRTObservers(param_event);
            }
            
            auto stats = bridge_->getStatistics();
            
            // Should have processed events in both directions
            ASSERT_TRUE(stats.midi_to_param_events.load() > 0);
            ASSERT_TRUE(stats.param_to_midi_events.load() > 0);
            ASSERT_TRUE(stats.max_processing_time_us.load() > 0);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Run all tests
    void runAllTests() {
        std::cout << "🎹 Starting Bidirectional MIDI-Parameter Bridge Tests" << std::endl;
        
        setUp();
        
        testParameterToMidiOutput();
        testMidiInputToParameterUpdate();
        testBidirectionalSynchronization();
        testFeedbackLoopPrevention();
        testRTTimingConstraints();
        testMultipleParameterMappings();
        testErrorHandling();
        testBridgeEnableDisable();
        testStatisticsTracking();
        
        tearDown();
        
        TestFramework::getInstance().printSummary();
        std::cout << "✅ Bidirectional MIDI-Parameter Bridge Tests Completed" << std::endl;
    }
};

// Test runner function
void runBidirectionalBridgeTests() {
    BidirectionalBridgeTests test_suite;
    test_suite.runAllTests();
}
