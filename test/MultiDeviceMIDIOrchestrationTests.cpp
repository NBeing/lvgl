/**
 * @brief Multi-Device MIDI Orchestration Tests
 * 
 * Tests the complete multi-synthesizer control system:
 * - Parameter distribution across multiple devices
 * - Device-specific MIDI mappings  
 * - Preset management for multi-device setups
 * - MIDI bandwidth optimization
 * - Hot-plug device management
 * - Cross-device parameter linking
 */

#include "TestFramework.h"
#include "components/controls/BidirectionalParameterMidiBridge.h"
#include "components/midi/MultiDeviceMIDIManager.h"  // Next component to build
#include "components/presets/PresetManager.h"        // Next component to build
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <unordered_map>

using namespace RTSafe;
using namespace Test;

/**
 * @brief Mock External Synthesizer for testing
 */
class MockExternalSynth {
private:
    std::string device_name_;
    uint8_t midi_channel_;
    std::unordered_map<uint8_t, uint8_t> cc_values_;  // CC# → Value
    std::atomic<int> messages_received_{0};
    
public:
    MockExternalSynth(const std::string& name, uint8_t channel) 
        : device_name_(name), midi_channel_(channel) {}
    
    void receiveMIDICC(uint8_t channel, uint8_t cc_number, uint8_t value) {
        if (channel == midi_channel_) {
            cc_values_[cc_number] = value;
            messages_received_++;
        }
    }
    
    uint8_t getCCValue(uint8_t cc_number) const {
        auto it = cc_values_.find(cc_number);
        return (it != cc_values_.end()) ? it->second : 0;
    }
    
    int getMessagesReceived() const {
        return messages_received_.load();
    }
    
    void resetMessageCount() {
        messages_received_ = 0;
    }
    
    const std::string& getName() const { return device_name_; }
    uint8_t getChannel() const { return midi_channel_; }
};

/**
 * @brief Multi-Device Orchestration Test Suite
 */
class MultiDeviceMIDIOrchestrationTests {
private:
    // Test setup with multiple mock synthesizers
    std::vector<std::unique_ptr<MockExternalSynth>> synthesizers_;
    std::unique_ptr<MultiDeviceMIDIManager> device_manager_;
    std::unique_ptr<PresetManager> preset_manager_;
    
public:
    void setUp() {
        // Create mock synthesizers
        synthesizers_.push_back(std::make_unique<MockExternalSynth>("Nord Lead", 0));
        synthesizers_.push_back(std::make_unique<MockExternalSynth>("Hydrasynth", 1)); 
        synthesizers_.push_back(std::make_unique<MockExternalSynth>("Drum Machine", 9));
        
        // Initialize device manager
        device_manager_ = std::make_unique<MultiDeviceMIDIManager>();
        
        // Register mock synthesizers
        for (auto& synth : synthesizers_) {
            device_manager_->registerDevice(synth->getName(), synth->getChannel());
        }
        
        // Initialize preset manager
        preset_manager_ = std::make_unique<PresetManager>(device_manager_.get());
    }
    
    void tearDown() {
        preset_manager_.reset();
        device_manager_.reset();
        synthesizers_.clear();
    }
    
