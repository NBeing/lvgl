/**
 * @brief MIDI-Parameter Integration Tests - Migrated to Unified Framework
 * 
 * Tests integration between MIDI system and parameter management
 */

#include "../framework/unified_test_framework.h"
#include "../fixtures/test_fixtures.h"
#include <functional>
#include <map>

using namespace TestFixtures;

// Integration layer components for testing
namespace Integration {

/**
 * @brief Bidirectional MIDI-Parameter Bridge (Simplified for Testing)
 */
class MidiParameterBridge {
public:
    struct ParameterMapping {
        uint32_t parameter_id;
        uint8_t midi_cc;
        uint8_t midi_channel;
        float min_value;
        float max_value;
    };
    
    void addMapping(uint32_t param_id, uint8_t cc, uint8_t channel = 0, 
                   float min_val = 0.0f, float max_val = 1.0f) {
        // Create composite key: channel in high byte, CC in low byte
        uint16_t mapping_key = (static_cast<uint16_t>(channel) << 8) | cc;
        mappings_[mapping_key] = {param_id, cc, channel, min_val, max_val};
    }
    
    void setMidiInterface(std::shared_ptr<MockMidiInterface> midi) {
        midi_interface_ = midi;
        if (midi) {
            midi->setMessageCallback([this](uint8_t status, uint8_t data1, uint8_t data2) {
                handleMidiMessage(status, data1, data2);
            });
        }
    }
    
    void setParameterManager(std::shared_ptr<MockParameterManager> params) {
        parameter_manager_ = params;
        if (params) {
            params->setChangeCallback([this](uint32_t param_id, float value) {
                handleParameterChange(param_id, value);
            });
        }
    }
    
    size_t getMappingCount() const { return mappings_.size(); }
    
    bool hasMapping(uint8_t cc, uint8_t channel = 0) const { 
        uint16_t mapping_key = (static_cast<uint16_t>(channel) << 8) | cc;
        return mappings_.find(mapping_key) != mappings_.end(); 
    }
    
    void clearMappings() { mappings_.clear(); }

private:
    std::map<uint16_t, ParameterMapping> mappings_;  // Key: (channel << 8) | cc
    std::shared_ptr<MockMidiInterface> midi_interface_;
    std::shared_ptr<MockParameterManager> parameter_manager_;
    
    void handleMidiMessage(uint8_t status, uint8_t data1, uint8_t data2) {
        // Handle CC messages
        if ((status & 0xF0) == 0xB0) {  // Control Change
            uint8_t channel = status & 0x0F;  // Extract channel from status
            uint8_t cc = data1;
            uint8_t value = data2;
            
            // Create composite key for lookup
            uint16_t mapping_key = (static_cast<uint16_t>(channel) << 8) | cc;
            auto mapping_it = mappings_.find(mapping_key);
            
            if (mapping_it != mappings_.end() && parameter_manager_) {
                const auto& mapping = mapping_it->second;
                
                // Convert MIDI value (0-127) to parameter range
                float normalized = (float)value / 127.0f;
                float param_value = mapping.min_value + normalized * (mapping.max_value - mapping.min_value);
                
                parameter_manager_->setParameter(mapping.parameter_id, param_value);
            }
        }
    }
    
    void handleParameterChange(uint32_t param_id, float value) {
        if (!midi_interface_) return;
        
        // Find mapping for this parameter
        for (const auto& pair : mappings_) {
            const auto& mapping = pair.second;
            if (mapping.parameter_id == param_id) {
                // Convert parameter value to MIDI range
                float normalized = (value - mapping.min_value) / (mapping.max_value - mapping.min_value);
                uint8_t midi_value = (uint8_t)(normalized * 127.0f);
                
                // Send CC message
                uint8_t status = 0xB0 | mapping.midi_channel;
                midi_interface_->sendMidiMessage(status, mapping.midi_cc, midi_value);
                break;
            }
        }
    }
};

/**
 * @brief Real-time Safe Event Distributor (Simplified)
 */
class RTSafeEventDistributor {
public:
    using EventCallback = std::function<void(const std::string&, const std::string&)>;
    
