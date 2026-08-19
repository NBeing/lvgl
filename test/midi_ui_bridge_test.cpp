#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "components/ui/MidiUIBridge.h"
#include "components/parameter/ParameterManager.h"
#include "components/ui/sequencer/SequencerUIManager.h"
#include <memory>
#include <thread>
#include <chrono>

using namespace UI;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::InSequence;

/**
 * @brief Mock UI Manager for testing UI actions
 */
class MockSequencerUIManager : public SequencerUIManager {
public:
    MockSequencerUIManager() : SequencerUIManager(nullptr, nullptr, nullptr, nullptr) {}
    
    MOCK_METHOD(void, setFocus, (int track, int step), (override));
    MOCK_METHOD(void, switchToPattern, (int pattern), (override));
    MOCK_METHOD(int, getCurrentTrack, (), (const, override));
    MOCK_METHOD(int, getCurrentStep, (), (const, override));
    MOCK_METHOD(int, getCurrentPattern, (), (const, override));
    MOCK_METHOD(void, quickSave, (), (override));
    MOCK_METHOD(void, quickLoad, (), (override));
    MOCK_METHOD(void, setZoomLevel, (float zoom), (override));
};

/**
 * @brief Test fixture for MIDI → UI bridge tests
 */
class MidiUIBridgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mock components
        param_manager_ = std::make_shared<Parameters::ParameterManager>();
        param_manager_->initialize();
        
        mock_ui_manager_ = std::make_shared<MockSequencerUIManager>();
        
        // Get bridge instance and initialize
        bridge_ = &MidiUIBridge::getInstance();
        bridge_->initialize(param_manager_);
        bridge_->setUIManager(mock_ui_manager_);
        
        setupMockBehavior();
    }
    
    void TearDown() override {
        bridge_->shutdown();
        param_manager_->shutdown();
    }
    
    void setupMockBehavior() {
        // Default UI manager behavior
        EXPECT_CALL(*mock_ui_manager_, getCurrentTrack())
            .WillRepeatedly(::testing::Return(0));
        EXPECT_CALL(*mock_ui_manager_, getCurrentStep())
            .WillRepeatedly(::testing::Return(0));
        EXPECT_CALL(*mock_ui_manager_, getCurrentPattern())
            .WillRepeatedly(::testing::Return(0));
    }
    
    /**
     * @brief Simulate MIDI CC that maps to UI parameter
     */
    void sendMidiToUIParameter(uint8_t channel, uint8_t cc, uint8_t value, 
                              MidiUIBridge::UIParameterID ui_param) {
        // Map MIDI CC to UI parameter
        bridge_->mapMidiToUI(channel, cc, ui_param);
        
        // Simulate MIDI CC input through parameter manager
        auto mapped_param = param_manager_->getMidiMapping(channel, cc);
        ASSERT_NE(mapped_param, 0) << "MIDI CC should be mapped";
        
        float normalized = static_cast<float>(value) / 127.0f;
        Parameters::ParameterChangeEvent event(
            mapped_param, normalized, Parameters::ParameterSource::MIDI_CC);
        event.midi_channel = channel;
        
        param_manager_->processParameterChange(event);
        
        // Give time for UI thread processing
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

protected:
    MidiUIBridge* bridge_;
    std::shared_ptr<Parameters::ParameterManager> param_manager_;
    std::shared_ptr<MockSequencerUIManager> mock_ui_manager_;
};

// =============================================================================
// INITIALIZATION AND LIFECYCLE TESTS
// =============================================================================

TEST_F(MidiUIBridgeTest, Initialization) {
    // Bridge should be initialized in SetUp
    EXPECT_EQ(bridge_->getActiveMappingCount(), 0);
    
    // Should be able to add mappings
    bridge_->mapMidiToUI(1, 16, MidiUIBridge::UIParameterID::UI_FOCUS_TRACK);
    EXPECT_EQ(bridge_->getActiveMappingCount(), 1);
}

TEST_F(MidiUIBridgeTest, DefaultMappings) {
    // Set up default mappings
    bridge_->setupDefaultUIMappings();
    
    // Should have created multiple mappings
    EXPECT_GT(bridge_->getActiveMappingCount(), 5);
    
    // Verify specific mappings exist by testing parameter manager
    auto track_focus_param = param_manager_->getMidiMapping(1, 16);
    EXPECT_NE(track_focus_param, 0);
    
    auto step_focus_param = param_manager_->getMidiMapping(1, 17);
    EXPECT_NE(step_focus_param, 0);
}

// =============================================================================
// UI ACTION TESTS
// =============================================================================

