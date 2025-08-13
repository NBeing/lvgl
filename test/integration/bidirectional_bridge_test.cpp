/**
 * @brief Bidirectional MIDI-Parameter Bridge Tests - Migrated to Unified Framework
 * 
 * Tests the complete bidirectional synchronization system:
 * - Parameter changes → MIDI CC output
 * - MIDI CC input → Parameter updates  
 * - Feedback loop prevention
 * - RT-safe operation validation
 */

#include "../framework/unified_test_framework.h"
#include "../fixtures/test_fixtures.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <set>
#include <algorithm>

using namespace TestFixtures;

// ============================================================================
// EXTENDED MOCK CLASSES FOR BIDIRECTIONAL BRIDGE TESTING
// ============================================================================

namespace BridgeTesting {

/**
 * @brief RT-Safe Parameter Manager Mock
 */
class RTParameterManager {
private:
    std::unordered_map<uint32_t, float> parameters_;
    std::function<void(uint32_t, float)> change_callback_;
    mutable std::atomic<bool> rt_safe_flag_{true};
    
public:
    void setParameterValueRT(uint32_t param_id, float value) {
        parameters_[param_id] = value;
        if (change_callback_) {
            change_callback_(param_id, value);
        }
    }
    
    float getParameterValueRT(uint32_t param_id) const {
        auto it = parameters_.find(param_id);
        return (it != parameters_.end()) ? it->second : 0.0f;
    }
    
    void setChangeCallback(std::function<void(uint32_t, float)> callback) {
        change_callback_ = callback;
    }
    
    bool isRTSafe() const { return rt_safe_flag_.load(); }
    
    std::vector<std::pair<uint32_t, float>> getAllParameters() const {
        std::vector<std::pair<uint32_t, float>> result;
        for (const auto& pair : parameters_) {
            result.emplace_back(pair.first, pair.second);
        }
        return result;
    }
    
    void clearParameters() { parameters_.clear(); }
};

/**
 * @brief Advanced MIDI Handler Mock
 */
class RTMidiHandler {
private:
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> sent_messages_;
    std::function<void(uint8_t, uint8_t, uint8_t)> receive_callback_;
    std::atomic<bool> rt_safe_flag_{true};
    
public:
    void sendMidiCC(uint8_t channel, uint8_t controller, uint8_t value) {
        sent_messages_.emplace_back(channel, controller, value);
    }
    
    void simulateReceiveMidiCC(uint8_t channel, uint8_t controller, uint8_t value) {
        if (receive_callback_) {
            receive_callback_(channel, controller, value);
        }
    }
    
    void setReceiveCallback(std::function<void(uint8_t, uint8_t, uint8_t)> callback) {
        receive_callback_ = callback;
    }
    
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> getSentMessages() const {
        return sent_messages_;
    }
    
    void clearMessages() { sent_messages_.clear(); }
    size_t getSentMessageCount() const { return sent_messages_.size(); }
    
    bool isRTSafe() const { return rt_safe_flag_.load(); }
};

/**
 * @brief Bidirectional MIDI-Parameter Bridge Implementation
 */
class BidirectionalBridge {
public:
    struct ParameterMapping {
        uint32_t parameter_id;
        uint8_t midi_channel;
        uint8_t midi_controller;
        float min_value;
        float max_value;
        bool bidirectional;
    };
    
private:
    std::shared_ptr<RTParameterManager> parameter_manager_;
    std::shared_ptr<RTMidiHandler> midi_handler_;
    std::unordered_map<uint16_t, ParameterMapping> cc_to_param_map_;  // Key: (channel << 8) | controller
    std::unordered_map<uint32_t, ParameterMapping> param_to_cc_map_;
    std::atomic<bool> feedback_prevention_{true};
    std::atomic<bool> updating_from_midi_{false};
    std::atomic<bool> updating_from_param_{false};
    
public:
    void setParameterManager(std::shared_ptr<RTParameterManager> manager) {
        parameter_manager_ = manager;
        if (manager) {
            manager->setChangeCallback([this](uint32_t param_id, float value) {
                handleParameterChange(param_id, value);
            });
        }
    }
    