    void setEventCallback(EventCallback callback) {
        event_callback_ = callback;
    }
    
    void distributeEvent(const std::string& event_type, const std::string& data) {
        event_count_++;
        if (event_callback_) {
            event_callback_(event_type, data);
        }
    }
    
    size_t getEventCount() const { return event_count_; }
    void reset() { event_count_ = 0; }

private:
    EventCallback event_callback_;
    size_t event_count_ = 0;
};

} // namespace Integration

using namespace Integration;

// ============================================================================
// INTEGRATION TESTS - MIDI-Parameter Bridge
// ============================================================================

TEST_INTEGRATION(MidiParameter, BasicMapping) {
    auto bridge = std::make_unique<MidiParameterBridge>();
    auto midi = std::make_shared<MockMidiInterface>();
    auto params = std::make_shared<MockParameterManager>();
    
    bridge->setMidiInterface(midi);
    bridge->setParameterManager(params);
    
    // Add mapping: CC 7 (volume) -> Parameter 1 (filter cutoff)
    bridge->addMapping(1, 7, 0, 0.0f, 1.0f);
    
    ASSERT_EQ(1lu, bridge->getMappingCount());
    ASSERT_TRUE(bridge->hasMapping(7, 0));  // CC 7 on channel 0
    
    // Simulate MIDI CC message: CC 7, value 64 (50%)
    midi->simulateReceivedMessage(0xB0, 7, 64);
    
    // Should have set parameter 1 to ~0.5
    ASSERT_EQ(1lu, params->getChangeCount());
    float param_value = params->getParameter(1);
    ASSERT_NEAR(0.5f, param_value, 0.01f);
}

TEST_INTEGRATION(MidiParameter, BidirectionalSync) {
    auto bridge = std::make_unique<MidiParameterBridge>();
    auto midi = std::make_shared<MockMidiInterface>();
    auto params = std::make_shared<MockParameterManager>();
    
    bridge->setMidiInterface(midi);
    bridge->setParameterManager(params);
    bridge->addMapping(1, 7, 0, 0.0f, 1.0f);
    
    // Test: Parameter -> MIDI
    params->setParameter(1, 0.75f);
    
    ASSERT_EQ(1lu, midi->getSentMessageCount());
    auto sent_msg = midi->sent_messages[0];
    ASSERT_EQ_NUM(0xB0, static_cast<int>(sent_msg.status));  // CC on channel 0
    ASSERT_EQ_NUM(7, static_cast<int>(sent_msg.data1));      // CC 7
    ASSERT_EQ_NUM(95, static_cast<int>(sent_msg.data2));     // 0.75 * 127 ≈ 95
    
    // Test: MIDI -> Parameter
    midi->clearMessages();
    params->clearChanges();
    
    midi->simulateReceivedMessage(0xB0, 7, 32);  // 25% value
    
    ASSERT_EQ(1lu, params->getChangeCount());
    ASSERT_NEAR(0.25f, params->getParameter(1), 0.02f);
}