TEST_F(MidiUIBridgeTest, TrackFocusControl) {
    // Set up expectation for track focus changes
    EXPECT_CALL(*mock_ui_manager_, setFocus(0, _)).Times(1);
    EXPECT_CALL(*mock_ui_manager_, setFocus(4, _)).Times(1);
    EXPECT_CALL(*mock_ui_manager_, setFocus(7, _)).Times(1);
    
    // Test track focus at different values
    sendMidiToUIParameter(1, 16, 0, MidiUIBridge::UIParameterID::UI_FOCUS_TRACK);   // Track 0
    sendMidiToUIParameter(1, 16, 64, MidiUIBridge::UIParameterID::UI_FOCUS_TRACK);  // Track 4
    sendMidiToUIParameter(1, 16, 127, MidiUIBridge::UIParameterID::UI_FOCUS_TRACK); // Track 7
}

TEST_F(MidiUIBridgeTest, StepFocusControl) {
    // Set up expectation for step focus changes
    EXPECT_CALL(*mock_ui_manager_, setFocus(_, 0)).Times(1);
    EXPECT_CALL(*mock_ui_manager_, setFocus(_, 8)).Times(1);
    EXPECT_CALL(*mock_ui_manager_, setFocus(_, 15)).Times(1);
    
    // Test step focus at different values
    sendMidiToUIParameter(1, 17, 0, MidiUIBridge::UIParameterID::UI_FOCUS_STEP);   // Step 0
    sendMidiToUIParameter(1, 17, 68, MidiUIBridge::UIParameterID::UI_FOCUS_STEP);  // Step 8
    sendMidiToUIParameter(1, 17, 127, MidiUIBridge::UIParameterID::UI_FOCUS_STEP); // Step 15
}

TEST_F(MidiUIBridgeTest, PatternSelection) {
    // Set up expectation for pattern switching
    EXPECT_CALL(*mock_ui_manager_, switchToPattern(0)).Times(1);
    EXPECT_CALL(*mock_ui_manager_, switchToPattern(8)).Times(1);
    EXPECT_CALL(*mock_ui_manager_, switchToPattern(15)).Times(1);
    
    // Test pattern selection
    sendMidiToUIParameter(1, 18, 0, MidiUIBridge::UIParameterID::UI_PAGE_SELECT);   // Pattern 0
    sendMidiToUIParameter(1, 18, 68, MidiUIBridge::UIParameterID::UI_PAGE_SELECT);  // Pattern 8
    sendMidiToUIParameter(1, 18, 127, MidiUIBridge::UIParameterID::UI_PAGE_SELECT); // Pattern 15
}

TEST_F(MidiUIBridgeTest, QuickSaveLoad) {
    // Set up expectations for quick save/load
    EXPECT_CALL(*mock_ui_manager_, quickSave()).Times(1);
    EXPECT_CALL(*mock_ui_manager_, quickLoad()).Times(1);
    
    // Test quick save (only triggers on high values >= 64)
    sendMidiToUIParameter(3, 56, 127, MidiUIBridge::UIParameterID::UI_QUICK_SAVE);
    
    // Test quick load
    sendMidiToUIParameter(3, 57, 100, MidiUIBridge::UIParameterID::UI_QUICK_LOAD);
    
    // Test that low values don't trigger
    sendMidiToUIParameter(3, 56, 30, MidiUIBridge::UIParameterID::UI_QUICK_SAVE);  // Should not call quickSave
}

// =============================================================================
// CUSTOM UI ACTIONS TESTS
// =============================================================================

TEST_F(MidiUIBridgeTest, CustomUIAction) {
    bool custom_action_called = false;
    float received_value = 0.0f;
    
    // Register custom UI action
    bridge_->registerUIAction(
        MidiUIBridge::UIParameterID::UI_BRIGHTNESS,
        [&](float value) {
            custom_action_called = true;
            received_value = value;
        }
    );
    
    // Test custom action
    sendMidiToUIParameter(2, 26, 64, MidiUIBridge::UIParameterID::UI_BRIGHTNESS);
    
    // Verify custom action was called
    EXPECT_TRUE(custom_action_called);
    EXPECT_NEAR(received_value, 0.504f, 0.01f); // 64/127 ≈ 0.504
}

