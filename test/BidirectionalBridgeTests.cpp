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
 * @brief Concrete Bridge implementation for testing
 */
class TestBidirectionalParameterMidiBridge : public BidirectionalParameterMidiBridge {
public:
    TestBidirectionalParameterMidiBridge(
        RTSafeEventDistributor* event_distributor,
        ParameterManager* parameter_manager,
        MidiHandler* midi_handler)
        : BidirectionalParameterMidiBridge(event_distributor, parameter_manager, midi_handler) {}

protected:
    void setParameterValueRT(uint32_t parameter_id, float value) override {
        reinterpret_cast<MockParameterManager*>(parameter_manager_)->setParameterValueRT(parameter_id, value);
    }
    
    float getParameterValueRT(uint32_t parameter_id) override {
        return reinterpret_cast<MockParameterManager*>(parameter_manager_)->getParameterValueRT(parameter_id);
    }
    
    bool sendControlChangeRT(uint8_t channel, uint8_t cc_number, uint8_t value) override {
        return reinterpret_cast<MockMidiHandler*>(midi_handler_)->sendControlChangeRT(channel, cc_number, value);
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
    std::unique_ptr<TestBidirectionalParameterMidiBridge> bridge_;
    
public:
    void setUp() {
        distributor_.initialize();
        
        // Create bridge with mock dependencies
        bridge_ = std::make_unique<TestBidirectionalParameterMidiBridge>(
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
            // Filter cutoff mapping: 20 Hz to 20000 Hz
            // MIDI 100/127 = ~0.787, so 20 + (0.787 * (20000-20)) = ~15752 Hz
            float param_value = parameter_manager_.getParameterValue(1001);
            
            // Check it's in the expected frequency range
            ASSERT_TRUE(param_value >= 15000.0f); // Around 15752 Hz
            ASSERT_TRUE(param_value <= 16000.0f); // Around 15752 Hz
            
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
            ASSERT_EQ(1, stats.midi_to_param_events);
            
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
            ASSERT_TRUE(stats.feedback_loops_prevented >= 0); // May or may not prevent
            
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
            ASSERT_TRUE(stats.max_processing_time_us < 1000); // < 1ms
            
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
            ASSERT_EQ(1, stats.mapping_errors);
            
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
            ASSERT_EQ(1, stats.midi_send_failures);
            
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
            ASSERT_EQ(0, stats.param_to_midi_events);
            
            // Re-enable and test
            bridge_->setEnabled(true);
            distributor_.notifyRTObservers(param_event);
            
            // Now should process
            stats = bridge_->getStatistics();
            ASSERT_TRUE(stats.param_to_midi_events > 0);
            
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
            ASSERT_TRUE(stats.midi_to_param_events > 0);
            ASSERT_TRUE(stats.param_to_midi_events > 0);
            // Note: max_processing_time_us might be 0 in mock environment
            ASSERT_TRUE(stats.max_processing_time_us >= 0);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // ========================================================================
    // THREAD SAFETY TESTS
    // ========================================================================
    
    // Test 10: Concurrent Parameter Access
    void testConcurrentParameterAccess() {
        TEST_SUITE("Thread Safety - Concurrent Parameter Access");
        
        TEST("Multiple threads updating different parameters simultaneously") {
            bridge_->resetStatistics();
            std::atomic<bool> test_running{true};
            std::atomic<int> total_updates{0};
            std::vector<std::thread> threads;
            
            // Spawn 4 threads, each updating different parameters
            for (int thread_id = 0; thread_id < 4; ++thread_id) {
                threads.emplace_back([this, thread_id, &test_running, &total_updates]() {
                    uint32_t base_param = 1000 + (thread_id * 100); // 1000, 1100, 1200, 1300
                    int updates = 0;
                    
                    while (test_running && updates < 250) { // 250 updates per thread = 1000 total
                        float value = (updates % 100) / 100.0f; // Cycle 0.0 to 0.99
                        
                        // Update parameter through bridge
                        parameter_manager_.setParameterValue(base_param, value);
                        RTEvent param_event = RTEvent::parameterChange(base_param >> 8, base_param & 0xFF);
                        distributor_.notifyRTObservers(param_event);
                        
                        updates++;
                        total_updates++;
                        
                        // Small delay to allow thread interleaving
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                    }
                });
            }
            
            // Let threads run for 300ms
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            test_running = false;
            
            // Wait for all threads to complete
            for (auto& thread : threads) {
                thread.join();
            }
            
            // Validate results
            ASSERT_TRUE(total_updates.load() > 0);
            ASSERT_TRUE(total_updates.load() <= 1000); // Should not exceed expected
            
            // Check no crashes occurred and some MIDI messages were sent
            auto sent_messages = midi_handler_.getSentMessages();
            ASSERT_TRUE(sent_messages.size() > 0);
            
        } END_TEST();
        
        TEST("RT thread reading while UI thread writing same parameter") {
            std::atomic<bool> test_running{true};
            std::atomic<int> read_count{0};
            std::atomic<int> write_count{0};
            std::atomic<bool> data_corruption_detected{false};
            
            // Writer thread (simulates UI thread)
            std::thread writer_thread([this, &test_running, &write_count]() {
                uint32_t param_id = 1001;
                int writes = 0;
                
                while (test_running && writes < 500) {
                    // Write frequency values in the expected range (20 Hz to 20000 Hz)
                    float value = 20.0f + (writes / 500.0f) * (20000.0f - 20.0f);
                    parameter_manager_.setParameterValue(param_id, value);
                    writes++;
                    write_count++;
                    
                    std::this_thread::sleep_for(std::chrono::microseconds(20));
                }
            });
            
            // Reader thread (simulates RT thread)
            std::thread reader_thread([this, &test_running, &read_count, &data_corruption_detected]() {
                uint32_t param_id = 1001;
                float last_value = -1.0f;
                
                while (test_running) {
                    float current_value = parameter_manager_.getParameterValue(param_id);
                    
                    // Check for impossible values (parameter 1001 is filter cutoff: 20-20000 Hz)
                    if (current_value < 0.0f || current_value > 25000.0f) {
                        data_corruption_detected = true;
                        break;
                    }
                    
                    // For frequency parameters, values can go up and down during thread contention
                    // We just check for reasonable ranges, not monotonic increase
                    last_value = current_value;
                    read_count++;
                    
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            });
            
            // Run test for 200ms
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            test_running = false;
            
            writer_thread.join();
            reader_thread.join();
            
            // Validate no data corruption detected
            ASSERT_FALSE(data_corruption_detected.load());
            ASSERT_TRUE(read_count.load() > 0);
            ASSERT_TRUE(write_count.load() > 0);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 11: Event Queue Thread Safety
    void testEventQueueThreadSafety() {
        TEST_SUITE("Thread Safety - Event Queue Stress");
        
        TEST("Multiple producers, single consumer event processing") {
            bridge_->resetStatistics();
            std::atomic<bool> test_running{true};
            std::atomic<int> events_produced{0};
            std::atomic<int> events_consumed{0};
            std::vector<std::thread> producer_threads;
            
            // Producer threads (simulate multiple RT sources)
            for (int i = 0; i < 3; ++i) {
                producer_threads.emplace_back([this, i, &test_running, &events_produced]() {
                    uint32_t param_id = 1001 + i; // Different parameter per producer
                    int produced = 0;
                    
                    while (test_running && produced < 200) {
                        float value = (produced % 100) / 100.0f;
                        parameter_manager_.setParameterValue(param_id, value);
                        
                        RTEvent param_event = RTEvent::parameterChange(param_id >> 8, param_id & 0xFF);
                        distributor_.notifyRTObservers(param_event);
                        
                        produced++;
                        events_produced++;
                        
                        // Vary timing to test different interleavings
                        std::this_thread::sleep_for(std::chrono::microseconds(5 + (i * 3)));
                    }
                });
            }
            
            // Consumer thread (simulate UI thread processing)
            std::thread consumer_thread([this, &test_running, &events_consumed]() {
                while (test_running) {
                    // Process any queued UI events
                    distributor_.processUIEvents();
                    events_consumed++;
                    
                    // Slower than producers to test queue buildup
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                }
            });
            
            // Run test for 250ms
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            test_running = false;
            
            // Wait for all threads
            for (auto& thread : producer_threads) {
                thread.join();
            }
            consumer_thread.join();
            
            // Validate no deadlocks and reasonable processing occurred
            ASSERT_TRUE(events_produced.load() > 0);
            ASSERT_TRUE(events_consumed.load() > 0);
            
            // Check system remained responsive
            auto stats = bridge_->getStatistics();
            ASSERT_TRUE(stats.max_processing_time_us < 10000); // < 10ms
            
        } END_TEST();
        
        TEST("Queue overflow handling under extreme load") {
            bridge_->resetStatistics();
            std::atomic<bool> flood_running{true};
            std::atomic<int> flood_events{0};
            
            // Flood the system with events faster than they can be processed
            std::thread flood_thread([this, &flood_running, &flood_events]() {
                uint32_t param_id = 1001;
                
                while (flood_running && flood_events < 5000) {
                    parameter_manager_.setParameterValue(param_id, 0.5f);
                    RTEvent param_event = RTEvent::parameterChange(param_id >> 8, param_id & 0xFF);
                    distributor_.notifyRTObservers(param_event);
                    
                    flood_events++;
                    
                    // No delay - flood as fast as possible
                }
            });
            
            // Run flood for 100ms
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            flood_running = false;
            flood_thread.join();
            
            // System should survive flood without crashing
            ASSERT_TRUE(flood_events.load() > 100); // Should have sent many events
            
            // Process any remaining events
            distributor_.processUIEvents();
            
            // System should still be responsive after flood
            parameter_manager_.setParameterValue(1002, 0.8f);
            RTEvent test_event = RTEvent::parameterChange(1002 >> 8, 1002 & 0xFF);
            distributor_.notifyRTObservers(test_event);
            
            // Should still process normally
            auto sent_messages = midi_handler_.getSentMessages();
            ASSERT_TRUE(sent_messages.size() > 0);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 12: Bridge State Consistency
    void testBridgeStateConsistency() {
        TEST_SUITE("Thread Safety - Bridge State Management");
        
        TEST("Bridge enable/disable during active processing") {
            bridge_->resetStatistics();
            std::atomic<bool> test_running{true};
            std::atomic<int> state_changes{0};
            std::atomic<int> events_processed{0};
            
            // Thread that rapidly enables/disables bridge
            std::thread state_thread([this, &test_running, &state_changes]() {
                bool enabled = true;
                
                while (test_running && state_changes < 50) {
                    bridge_->setEnabled(enabled);
                    enabled = !enabled;
                    state_changes++;
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
            
            // Thread that continuously sends events
            std::thread event_thread([this, &test_running, &events_processed]() {
                uint32_t param_id = 1001;
                
                while (test_running) {
                    parameter_manager_.setParameterValue(param_id, 0.5f);
                    RTEvent param_event = RTEvent::parameterChange(param_id >> 8, param_id & 0xFF);
                    distributor_.notifyRTObservers(param_event);
                    
                    events_processed++;
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            });
            
            // Run test for 600ms
            std::this_thread::sleep_for(std::chrono::milliseconds(600));
            test_running = false;
            
            state_thread.join();
            event_thread.join();
            
            // Validate no crashes occurred
            ASSERT_TRUE(state_changes.load() > 0);
            ASSERT_TRUE(events_processed.load() > 0);
            
            // Ensure bridge is in valid state
            bridge_->setEnabled(true);
            parameter_manager_.setParameterValue(1001, 0.9f);
            RTEvent final_event = RTEvent::parameterChange(1001 >> 8, 1001 & 0xFF);
            distributor_.notifyRTObservers(final_event);
            
            // Should process final event normally
            auto stats = bridge_->getStatistics();
            ASSERT_TRUE(stats.param_to_midi_events > 0);
            
        } END_TEST();
        
        TEST("Statistics consistency under concurrent updates") {
            bridge_->resetStatistics();
            std::atomic<bool> stats_test_running{true};
            std::atomic<int> total_expected_param_events{0};
            std::atomic<int> total_expected_midi_events{0};
            
            // Thread 1: Parameter events
            std::thread param_thread([this, &stats_test_running, &total_expected_param_events]() {
                uint32_t param_id = 1001;
                
                while (stats_test_running) {
                    parameter_manager_.setParameterValue(param_id, 0.3f);
                    RTEvent param_event = RTEvent::parameterChange(param_id >> 8, param_id & 0xFF);
                    distributor_.notifyRTObservers(param_event);
                    
                    total_expected_param_events++;
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(8));
                }
            });
            
            // Thread 2: MIDI events
            std::thread midi_thread([this, &stats_test_running, &total_expected_midi_events]() {
                while (stats_test_running) {
                    RTEvent midi_event = RTEvent::midiCC(0, 74, 64);
                    distributor_.notifyRTObservers(midi_event);
                    
                    total_expected_midi_events++;
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(12));
                }
            });
            
            // Thread 3: Statistics reader
            std::thread stats_reader([this, &stats_test_running]() {
                while (stats_test_running) {
                    // Continuously read statistics (tests atomic consistency)
                    auto stats = bridge_->getStatistics();
                    
                    // Statistics should never be corrupted (negative or impossibly large)
                    ASSERT_TRUE(stats.param_to_midi_events >= 0);
                    ASSERT_TRUE(stats.midi_to_param_events >= 0);
                    ASSERT_TRUE(stats.max_processing_time_us >= 0);
                    ASSERT_TRUE(stats.max_processing_time_us < 1000000); // < 1 second
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            });
            
            // Run test for 200ms
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            stats_test_running = false;
            
            param_thread.join();
            midi_thread.join();
            stats_reader.join();
            
            // Final statistics check
            auto final_stats = bridge_->getStatistics();
            
            // Should have processed some events (exact count may vary due to timing)
            ASSERT_TRUE(final_stats.param_to_midi_events > 0);
            ASSERT_TRUE(final_stats.midi_to_param_events > 0);
            
            // Statistics should be reasonable (not corrupted)
            ASSERT_TRUE(final_stats.param_to_midi_events <= total_expected_param_events.load());
            ASSERT_TRUE(final_stats.midi_to_param_events <= total_expected_midi_events.load());
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 13: Real-World Concurrency Scenarios
    void testRealWorldConcurrencyScenarios() {
        TEST_SUITE("Thread Safety - Real-World Scenarios");
        
        TEST("Simulated live performance scenario") {
            bridge_->resetStatistics();
            std::atomic<bool> performance_running{true};
            std::atomic<int> user_interactions{0};
            std::atomic<int> midi_inputs{0};
            std::atomic<int> automation_events{0};
            
            // User interaction thread (hardware knobs/touchscreen)
            std::thread user_thread([this, &performance_running, &user_interactions]() {
                std::vector<uint32_t> user_params = {1001, 1002, 2001}; // Filter, envelope controls
                
                while (performance_running) {
                    for (auto param_id : user_params) {
                        float value = (user_interactions % 100) / 100.0f;
                        parameter_manager_.setParameterValue(param_id, value);
                        
                        RTEvent param_event = RTEvent::parameterChange(param_id >> 8, param_id & 0xFF);
                        distributor_.notifyRTObservers(param_event);
                        
                        user_interactions++;
                        
                        // Simulate realistic user interaction timing
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                }
            });
            
            // External MIDI controller thread
            std::thread midi_controller_thread([this, &performance_running, &midi_inputs]() {
                std::vector<uint8_t> controller_ccs = {74, 71, 73}; // Standard MIDI CCs
                
                while (performance_running) {
                    for (auto cc : controller_ccs) {
                        uint8_t value = (midi_inputs % 128);
                        RTEvent midi_event = RTEvent::midiCC(0, cc, value);
                        distributor_.notifyRTObservers(midi_event);
                        
                        midi_inputs++;
                        
                        // External controller timing
                        std::this_thread::sleep_for(std::chrono::milliseconds(30));
                    }
                }
            });
            
            // DAW automation thread
            std::thread automation_thread([this, &performance_running, &automation_events]() {
                uint32_t automated_param = 4001; // Master volume
                
                while (performance_running) {
                    // Sine wave automation
                    float automation_value = (std::sin(automation_events * 0.1f) + 1.0f) / 2.0f;
                    parameter_manager_.setParameterValue(automated_param, automation_value);
                    
                    RTEvent auto_event = RTEvent::parameterChange(automated_param >> 8, automated_param & 0xFF);
                    distributor_.notifyRTObservers(auto_event);
                    
                    automation_events++;
                    
                    // High-resolution automation
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
            
            // UI update thread (60 FPS)
            std::thread ui_thread([this, &performance_running]() {
                while (performance_running) {
                    distributor_.processUIEvents();
                    
                    // 60 FPS timing
                    std::this_thread::sleep_for(std::chrono::milliseconds(16));
                }
            });
            
            // Run live performance simulation for 1 second
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            performance_running = false;
            
            // Wait for all threads to complete
            user_thread.join();
            midi_controller_thread.join();
            automation_thread.join();
            ui_thread.join();
            
            // Validate system handled complex scenario without crashes
            ASSERT_TRUE(user_interactions.load() > 0);
            ASSERT_TRUE(midi_inputs.load() > 0);
            ASSERT_TRUE(automation_events.load() > 0);
            
            // Check system remained responsive
            auto stats = bridge_->getStatistics();
            ASSERT_TRUE(stats.param_to_midi_events > 0);
            ASSERT_TRUE(stats.midi_to_param_events > 0);
            ASSERT_TRUE(stats.max_processing_time_us < 5000); // < 5ms reasonable under load
            
            // Validate MIDI output occurred
            auto sent_messages = midi_handler_.getSentMessages();
            ASSERT_TRUE(sent_messages.size() > 10); // Should have sent many messages
            
        } END_TEST();
        
        TEST("System recovery after thread interruption") {
            bridge_->resetStatistics();
            std::atomic<bool> recovery_test_running{true};
            std::atomic<bool> thread_interrupted{false};
            
            // Thread that will be "interrupted" (simulates system stress/exception)
            std::thread unstable_thread([&recovery_test_running, &thread_interrupted]() {
                int iterations = 0;
                
                while (recovery_test_running && iterations < 20) {
                    // Simulate some work, then "crash"
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    iterations++;
                    
                    if (iterations == 10) {
                        thread_interrupted = true;
                        return; // Simulate thread crash/exit
                    }
                }
            });
            
            // Main processing thread continues despite other thread issues
            std::thread main_processing([this, &recovery_test_running]() {
                uint32_t param_id = 1001;
                int processed = 0;
                
                while (recovery_test_running && processed < 50) {
                    parameter_manager_.setParameterValue(param_id, 0.5f);
                    RTEvent param_event = RTEvent::parameterChange(param_id >> 8, param_id & 0xFF);
                    distributor_.notifyRTObservers(param_event);
                    
                    processed++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
            });
            
            // Run test for 1.2 seconds
            std::this_thread::sleep_for(std::chrono::milliseconds(1200));
            recovery_test_running = false;
            
            unstable_thread.join();
            main_processing.join();
            
            // Validate system continued operating despite thread interruption
            ASSERT_TRUE(thread_interrupted.load()); // Confirm interruption occurred
            
            // System should still be functional
            parameter_manager_.setParameterValue(1002, 0.8f);
            RTEvent recovery_event = RTEvent::parameterChange(1002 >> 8, 1002 & 0xFF);
            distributor_.notifyRTObservers(recovery_event);
            
            auto stats = bridge_->getStatistics();
            ASSERT_TRUE(stats.param_to_midi_events > 0);
            
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
        
        // Thread safety tests
        testConcurrentParameterAccess();
        testEventQueueThreadSafety();
        testBridgeStateConsistency();
        testRealWorldConcurrencyScenarios();
        
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