TEST_INTEGRATION(MidiParameter, MultipleParameterMappings) {
    auto bridge = std::make_unique<MidiParameterBridge>();
    auto midi = std::make_shared<MockMidiInterface>();
    auto params = std::make_shared<MockParameterManager>();
    
    bridge->setMidiInterface(midi);
    bridge->setParameterManager(params);
    
    // Add multiple mappings
    bridge->addMapping(1, 7, 0, 0.0f, 1.0f);   // Volume -> Filter Cutoff
    bridge->addMapping(2, 71, 0, 0.0f, 1.0f);  // Resonance -> CC 71
    bridge->addMapping(3, 74, 0, 0.0f, 1.0f);  // LFO Rate -> CC 74
    
    ASSERT_EQ(3lu, bridge->getMappingCount());
    
    // Send multiple CC messages
    midi->simulateReceivedMessage(0xB0, 7, 100);   // Filter cutoff to ~78%
    midi->simulateReceivedMessage(0xB0, 71, 50);   // Resonance to ~39%
    midi->simulateReceivedMessage(0xB0, 74, 127);  // LFO Rate to 100%
    
    ASSERT_EQ(3lu, params->getChangeCount());
    
    ASSERT_NEAR(0.78f, params->getParameter(1), 0.02f);  // 100/127
    ASSERT_NEAR(0.39f, params->getParameter(2), 0.02f);  // 50/127
    ASSERT_NEAR(1.0f, params->getParameter(3), 0.01f);   // 127/127
}

TEST_INTEGRATION(MidiParameter, CustomValueRanges) {
    auto bridge = std::make_unique<MidiParameterBridge>();
    auto midi = std::make_shared<MockMidiInterface>();
    auto params = std::make_shared<MockParameterManager>();
    
    bridge->setMidiInterface(midi);
    bridge->setParameterManager(params);
    
    // Map CC 1 to parameter with custom range (100Hz to 10kHz for filter)
    bridge->addMapping(1, 1, 0, 100.0f, 10000.0f);
    
    // Send MIDI CC: value 64 (50%)
    midi->simulateReceivedMessage(0xB0, 1, 64);
    
    // Should map to middle of range: 100 + 0.5 * (10000 - 100) = 5050Hz
    float param_value = params->getParameter(1);
    ASSERT_NEAR(5050.0f, param_value, 50.0f);
    
    // Test reverse: set parameter to 2000Hz
    params->clearChanges();
    midi->clearMessages();
    
    params->setParameter(1, 2000.0f);
    
    // Should send MIDI: (2000 - 100) / (10000 - 100) * 127 ≈ 24
    ASSERT_EQ(1lu, midi->getSentMessageCount());
    auto sent_msg = midi->sent_messages[0];
    ASSERT_EQ_NUM(1, static_cast<int>(sent_msg.data1));      // CC 1
    ASSERT_NEAR(24, static_cast<int>(sent_msg.data2), 2); // Allow some rounding tolerance
}

// ============================================================================
// INTEGRATION TESTS - Event Distribution
// ============================================================================

TEST_INTEGRATION(EventDistribution, MidiParameterEvents) {
    auto bridge = std::make_unique<MidiParameterBridge>();
    auto midi = std::make_shared<MockMidiInterface>();
    auto params = std::make_shared<MockParameterManager>();
    auto distributor = std::make_unique<RTSafeEventDistributor>();
    
    std::vector<std::pair<std::string, std::string>> received_events;
    
    distributor->setEventCallback([&received_events](const std::string& type, const std::string& data) {
        received_events.push_back({type, data});
    });
    
    // Set up bridge with event tracking
    bridge->setMidiInterface(midi);
    bridge->setParameterManager(params);
    bridge->addMapping(1, 7, 0, 0.0f, 1.0f);
    
    // Override parameter callback to generate events
    params->setChangeCallback([&distributor, &params](uint32_t param_id, float value) {
        distributor->distributeEvent("PARAMETER_CHANGE", 
                                   std::to_string(param_id) + ":" + std::to_string(value));
        // Call original parameter manager logic
        params->setParameter(param_id, value);
    });
    
    // Simulate MIDI input -> parameter change -> event
    midi->simulateReceivedMessage(0xB0, 7, 64);
    
    ASSERT_EQ(1lu, distributor->getEventCount());
    ASSERT_EQ(1lu, received_events.size());
    ASSERT_EQ("PARAMETER_CHANGE", received_events[0].first);
    ASSERT_TRUE(received_events[0].second.find("1:") == 0);  // Should start with "1:"
}