TEST_F(MidiUIBridgeTest, MultipleCustomActions) {
    int brightness_calls = 0;
    int contrast_calls = 0;
    
    // Register multiple custom actions
    bridge_->registerUIAction(
        MidiUIBridge::UIParameterID::UI_BRIGHTNESS,
        [&](float value) { brightness_calls++; (void)value; }
    );
    
    bridge_->registerUIAction(
        MidiUIBridge::UIParameterID::UI_CONTRAST,
        [&](float value) { contrast_calls++; (void)value; }
    );
    
    // Test both actions
    sendMidiToUIParameter(2, 26, 80, MidiUIBridge::UIParameterID::UI_BRIGHTNESS);
    sendMidiToUIParameter(2, 27, 90, MidiUIBridge::UIParameterID::UI_CONTRAST);
    sendMidiToUIParameter(2, 26, 100, MidiUIBridge::UIParameterID::UI_BRIGHTNESS);
    
    // Verify call counts
    EXPECT_EQ(brightness_calls, 2);
    EXPECT_EQ(contrast_calls, 1);
}

// =============================================================================
// MAPPING MANAGEMENT TESTS
// =============================================================================

TEST_F(MidiUIBridgeTest, MappingAddRemove) {
    // Initially no mappings
    EXPECT_EQ(bridge_->getActiveMappingCount(), 0);
    
    // Add mapping
    bridge_->mapMidiToUI(1, 50, MidiUIBridge::UIParameterID::UI_ZOOM_LEVEL);
    EXPECT_EQ(bridge_->getActiveMappingCount(), 1);
    
    // Add another mapping
    bridge_->mapMidiToUI(2, 51, MidiUIBridge::UIParameterID::UI_COLOR_SCHEME);
    EXPECT_EQ(bridge_->getActiveMappingCount(), 2);
    
    // Remove mapping
    bridge_->removeMidiUIMapping(1, 50);
    EXPECT_EQ(bridge_->getActiveMappingCount(), 1);
    
    // Remove non-existent mapping (should not crash)
    bridge_->removeMidiUIMapping(5, 99);
    EXPECT_EQ(bridge_->getActiveMappingCount(), 1);
}

TEST_F(MidiUIBridgeTest, OverwriteMapping) {
    // Map CC to one UI parameter
    bridge_->mapMidiToUI(1, 30, MidiUIBridge::UIParameterID::UI_BRIGHTNESS);
    EXPECT_EQ(bridge_->getActiveMappingCount(), 1);
    
    // Map same CC to different UI parameter
    bridge_->mapMidiToUI(1, 30, MidiUIBridge::UIParameterID::UI_CONTRAST);
    
    // Should still have only one mapping (overwritten)
    EXPECT_EQ(bridge_->getActiveMappingCount(), 1);
}

// =============================================================================
// MULTI-CHANNEL TESTS
// =============================================================================

TEST_F(MidiUIBridgeTest, MultiChannelMappings) {
    // Map same CC on different channels to different UI parameters
    bridge_->mapMidiToUI(1, 20, MidiUIBridge::UIParameterID::UI_FOCUS_TRACK);
    bridge_->mapMidiToUI(2, 20, MidiUIBridge::UIParameterID::UI_FOCUS_STEP);
    bridge_->mapMidiToUI(3, 20, MidiUIBridge::UIParameterID::UI_PAGE_SELECT);
    
    EXPECT_EQ(bridge_->getActiveMappingCount(), 3);
    
    // Set up expectations
    EXPECT_CALL(*mock_ui_manager_, setFocus(3, _)).Times(1);      // Track focus
    EXPECT_CALL(*mock_ui_manager_, setFocus(_, 7)).Times(1);      // Step focus
    EXPECT_CALL(*mock_ui_manager_, switchToPattern(7)).Times(1);  // Pattern select
    
    // Test each channel
    sendMidiToUIParameter(1, 20, 64, MidiUIBridge::UIParameterID::UI_FOCUS_TRACK);  // Ch1 → Track
    sendMidiToUIParameter(2, 20, 64, MidiUIBridge::UIParameterID::UI_FOCUS_STEP);   // Ch2 → Step
    sendMidiToUIParameter(3, 20, 64, MidiUIBridge::UIParameterID::UI_PAGE_SELECT);  // Ch3 → Pattern
}

// =============================================================================
// ERROR HANDLING TESTS
// =============================================================================

TEST_F(MidiUIBridgeTest, UnmappedMidiCC) {
    // Sending MIDI CC that's not mapped should not crash
    auto unmapped_param = param_manager_->getMidiMapping(5, 99);
    EXPECT_EQ(unmapped_param, 0); // Should not be mapped
    
    // This should not cause any UI actions
    Parameters::ParameterChangeEvent event(
        12345, 0.5f, Parameters::ParameterSource::MIDI_CC);
    param_manager_->processParameterChange(event);
    
    // No expectations set, so if any UI manager methods are called, test will fail
}