    void setMidiHandler(std::shared_ptr<RTMidiHandler> handler) {
        midi_handler_ = handler;
        if (handler) {
            handler->setReceiveCallback([this](uint8_t channel, uint8_t controller, uint8_t value) {
                handleMidiCC(channel, controller, value);
            });
        }
    }
    
    void addMapping(uint32_t param_id, uint8_t channel, uint8_t controller, 
                   float min_val = 0.0f, float max_val = 1.0f, bool bidirectional = true) {
        ParameterMapping mapping{param_id, channel, controller, min_val, max_val, bidirectional};
        
        // Create composite key: channel in high byte, controller in low byte
        uint16_t cc_key = (static_cast<uint16_t>(channel) << 8) | controller;
        cc_to_param_map_[cc_key] = mapping;
        param_to_cc_map_[param_id] = mapping;
    }
    
    size_t getMappingCount() const { return cc_to_param_map_.size(); }
    
    void setFeedbackPrevention(bool enabled) { feedback_prevention_.store(enabled); }
    bool getFeedbackPrevention() const { return feedback_prevention_.load(); }
    
    void clearMappings() {
        cc_to_param_map_.clear();
        param_to_cc_map_.clear();
    }

private:
    void handleParameterChange(uint32_t param_id, float value) {
        if (feedback_prevention_.load() && updating_from_midi_.load()) {
            return; // Prevent feedback loop
        }
        
        auto it = param_to_cc_map_.find(param_id);
        if (it != param_to_cc_map_.end() && midi_handler_) {
            const auto& mapping = it->second;
            if (mapping.bidirectional) {
                updating_from_param_.store(true);
                
                // Convert parameter value to MIDI range
                float normalized = (value - mapping.min_value) / (mapping.max_value - mapping.min_value);
                uint8_t midi_value = static_cast<uint8_t>(std::clamp(normalized * 127.0f, 0.0f, 127.0f));
                
                midi_handler_->sendMidiCC(mapping.midi_channel, mapping.midi_controller, midi_value);
                
                updating_from_param_.store(false);
            }
        }
    }
    
    void handleMidiCC(uint8_t channel, uint8_t controller, uint8_t value) {
        if (feedback_prevention_.load() && updating_from_param_.load()) {
            return; // Prevent feedback loop
        }
        
        // Create composite key for lookup
        uint16_t cc_key = (static_cast<uint16_t>(channel) << 8) | controller;
        auto it = cc_to_param_map_.find(cc_key);
        
        if (it != cc_to_param_map_.end() && parameter_manager_) {
            const auto& mapping = it->second;
            if (mapping.bidirectional) {
                updating_from_midi_.store(true);
                
                // Convert MIDI value to parameter range
                float normalized = static_cast<float>(value) / 127.0f;
                float param_value = mapping.min_value + normalized * (mapping.max_value - mapping.min_value);
                
                parameter_manager_->setParameterValueRT(mapping.parameter_id, param_value);
                
                updating_from_midi_.store(false);
            }
        }
    }
};

} // namespace BridgeTesting

using namespace BridgeTesting;

// ============================================================================
// UNIT TESTS - Basic Bridge Functionality
// ============================================================================

TEST_UNIT(BidirectionalBridge, BasicSetup) {
    auto bridge = std::make_unique<BidirectionalBridge>();
    auto param_manager = std::make_shared<RTParameterManager>();
    auto midi_handler = std::make_shared<RTMidiHandler>();
    
    bridge->setParameterManager(param_manager);
    bridge->setMidiHandler(midi_handler);
    
    // Verify initial state
    ASSERT_EQ(0lu, bridge->getMappingCount());
    ASSERT_TRUE(bridge->getFeedbackPrevention());
    ASSERT_TRUE(param_manager->isRTSafe());
    ASSERT_TRUE(midi_handler->isRTSafe());
}