    // Test 1: Multi-Device Parameter Distribution
    void testMultiDeviceParameterDistribution() {
        TEST_SUITE("Multi-Device Parameter Distribution");
        
        TEST("Master filter control affects multiple synthesizers") {
            // Set up linked parameter: Master Filter → All synths
            device_manager_->linkParameter("master_filter", {
                {"Nord Lead", 74},    // Filter Cutoff CC
                {"Hydrasynth", 83},   // Different CC for same parameter
                {"Drum Machine", 74}  // Drum machine filter
            });
            
            // User adjusts master filter to 75% (MIDI value ~96)
            device_manager_->setLinkedParameter("master_filter", 0.75f);
            
            // Check all devices received appropriate values
            ASSERT_EQ(96, synthesizers_[0]->getCCValue(74));  // Nord Lead
            ASSERT_EQ(96, synthesizers_[1]->getCCValue(83));  // Hydrasynth  
            ASSERT_EQ(96, synthesizers_[2]->getCCValue(74));  // Drum Machine
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 2: Device-Specific MIDI Mappings
    void testDeviceSpecificMappings() {
        TEST_SUITE("Device-Specific MIDI Mappings");
        
        TEST("Same logical parameter maps to different CCs per device") {
            // Define device-specific mappings
            DeviceMapping nord_mapping = {
                {"filter_cutoff", 74}, {"filter_resonance", 71}, {"envelope_attack", 73}
            };
            DeviceMapping hydra_mapping = {
                {"filter_cutoff", 83}, {"filter_resonance", 82}, {"envelope_attack", 85}
            };
            
            device_manager_->setDeviceMapping("Nord Lead", nord_mapping);
            device_manager_->setDeviceMapping("Hydrasynth", hydra_mapping);
            
            // Set filter cutoff parameter
            device_manager_->setDeviceParameter("Nord Lead", "filter_cutoff", 0.6f);
            device_manager_->setDeviceParameter("Hydrasynth", "filter_cutoff", 0.6f);
            
            // Check correct CCs were sent to each device
            ASSERT_EQ(76, synthesizers_[0]->getCCValue(74));  // Nord Lead CC 74
            ASSERT_EQ(76, synthesizers_[1]->getCCValue(83));  // Hydrasynth CC 83
            ASSERT_EQ(0, synthesizers_[1]->getCCValue(74));   // Hydrasynth CC 74 not set
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 3: Multi-Device Preset Management
    void testMultiDevicePresetManagement() {
        TEST_SUITE("Multi-Device Preset Management");
        
        TEST("Save and load complete multi-device setup") {
            // Set up different parameters on each device
            device_manager_->setDeviceParameter("Nord Lead", "filter_cutoff", 0.8f);
            device_manager_->setDeviceParameter("Nord Lead", "filter_resonance", 0.3f);
            device_manager_->setDeviceParameter("Hydrasynth", "filter_cutoff", 0.5f);
            device_manager_->setDeviceParameter("Hydrasynth", "envelope_attack", 0.7f);
            device_manager_->setDeviceParameter("Drum Machine", "volume", 0.9f);
            
            // Save preset
            preset_manager_->savePreset("Live Setup A");
            
            // Change all parameters
            device_manager_->setDeviceParameter("Nord Lead", "filter_cutoff", 0.1f);
            device_manager_->setDeviceParameter("Hydrasynth", "filter_cutoff", 0.1f);
            device_manager_->setDeviceParameter("Drum Machine", "volume", 0.1f);
            
            // Load preset
            preset_manager_->loadPreset("Live Setup A");
            
            // Verify all devices restored to original state
            ASSERT_EQ(102, synthesizers_[0]->getCCValue(74));  // Nord filter ~0.8
            ASSERT_EQ(38, synthesizers_[0]->getCCValue(71));   // Nord resonance ~0.3
            ASSERT_EQ(64, synthesizers_[1]->getCCValue(83));   // Hydra filter ~0.5
            ASSERT_EQ(114, synthesizers_[2]->getCCValue(7));   // Drum volume ~0.9
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 4: MIDI Bandwidth Stress Test
    void testMIDIBandwidthUnderLoad() {
        TEST_SUITE("MIDI Bandwidth Management");
        
        TEST("High-frequency parameter changes across multiple devices") {
            std::atomic<bool> stress_running{true};
            std::atomic<int> parameters_sent{0};
            
            // Stress test thread: Rapidly change parameters
            std::thread stress_thread([this, &stress_running, &parameters_sent]() {
                int iteration = 0;
                while (stress_running && iteration < 1000) {
                    float value = (iteration % 128) / 127.0f;
                    
                    // Simultaneously update multiple devices
                    device_manager_->setDeviceParameter("Nord Lead", "filter_cutoff", value);
                    device_manager_->setDeviceParameter("Hydrasynth", "filter_cutoff", value);
                    device_manager_->setDeviceParameter("Drum Machine", "volume", value);
                    
                    parameters_sent += 3;
                    iteration++;
                    
                    // Simulate rapid knob turning (10ms between changes)
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
            
            // Run stress test for 10 seconds
            std::this_thread::sleep_for(std::chrono::milliseconds(10000));
            stress_running = false;
            stress_thread.join();
            
            // Verify all devices received messages without loss
            int total_received = 0;
            for (auto& synth : synthesizers_) {
                total_received += synth->getMessagesReceived();
            }
            
            // Should receive most messages (allow some tolerance for timing)
            ASSERT_TRUE(total_received >= parameters_sent * 0.95); // 95% delivery rate
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 5: Device Hot-Plug Management
    void testDeviceHotPlugHandling() {
        TEST_SUITE("Device Hot-Plug Management");
        
        TEST("Device disconnect and reconnect during operation") {
            // Set initial parameters
            device_manager_->setDeviceParameter("Nord Lead", "filter_cutoff", 0.5f);
            device_manager_->setDeviceParameter("Hydrasynth", "filter_cutoff", 0.7f);
            
            // Simulate Nord Lead disconnect
            device_manager_->disconnectDevice("Nord Lead");
            
            // Try to send parameter - should not crash
            device_manager_->setDeviceParameter("Nord Lead", "filter_cutoff", 0.8f);
            
            // Hydrasynth should still work
            device_manager_->setDeviceParameter("Hydrasynth", "filter_cutoff", 0.9f);
            ASSERT_EQ(115, synthesizers_[1]->getCCValue(83)); // Hydrasynth still responds
            
            // Reconnect Nord Lead
            device_manager_->reconnectDevice("Nord Lead", 0);
            
            // Should work again
            device_manager_->setDeviceParameter("Nord Lead", "filter_cutoff", 0.6f);
            ASSERT_EQ(76, synthesizers_[0]->getCCValue(74)); // Nord Lead responds again
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Run all tests
    void runAllTests() {
        std::cout << "🎛️ Starting Multi-Device MIDI Orchestration Tests" << std::endl;
        
        setUp();
        
        testMultiDeviceParameterDistribution();
        testDeviceSpecificMappings();
        testMultiDevicePresetManagement();
        testMIDIBandwidthUnderLoad();
        testDeviceHotPlugHandling();
        
        tearDown();
        
        TestFramework::getInstance().printSummary();
        std::cout << "✅ Multi-Device MIDI Orchestration Tests Completed" << std::endl;
    }
};

// Test runner function
void runMultiDeviceMIDIOrchestrationTests() {
    MultiDeviceMIDIOrchestrationTests test_suite;
    test_suite.runAllTests();
}