TEST_INTEGRATION(EventDistribution, HighVolumeEventFlow) {
    auto distributor = std::make_unique<RTSafeEventDistributor>();
    size_t received_event_count = 0;
    
    distributor->setEventCallback([&received_event_count](const std::string& type, const std::string& data) {
        received_event_count++;
        // Use parameters to avoid warnings
        (void)type;
        (void)data;
    });
    
    // Generate high volume of events
    const size_t event_count = 1000;
    for (size_t i = 0; i < event_count; ++i) {
        distributor->distributeEvent("MIDI_EVENT", "note" + std::to_string(i));
    }
    
    ASSERT_EQ(event_count, distributor->getEventCount());
    ASSERT_EQ(event_count, received_event_count);
}

// ============================================================================
// SYSTEM TESTS - Complete Workflow
// ============================================================================

TEST_SYSTEM(CompleteParameterLockWorkflow) {
    // This would test the complete workflow from UI interaction
    // through parameter lock application to MIDI output
    
    auto bridge = std::make_unique<MidiParameterBridge>();
    auto midi = std::make_shared<MockMidiInterface>();
    auto params = std::make_shared<MockParameterManager>();
    
    // Set up complete system
    bridge->setMidiInterface(midi);
    bridge->setParameterManager(params);
    bridge->addMapping(1, 7, 0, 0.0f, 1.0f);   // Filter cutoff
    bridge->addMapping(2, 71, 0, 0.0f, 1.0f);  // Resonance
    
    // Simulate complete workflow: 
    // 1. MIDI input changes parameters
    // 2. Parameter locks are applied
    // 3. Parameters are sent back as MIDI
    
    // Step 1: Receive MIDI CC messages
    midi->simulateReceivedMessage(0xB0, 7, 80);   // Filter to 63%
    midi->simulateReceivedMessage(0xB0, 71, 100); // Resonance to 79%
    
    // Verify parameter changes
    ASSERT_EQ(2lu, params->getChangeCount());
    ASSERT_NEAR(0.63f, params->getParameter(1), 0.02f);
    ASSERT_NEAR(0.79f, params->getParameter(2), 0.02f);
    
    // Step 2: Clear MIDI messages and change parameters (simulating parameter locks)
    midi->clearMessages();
    params->clearChanges();
    
    // Simulate parameter lock applying different values
    params->setParameter(1, 0.25f);  // Lock filter to 25%
    params->setParameter(2, 0.9f);   // Lock resonance to 90%
    
    // Step 3: Verify MIDI output reflects parameter locks
    ASSERT_EQ(2lu, midi->getSentMessageCount());
    
    // Find the messages (may be in any order)
    bool found_filter_msg = false, found_resonance_msg = false;
    for (const auto& msg : midi->sent_messages) {
        if (msg.data1 == 7) {  // Filter CC
            found_filter_msg = true;
            ASSERT_NEAR(32, msg.data2, 2);  // 0.25 * 127 ≈ 32
        } else if (msg.data1 == 71) {  // Resonance CC
            found_resonance_msg = true;
            ASSERT_NEAR(114, msg.data2, 2);  // 0.9 * 127 ≈ 114
        }
    }
    ASSERT_TRUE(found_filter_msg);
    ASSERT_TRUE(found_resonance_msg);
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main() {
    std::cout << "🔗 MIDI-Parameter Integration Tests - Unified Framework" << std::endl;
    std::cout << "=======================================================" << std::endl;
    
    auto& runner = TestFramework::TestRunner::getInstance();
    
    // Run integration tests
    auto integration_results = runner.runCategory("integration/MidiParameter");
    
    // Run system tests  
    auto system_results = runner.runCategory("system");
    
    // Combined results
    bool all_passed = (integration_results.failed_tests == 0) && (system_results.failed_tests == 0);
    
    return all_passed ? 0 : 1;
}