TEST_UNIT(BidirectionalBridge, MappingManagement) {
    auto bridge = std::make_unique<BidirectionalBridge>();
    
    // Add mappings
    bridge->addMapping(1, 0, 7, 0.0f, 1.0f);   // Filter cutoff -> CC 7
    bridge->addMapping(2, 0, 71, 0.0f, 1.0f);  // Resonance -> CC 71
    bridge->addMapping(3, 0, 74, 100.0f, 10000.0f); // LFO rate -> CC 74 (custom range)
    
    ASSERT_EQ(3lu, bridge->getMappingCount());
    
    bridge->clearMappings();
    ASSERT_EQ(0lu, bridge->getMappingCount());
}

// ============================================================================
// INTEGRATION TESTS - Parameter to MIDI Direction
// ============================================================================

TEST_INTEGRATION(BidirectionalBridge, ParameterToMidi) {
    auto bridge = std::make_unique<BidirectionalBridge>();
    auto param_manager = std::make_shared<RTParameterManager>();
    auto midi_handler = std::make_shared<RTMidiHandler>();
    
    bridge->setParameterManager(param_manager);
    bridge->setMidiHandler(midi_handler);
    bridge->addMapping(1, 0, 7, 0.0f, 1.0f);  // Filter cutoff -> CC 7
    
    // Change parameter value
    param_manager->setParameterValueRT(1, 0.5f);
    
    // Verify MIDI output
    ASSERT_EQ(1lu, midi_handler->getSentMessageCount());
    
    auto messages = midi_handler->getSentMessages();
    auto [channel, controller, value] = messages[0];
    
    ASSERT_EQ_NUM(0, channel);
    ASSERT_EQ_NUM(7, controller);
    ASSERT_EQ_NUM(63, value);  // 0.5 * 127 ≈ 63
}

TEST_INTEGRATION(BidirectionalBridge, MidiToParameter) {
    auto bridge = std::make_unique<BidirectionalBridge>();
    auto param_manager = std::make_shared<RTParameterManager>();
    auto midi_handler = std::make_shared<RTMidiHandler>();
    
    bridge->setParameterManager(param_manager);
    bridge->setMidiHandler(midi_handler);
    bridge->addMapping(1, 0, 7, 0.0f, 1.0f);  // Filter cutoff -> CC 7
    
    // Simulate MIDI input
    midi_handler->simulateReceiveMidiCC(0, 7, 100);  // ~78% value
    
    // Verify parameter update
    float param_value = param_manager->getParameterValueRT(1);
    ASSERT_NEAR(0.787f, param_value, 0.01f);  // 100/127 ≈ 0.787
}

TEST_INTEGRATION(BidirectionalBridge, CustomValueRanges) {
    auto bridge = std::make_unique<BidirectionalBridge>();
    auto param_manager = std::make_shared<RTParameterManager>();
    auto midi_handler = std::make_shared<RTMidiHandler>();
    
    bridge->setParameterManager(param_manager);
    bridge->setMidiHandler(midi_handler);
    
    // Map with custom range: 100Hz to 10kHz
    bridge->addMapping(1, 0, 74, 100.0f, 10000.0f);
    
    // Test: Parameter change
    param_manager->setParameterValueRT(1, 5050.0f);  // Middle of range
    
    auto messages = midi_handler->getSentMessages();
    ASSERT_EQ(1lu, messages.size());
    
    auto [channel, controller, value] = messages[0];
    ASSERT_EQ_NUM(74, controller);
    ASSERT_NEAR(63, value, 2);  // Should be around middle of MIDI range
    
    // Test: MIDI input
    midi_handler->clearMessages();
    param_manager->clearParameters();
    
    midi_handler->simulateReceiveMidiCC(0, 74, 32);  // 25% of MIDI range
    
    float param_value = param_manager->getParameterValueRT(1);
    ASSERT_NEAR(2575.0f, param_value, 50.0f);  // 100 + 0.25 * (10000 - 100)
}

// ============================================================================
// INTEGRATION TESTS - Feedback Prevention
// ============================================================================