TEST_F(MidiUIBridgeTest, CustomActionException) {
    // Register action that throws exception
    bridge_->registerUIAction(
        MidiUIBridge::UIParameterID::UI_ZOOM_LEVEL,
        [](float value) {
            (void)value;
            throw std::runtime_error("Test exception");
        }
    );
    
    // Should not crash when exception is thrown
    EXPECT_NO_THROW({
        sendMidiToUIParameter(4, 50, 100, MidiUIBridge::UIParameterID::UI_ZOOM_LEVEL);
    });
}

// =============================================================================
// THREAD SAFETY TESTS
// =============================================================================

TEST_F(MidiUIBridgeTest, ConcurrentMappingOperations) {
    std::atomic<int> successful_mappings{0};
    std::atomic<bool> stop_test{false};
    
    // Create multiple threads adding/removing mappings
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([&, i]() {
            int cc = 40 + i;
            while (!stop_test.load()) {
                bridge_->mapMidiToUI(1, cc, MidiUIBridge::UIParameterID::UI_BRIGHTNESS);
                successful_mappings.fetch_add(1);
                bridge_->removeMidiUIMapping(1, cc);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }
    
    // Let threads run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    stop_test.store(true);
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify some operations completed successfully
    EXPECT_GT(successful_mappings.load(), 0);
}

// =============================================================================
// INTEGRATION TESTS
// =============================================================================

TEST_F(MidiUIBridgeTest, CompleteUIWorkflow) {
    InSequence workflow_sequence;
    
    // Set up default mappings
    bridge_->setupDefaultUIMappings();
    
    // 1. Focus on track 2
    EXPECT_CALL(*mock_ui_manager_, setFocus(2, _)).Times(1);
    sendMidiToUIParameter(1, 16, 36, MidiUIBridge::UIParameterID::UI_FOCUS_TRACK); // ~Track 2
    
    // 2. Focus on step 8
    EXPECT_CALL(*mock_ui_manager_, setFocus(_, 8)).Times(1);
    sendMidiToUIParameter(1, 17, 68, MidiUIBridge::UIParameterID::UI_FOCUS_STEP); // ~Step 8
    
    // 3. Switch to pattern 5
    EXPECT_CALL(*mock_ui_manager_, switchToPattern(5)).Times(1);
    sendMidiToUIParameter(1, 18, 42, MidiUIBridge::UIParameterID::UI_PAGE_SELECT); // ~Pattern 5
    
    // 4. Quick save
    EXPECT_CALL(*mock_ui_manager_, quickSave()).Times(1);
    sendMidiToUIParameter(3, 56, 127, MidiUIBridge::UIParameterID::UI_QUICK_SAVE);
    
    // 5. Switch to different pattern
    EXPECT_CALL(*mock_ui_manager_, switchToPattern(10)).Times(1);
    sendMidiToUIParameter(1, 18, 85, MidiUIBridge::UIParameterID::UI_PAGE_SELECT); // ~Pattern 10
    
    // 6. Quick load to restore
    EXPECT_CALL(*mock_ui_manager_, quickLoad()).Times(1);
    sendMidiToUIParameter(3, 57, 127, MidiUIBridge::UIParameterID::UI_QUICK_LOAD);
}

// =============================================================================
// STATISTICS TESTS
// =============================================================================

TEST_F(MidiUIBridgeTest, StatisticsTracking) {
    // Set up some mappings and actions
    bridge_->setupDefaultUIMappings();
    
    size_t initial_count = bridge_->getActiveMappingCount();
    EXPECT_GT(initial_count, 0);
    
    // Trigger some UI actions
    EXPECT_CALL(*mock_ui_manager_, setFocus(_, _)).Times(AtLeast(2));
    EXPECT_CALL(*mock_ui_manager_, switchToPattern(_)).Times(AtLeast(1));
    
    sendMidiToUIParameter(1, 16, 50, MidiUIBridge::UIParameterID::UI_FOCUS_TRACK);
    sendMidiToUIParameter(1, 17, 75, MidiUIBridge::UIParameterID::UI_FOCUS_STEP);
    sendMidiToUIParameter(1, 18, 90, MidiUIBridge::UIParameterID::UI_PAGE_SELECT);
    
    // Export mappings (should not crash)
    std::string mappings = bridge_->exportUIMappings();
    EXPECT_FALSE(mappings.empty());
    EXPECT_TRUE(mappings.find("Statistics") != std::string::npos);
    
    // Print statistics (should not crash)
    EXPECT_NO_THROW({
        bridge_->printUIMappingStatistics();
    });
}

} // Anonymous namespace

/**
 * @brief Main test runner
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