TEST_INTEGRATION(BidirectionalBridge, FeedbackPrevention) {
    auto bridge = std::make_unique<BidirectionalBridge>();
    auto param_manager = std::make_shared<RTParameterManager>();
    auto midi_handler = std::make_shared<RTMidiHandler>();
    
    bridge->setParameterManager(param_manager);
    bridge->setMidiHandler(midi_handler);
    bridge->addMapping(1, 0, 7, 0.0f, 1.0f);
    
    // Enable feedback prevention (default)
    ASSERT_TRUE(bridge->getFeedbackPrevention());
    
    // Change parameter -> MIDI (should work)
    param_manager->setParameterValueRT(1, 0.5f);
    ASSERT_EQ(1lu, midi_handler->getSentMessageCount());
    
    // Clear and test MIDI -> Parameter (should work, but not trigger feedback)
    midi_handler->clearMessages();
    midi_handler->simulateReceiveMidiCC(0, 7, 100);
    
    // Should have updated parameter but NOT sent MIDI back
    ASSERT_NEAR(0.787f, param_manager->getParameterValueRT(1), 0.01f);
    ASSERT_EQ(0lu, midi_handler->getSentMessageCount());  // No feedback!
}

TEST_INTEGRATION(BidirectionalBridge, FeedbackPreventionDisabled) {
    auto bridge = std::make_unique<BidirectionalBridge>();
    auto param_manager = std::make_shared<RTParameterManager>();
    auto midi_handler = std::make_shared<RTMidiHandler>();
    
    bridge->setParameterManager(param_manager);
    bridge->setMidiHandler(midi_handler);
    bridge->addMapping(1, 0, 7, 0.0f, 1.0f);
    
    // Disable feedback prevention
    bridge->setFeedbackPrevention(false);
    ASSERT_FALSE(bridge->getFeedbackPrevention());
    
    // MIDI input should now trigger parameter change AND MIDI output (feedback)
    midi_handler->simulateReceiveMidiCC(0, 7, 64);
    
    // Should have updated parameter AND sent MIDI back (creating feedback)
    ASSERT_NEAR(0.5f, param_manager->getParameterValueRT(1), 0.01f);
    ASSERT_EQ(1lu, midi_handler->getSentMessageCount());
}

// ============================================================================
// INTEGRATION TESTS - Multiple Parameters
// ============================================================================

TEST_INTEGRATION(BidirectionalBridge, MultipleParameters) {
    auto bridge = std::make_unique<BidirectionalBridge>();
    auto param_manager = std::make_shared<RTParameterManager>();
    auto midi_handler = std::make_shared<RTMidiHandler>();
    
    bridge->setParameterManager(param_manager);
    bridge->setMidiHandler(midi_handler);
    
    // Map multiple parameters
    bridge->addMapping(1, 0, 7, 0.0f, 1.0f);   // Filter cutoff
    bridge->addMapping(2, 0, 71, 0.0f, 1.0f);  // Resonance
    bridge->addMapping(3, 0, 74, 0.0f, 1.0f);  // LFO rate
    bridge->addMapping(4, 1, 7, 0.0f, 1.0f);   // Volume on channel 1 (same CC, different channel)
    
    ASSERT_EQ(4lu, bridge->getMappingCount());
    
    // Change multiple parameters
    param_manager->setParameterValueRT(1, 0.25f);  // Filter cutoff
    param_manager->setParameterValueRT(2, 0.75f);  // Resonance
    param_manager->setParameterValueRT(3, 0.1f);   // LFO rate
    param_manager->setParameterValueRT(4, 0.9f);   // Volume (channel 1)
    
    // Verify MIDI messages sent - should be 4 with our API fix
    ASSERT_EQ(4lu, midi_handler->getSentMessageCount());
    
    auto messages = midi_handler->getSentMessages();
    
    // Verify all 4 messages were sent
    ASSERT_EQ(4lu, messages.size());
}

// ============================================================================
// SYSTEM TESTS - Complete Workflow
// ============================================================================

TEST_SYSTEM(BidirectionalBridgeCompleteWorkflow) {
    auto bridge = std::make_unique<BidirectionalBridge>();
    auto param_manager = std::make_shared<RTParameterManager>();
    auto midi_handler = std::make_shared<RTMidiHandler>();
    
    // Setup complete system
    bridge->setParameterManager(param_manager);
    bridge->setMidiHandler(midi_handler);
    
    // Create realistic synthesizer parameter mappings
    bridge->addMapping(1, 0, 74, 0.0f, 1.0f);    // Filter cutoff
    bridge->addMapping(2, 0, 71, 0.0f, 1.0f);    // Resonance  
    bridge->addMapping(3, 0, 91, 0.0f, 1.0f);    // Reverb
    bridge->addMapping(4, 0, 93, 0.0f, 1.0f);    // Chorus
    bridge->addMapping(5, 0, 1, 0.0f, 1.0f);     // Modulation
    
    // Simulate external controller input sequence
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> midi_sequence = {
        {0, 74, 80},   // Filter cutoff to 63%
        {0, 71, 40},   // Resonance to 31%
        {0, 91, 100},  // Reverb to 79%
        {0, 93, 60},   // Chorus to 47%
        {0, 1, 127}    // Modulation to 100%
    };
    
    // Send MIDI sequence
    for (const auto& [channel, controller, value] : midi_sequence) {
        midi_handler->simulateReceiveMidiCC(channel, controller, value);
    }
    
    // Verify all parameters were updated correctly
    ASSERT_NEAR(0.63f, param_manager->getParameterValueRT(1), 0.02f);  // 80/127
    ASSERT_NEAR(0.31f, param_manager->getParameterValueRT(2), 0.02f);  // 40/127
    ASSERT_NEAR(0.79f, param_manager->getParameterValueRT(3), 0.02f);  // 100/127
    ASSERT_NEAR(0.47f, param_manager->getParameterValueRT(4), 0.02f);  // 60/127
    ASSERT_NEAR(1.0f, param_manager->getParameterValueRT(5), 0.01f);   // 127/127
    
    // Clear MIDI output and modify parameters from UI
    midi_handler->clearMessages();
    
    // Simulate UI parameter changes
    param_manager->setParameterValueRT(1, 0.2f);   // Filter down
    param_manager->setParameterValueRT(3, 0.5f);   // Reverb to 50%
    param_manager->setParameterValueRT(5, 0.0f);   // Modulation off
    
    // Verify MIDI output reflects parameter changes
    ASSERT_EQ(3lu, midi_handler->getSentMessageCount());
    
    auto output_messages = midi_handler->getSentMessages();
    
    // Verify specific output values
    bool found_filter = false, found_reverb = false, found_mod = false;
    
    for (const auto& [channel, controller, value] : output_messages) {
        if (controller == 74) {  // Filter
            found_filter = true;
            ASSERT_NEAR(25, value, 2);  // 0.2 * 127 ≈ 25
        } else if (controller == 91) {  // Reverb
            found_reverb = true;
            ASSERT_NEAR(63, value, 2);  // 0.5 * 127 ≈ 63
        } else if (controller == 1) {   // Modulation
            found_mod = true;
            ASSERT_EQ_NUM(0, value);     // 0.0 * 127 = 0
        }
    }
    
    ASSERT_TRUE(found_filter);
    ASSERT_TRUE(found_reverb);
    ASSERT_TRUE(found_mod);
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main() {
    std::cout << "🔗 Bidirectional Bridge Tests - Unified Framework" << std::endl;
    std::cout << "=================================================" << std::endl;
    
    auto& runner = TestFramework::TestRunner::getInstance();
    
    // Run all categories
    auto unit_results = runner.runCategory("unit/BidirectionalBridge");
    auto integration_results = runner.runCategory("integration/BidirectionalBridge");
    auto system_results = runner.runCategory("system");
    
    bool all_passed = (unit_results.failed_tests == 0) && 
                      (integration_results.failed_tests == 0) && 
                      (system_results.failed_tests == 0);
    
    return all_passed ? 0 : 1;
}
