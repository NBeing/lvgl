#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "components/midi/SequencerMidiController.h"
#include "components/midi/StepSequencer.h"
#include "components/parameter/ParameterManager.h"
#include <memory>
#include <thread>
#include <chrono>

using namespace MIDI;
using ::testing::_;
using ::testing::Return;
using ::testing::AtLeast;
using ::testing::InSequence;

/**
 * @brief Mock StepSequencer for testing sequencer MIDI control
 */
class MockStepSequencer : public StepSequencer {
public:
    MOCK_METHOD(void, play, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, reset, (), (override));
    MOCK_METHOD(bool, isPlaying, (), (const, override));
    
    MOCK_METHOD(void, setStep, (int track, int step, const Step& step_data), (override));
    MOCK_METHOD(void, clearStep, (int track, int step), (override));
    MOCK_METHOD(void, toggleStep, (int track, int step), (override));
    MOCK_METHOD(const Step&, getStep, (int track, int step), (const, override));
    
    MOCK_METHOD(void, setTrackChannel, (int track, uint8_t channel), (override));
    MOCK_METHOD(void, muteTrack, (int track, bool muted), (override));
    MOCK_METHOD(void, soloTrack, (int track, bool solo), (override));
    
    MOCK_METHOD(void, setPatternLength, (int steps), (override));
    MOCK_METHOD(int, getPatternLength, (), (const, override));
    MOCK_METHOD(void, setSwing, (float swing_amount), (override));
    MOCK_METHOD(float, getSwing, (), (const, override));
    
    MOCK_METHOD(void, setStepParameterLock, (int track, int step, Parameters::ParameterID param_id, float value), (override));
    MOCK_METHOD(void, clearStepParameterLock, (int track, int step, Parameters::ParameterID param_id), (override));
    MOCK_METHOD(void, clearAllStepParameterLocks, (int track, int step), (override));
    MOCK_METHOD(bool, hasStepParameterLock, (int track, int step, Parameters::ParameterID param_id), (const, override));
    MOCK_METHOD(float, getStepParameterLock, (int track, int step, Parameters::ParameterID param_id), (const, override));
    MOCK_METHOD(std::vector<Parameters::ParameterID>, getStepParameterLocks, (int track, int step), (const, override));
    
    MOCK_METHOD(void, randomizePattern, (int track_id, float probability), (override));
    MOCK_METHOD(void, shiftPattern, (int track_id, int steps), (override));
    MOCK_METHOD(void, transposeTrack, (int track_id, int semitones), (override));
    
    // Provide mock track access
    Track& getTrack(int track_id) override {
        return mock_tracks_[track_id];
    }
    
    const Track& getTrack(int track_id) const override {
        return mock_tracks_[track_id];
    }
    
private:
    std::array<Track, MAX_TRACKS> mock_tracks_;
};

/**
 * @brief Mock ParameterManager for testing parameter integration
 */
class MockParameterManager : public Parameters::ParameterManager {
public:
    MOCK_METHOD(void, processParameterChange, (const Parameters::ParameterChangeEvent& event), (override));
    MOCK_METHOD(float, getParameterNormalized, (Parameters::ParameterID parameter_id), (const, override));
    MOCK_METHOD(void, setParameterNormalized, (Parameters::ParameterID parameter_id, float value, Parameters::ParameterSource source), (override));
    MOCK_METHOD(Parameters::ParameterID, getMidiMapping, (uint8_t channel, uint8_t cc), (const, override));
    MOCK_METHOD(void, assignMidiCC, (Parameters::ParameterID parameter_id, uint8_t channel, uint8_t cc), (override));
    MOCK_METHOD(bool, isMidiLearning, (), (const, override));
    MOCK_METHOD(Parameters::ParameterID, getMidiLearnParameter, (), (const, override));
};

/**
 * @brief Test fixture for sequencer MIDI controller tests
 */
class SequencerMidiControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mock components
        mock_sequencer_ = std::make_shared<MockStepSequencer>();
        mock_param_manager_ = std::make_shared<MockParameterManager>();
        
        // Get controller instance and initialize
        controller_ = &SequencerMidiController::getInstance();
        controller_->initialize(mock_sequencer_, mock_param_manager_);
        
        // Set up default mock behavior
        setupDefaultMockBehavior();
    }
    
    void TearDown() override {
        controller_->shutdown();
    }
    
    void setupDefaultMockBehavior() {
        // Default sequencer behavior
        EXPECT_CALL(*mock_sequencer_, isPlaying())
            .WillRepeatedly(Return(false));
        EXPECT_CALL(*mock_sequencer_, getPatternLength())
            .WillRepeatedly(Return(16));
        EXPECT_CALL(*mock_sequencer_, getSwing())
            .WillRepeatedly(Return(0.0f));
        
        // Default parameter manager behavior
        EXPECT_CALL(*mock_param_manager_, getParameterNormalized(_))
            .WillRepeatedly(Return(0.5f));
        EXPECT_CALL(*mock_param_manager_, isMidiLearning())
            .WillRepeatedly(Return(false));
        
        // Create a default step for getStep calls
        default_step_.active = false;
        default_step_.note = 60;
        default_step_.velocity = 100;
        default_step_.length = 6;
        
        EXPECT_CALL(*mock_sequencer_, getStep(_, _))
            .WillRepeatedly(::testing::ReturnRef(default_step_));
    }
    
    /**
     * @brief Simulate MIDI CC input
     */
    void sendMidiCC(uint8_t channel, uint8_t cc, uint8_t value) {
        controller_->processSequencerMidiCC(channel - 1, cc, value); // Convert to 0-based channel
    }
    
    /**
     * @brief Simulate MIDI Note On input
     */
    void sendMidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
        controller_->processSequencerNoteOn(channel - 1, note, velocity);
    }

protected:
    SequencerMidiController* controller_;
    std::shared_ptr<MockStepSequencer> mock_sequencer_;
    std::shared_ptr<MockParameterManager> mock_param_manager_;
    StepSequencer::Step default_step_;
};

// =============================================================================
// INITIALIZATION AND LIFECYCLE TESTS
// =============================================================================

TEST_F(SequencerMidiControllerTest, Initialization) {
    // Controller should be initialized in SetUp()
    EXPECT_TRUE(controller_ != nullptr);
    
    // Should have default MIDI mappings loaded
    auto mapping = controller_->getSequencerMidiMapping(1, 80); // Transport Play
    EXPECT_EQ(mapping, SequencerMidiController::SequencerParameterID::TRANSPORT_PLAY);
    
    // Should have valid initial state
    EXPECT_EQ(controller_->getCurrentTrack(), 0);
    EXPECT_EQ(controller_->getCurrentStep(), 0);
    EXPECT_FALSE(controller_->isParameterLockModeActive());
}

TEST_F(SequencerMidiControllerTest, DefaultMidiMappings) {
    // Test key transport mappings
    EXPECT_EQ(controller_->getSequencerMidiMapping(1, 80), 
              SequencerMidiController::SequencerParameterID::TRANSPORT_PLAY);
    EXPECT_EQ(controller_->getSequencerMidiMapping(1, 81), 
              SequencerMidiController::SequencerParameterID::TRANSPORT_STOP);
    
    // Test step trigger mappings
    EXPECT_EQ(controller_->getSequencerMidiMapping(1, 91), 
              SequencerMidiController::SequencerParameterID::STEP_TRIGGER_01);
    EXPECT_EQ(controller_->getSequencerMidiMapping(1, 106), 
              SequencerMidiController::SequencerParameterID::STEP_TRIGGER_16);
    
    // Test track control mappings
    EXPECT_EQ(controller_->getSequencerMidiMapping(1, 120), 
              SequencerMidiController::SequencerParameterID::TRACK_SELECT);
    EXPECT_EQ(controller_->getSequencerMidiMapping(1, 121), 
              SequencerMidiController::SequencerParameterID::TRACK_MUTE);
    
    // Test parameter lock mappings
    EXPECT_EQ(controller_->getSequencerMidiMapping(1, 70), 
              SequencerMidiController::SequencerParameterID::PARAM_LOCK_MODE);
    EXPECT_EQ(controller_->getSequencerMidiMapping(1, 72), 
              SequencerMidiController::SequencerParameterID::PARAM_LOCK_VALUE);
}

// =============================================================================
// TRANSPORT CONTROL TESTS
// =============================================================================

TEST_F(SequencerMidiControllerTest, TransportPlayControl) {
    // Test play/stop toggle
    EXPECT_CALL(*mock_sequencer_, isPlaying())
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_sequencer_, play()).Times(1);
    EXPECT_CALL(*mock_sequencer_, stop()).Times(1);
    
    // Send play command (value >= 64 = trigger)
    EXPECT_TRUE(sendMidiCC(1, 80, 127));
    
    // Send play command again (should stop)
    EXPECT_TRUE(sendMidiCC(1, 80, 100));
}

TEST_F(SequencerMidiControllerTest, TransportStopControl) {
    EXPECT_CALL(*mock_sequencer_, stop()).Times(1);
    EXPECT_CALL(*mock_sequencer_, reset()).Times(1);
    
    // Send stop command
    EXPECT_TRUE(sendMidiCC(1, 81, 127));
}

TEST_F(SequencerMidiControllerTest, PatternLengthControl) {
    // Test pattern length setting (CC 85)
    EXPECT_CALL(*mock_sequencer_, setPatternLength(8)).Times(1);  // ~50% of range
    EXPECT_CALL(*mock_sequencer_, setPatternLength(16)).Times(1); // Max value
    EXPECT_CALL(*mock_sequencer_, setPatternLength(1)).Times(1);  // Min value
    
    sendMidiCC(1, 85, 63);  // ~50% -> 8 steps
    sendMidiCC(1, 85, 127); // 100% -> 16 steps
    sendMidiCC(1, 85, 0);   // 0% -> 1 step
}

TEST_F(SequencerMidiControllerTest, SwingControl) {
    // Test swing amount setting (CC 86)
    EXPECT_CALL(*mock_sequencer_, setSwing(0.0f)).Times(1);
    EXPECT_CALL(*mock_sequencer_, setSwing(0.5f)).Times(1);
    EXPECT_CALL(*mock_sequencer_, setSwing(1.0f)).Times(1);
    
    sendMidiCC(1, 86, 0);   // 0% swing
    sendMidiCC(1, 86, 63);  // ~50% swing
    sendMidiCC(1, 86, 127); // 100% swing
}

// =============================================================================
// TRACK CONTROL TESTS
// =============================================================================

TEST_F(SequencerMidiControllerTest, TrackSelection) {
    // Test track selection (CC 120)
    sendMidiCC(1, 120, 0);   // Track 1
    EXPECT_EQ(controller_->getCurrentTrack(), 0);
    
    sendMidiCC(1, 120, 63);  // ~Track 4
    EXPECT_EQ(controller_->getCurrentTrack(), 3);
    
    sendMidiCC(1, 120, 127); // Track 8
    EXPECT_EQ(controller_->getCurrentTrack(), 7);
}

TEST_F(SequencerMidiControllerTest, TrackMuteControl) {
    // Set current track to 2
    controller_->setCurrentTrack(2);
    
    // Mock track state
    EXPECT_CALL(*mock_sequencer_, muteTrack(2, true)).Times(1);
    EXPECT_CALL(*mock_sequencer_, muteTrack(2, false)).Times(1);
    
    // First mute command should mute
    sendMidiCC(1, 121, 127);
    
    // Second mute command should unmute
    sendMidiCC(1, 121, 100);
}

TEST_F(SequencerMidiControllerTest, TrackSoloControl) {
    // Set current track to 1
    controller_->setCurrentTrack(1);
    
    EXPECT_CALL(*mock_sequencer_, soloTrack(1, true)).Times(1);
    EXPECT_CALL(*mock_sequencer_, soloTrack(1, false)).Times(1);
    
    // Toggle solo on
    sendMidiCC(1, 122, 127);
    
    // Toggle solo off
    sendMidiCC(1, 122, 80);
}

TEST_F(SequencerMidiControllerTest, TrackTranspose) {
    // Set current track to 0
    controller_->setCurrentTrack(0);
    
    EXPECT_CALL(*mock_sequencer_, transposeTrack(0, -12)).Times(1); // Min transpose
    EXPECT_CALL(*mock_sequencer_, transposeTrack(0, 0)).Times(1);   // Center
    EXPECT_CALL(*mock_sequencer_, transposeTrack(0, 12)).Times(1);  // Max transpose
    
    sendMidiCC(1, 123, 0);   // -12 semitones
    sendMidiCC(1, 123, 64);  // 0 semitones
    sendMidiCC(1, 123, 127); // +12 semitones
}

// =============================================================================
// STEP PROGRAMMING TESTS
// =============================================================================

TEST_F(SequencerMidiControllerTest, StepSelection) {
    // Test step selection (CC 90)
    sendMidiCC(1, 90, 0);   // Step 1
    EXPECT_EQ(controller_->getCurrentStep(), 0);
    
    sendMidiCC(1, 90, 63);  // ~Step 8
    EXPECT_EQ(controller_->getCurrentStep(), 7);
    
    sendMidiCC(1, 90, 127); // Step 16
    EXPECT_EQ(controller_->getCurrentStep(), 15);
}

TEST_F(SequencerMidiControllerTest, StepTriggerControl) {
    // Set current track to 1
    controller_->setCurrentTrack(1);
    
    // Test individual step triggers (CC 91-106)
    EXPECT_CALL(*mock_sequencer_, toggleStep(1, 0)).Times(1);  // Step 1
    EXPECT_CALL(*mock_sequencer_, toggleStep(1, 7)).Times(1);  // Step 8
    EXPECT_CALL(*mock_sequencer_, toggleStep(1, 15)).Times(1); // Step 16
    
    sendMidiCC(1, 91, 127);  // Step 1 trigger
    sendMidiCC(1, 98, 100);  // Step 8 trigger
    sendMidiCC(1, 106, 80);  // Step 16 trigger
}

TEST_F(SequencerMidiControllerTest, StepVelocityControl) {
    // Set current step to 3
    controller_->setCurrentStep(3);
    controller_->setCurrentTrack(0);
    
    // Mock step data will be modified
    StepSequencer::Step modified_step = default_step_;
    
    EXPECT_CALL(*mock_sequencer_, setStep(0, 3, _))
        .Times(AtLeast(1))
        .WillRepeatedly([&](int track, int step, const StepSequencer::Step& step_data) {
            modified_step = step_data;
        });
    
    // Set step velocity (CC 107)
    sendMidiCC(1, 107, 63);  // ~50% velocity
    
    // Should set velocity to ~50% of 127
    EXPECT_NEAR(modified_step.velocity, 63, 1);
}

TEST_F(SequencerMidiControllerTest, StepNoteControl) {
    controller_->setCurrentStep(5);
    controller_->setCurrentTrack(2);
    
    StepSequencer::Step modified_step = default_step_;
    
    EXPECT_CALL(*mock_sequencer_, setStep(2, 5, _))
        .Times(AtLeast(1))
        .WillRepeatedly([&](int track, int step, const StepSequencer::Step& step_data) {
            modified_step = step_data;
        });
    
    // Set step note (CC 110)
    sendMidiCC(1, 110, 64);  // ~Note 64
    
    EXPECT_NEAR(modified_step.note, 64, 1);
}

// =============================================================================
// PARAMETER LOCK TESTS
// =============================================================================

TEST_F(SequencerMidiControllerTest, ParameterLockMode) {
    // Test parameter lock mode toggle (CC 70)
    EXPECT_FALSE(controller_->isParameterLockModeActive());
    
    sendMidiCC(1, 70, 127); // Enter lock mode
    EXPECT_TRUE(controller_->isParameterLockModeActive());
    
    sendMidiCC(1, 70, 100); // Exit lock mode
    EXPECT_FALSE(controller_->isParameterLockModeActive());
}

TEST_F(SequencerMidiControllerTest, ParameterLockValueSetting) {
    // Enter parameter lock mode
    sendMidiCC(1, 70, 127);
    EXPECT_TRUE(controller_->isParameterLockModeActive());
    
    // Select parameter to lock (CC 71)
    sendMidiCC(1, 71, 100); // Parameter ID ~100
    
    // Set current position
    controller_->setCurrentTrack(1);
    controller_->setCurrentStep(4);
    
    // Expect parameter lock to be set
    EXPECT_CALL(*mock_sequencer_, setStepParameterLock(1, 4, 
                static_cast<Parameters::ParameterID>(100), 
                ::testing::FloatNear(0.5f, 0.01f)))
        .Times(1);
    
    // Set parameter lock value (CC 72)
    sendMidiCC(1, 72, 64); // ~50% value
}

TEST_F(SequencerMidiControllerTest, ParameterLockClearAll) {
    controller_->setCurrentTrack(2);
    controller_->setCurrentStep(7);
    
    EXPECT_CALL(*mock_sequencer_, clearAllStepParameterLocks(2, 7)).Times(1);
    
    // Clear all parameter locks (CC 74)
    sendMidiCC(1, 74, 127);
}

// =============================================================================
// PERFORMANCE CONTROL TESTS
// =============================================================================

TEST_F(SequencerMidiControllerTest, MuteAllTracks) {
    // Mock all track mute operations
    for (int i = 0; i < StepSequencer::MAX_TRACKS; ++i) {
        EXPECT_CALL(*mock_sequencer_, muteTrack(i, true)).Times(1);
    }
    
    // Send mute all command (CC 61)
    sendMidiCC(1, 61, 127);
}

TEST_F(SequencerMidiControllerTest, ClearAllSolo) {
    // Mock all track solo clear operations
    for (int i = 0; i < StepSequencer::MAX_TRACKS; ++i) {
        EXPECT_CALL(*mock_sequencer_, soloTrack(i, false)).Times(1);
    }
    
    // Send clear all solo command (CC 62)
    sendMidiCC(1, 62, 100);
}

TEST_F(SequencerMidiControllerTest, RandomizeTrack) {
    controller_->setCurrentTrack(3);
    
    EXPECT_CALL(*mock_sequencer_, randomizePattern(3, 0.5f)).Times(1);
    
    // Send randomize command (CC 64)
    sendMidiCC(1, 64, 127);
}

TEST_F(SequencerMidiControllerTest, ShiftTrackPattern) {
    controller_->setCurrentTrack(2);
    
    EXPECT_CALL(*mock_sequencer_, shiftPattern(2, -1)).Times(1); // Shift left
    EXPECT_CALL(*mock_sequencer_, shiftPattern(2, 1)).Times(1);  // Shift right
    
    // Send shift left command (CC 65)
    sendMidiCC(1, 65, 127);
    
    // Send shift right command (CC 66)
    sendMidiCC(1, 66, 80);
}

TEST_F(SequencerMidiControllerTest, ClearTrack) {
    controller_->setCurrentTrack(1);
    
    // Expect all steps to be cleared
    for (int step = 0; step < StepSequencer::MAX_STEPS; ++step) {
        EXPECT_CALL(*mock_sequencer_, clearStep(1, step)).Times(1);
    }
    
    // Send clear track command (CC 67)
    sendMidiCC(1, 67, 127);
}

// =============================================================================
// NOTE INPUT TESTS
// =============================================================================

TEST_F(SequencerMidiControllerTest, NoteInputStepProgramming) {
    controller_->setCurrentTrack(0);
    controller_->setCurrentStep(5);
    
    StepSequencer::Step modified_step = default_step_;
    
    EXPECT_CALL(*mock_sequencer_, setStep(0, 5, _))
        .Times(1)
        .WillOnce([&](int track, int step, const StepSequencer::Step& step_data) {
            modified_step = step_data;
        });
    
    // Send note on
    EXPECT_TRUE(controller_->processSequencerNoteOn(0, 72, 110)); // C5, velocity 110
    
    // Verify step was programmed
    EXPECT_EQ(modified_step.note, 72);
    EXPECT_EQ(modified_step.velocity, 110);
    EXPECT_TRUE(modified_step.active);
    
    // Verify step advanced to next position
    EXPECT_EQ(controller_->getCurrentStep(), 6);
}

TEST_F(SequencerMidiControllerTest, NoteInputStepAdvancement) {
    controller_->setCurrentTrack(0);
    controller_->setCurrentStep(15); // Last step
    
    EXPECT_CALL(*mock_sequencer_, setStep(0, 15, _)).Times(1);
    EXPECT_CALL(*mock_sequencer_, getPatternLength()).WillOnce(Return(16));
    
    // Send note on at last step
    controller_->processSequencerNoteOn(0, 60, 100);
    
    // Should wrap around to first step
    EXPECT_EQ(controller_->getCurrentStep(), 0);
}

// =============================================================================
// MIDI MAPPING TESTS
// =============================================================================

TEST_F(SequencerMidiControllerTest, CustomMidiMapping) {
    // Test custom MIDI mapping assignment
    controller_->assignSequencerMidiCC(2, 50, 
        SequencerMidiController::SequencerParameterID::TRANSPORT_PLAY);
    
    // Verify mapping was set
    auto mapped_param = controller_->getSequencerMidiMapping(2, 50);
    EXPECT_EQ(mapped_param, SequencerMidiController::SequencerParameterID::TRANSPORT_PLAY);
    
    // Test that custom mapping works
    EXPECT_CALL(*mock_sequencer_, isPlaying()).WillOnce(Return(false));
    EXPECT_CALL(*mock_sequencer_, play()).Times(1);
    
    sendMidiCC(2, 50, 127); // Should trigger play on channel 2, CC 50
}

TEST_F(SequencerMidiControllerTest, RemoveMidiMapping) {
    // First verify default mapping exists
    auto mapping = controller_->getSequencerMidiMapping(1, 80);
    EXPECT_EQ(mapping, SequencerMidiController::SequencerParameterID::TRANSPORT_PLAY);
    
    // Remove the mapping
    controller_->removeSequencerMidiCC(1, 80);
    
    // Verify mapping was removed
    mapping = controller_->getSequencerMidiMapping(1, 80);
    EXPECT_EQ(mapping, static_cast<SequencerMidiController::SequencerParameterID>(0));
    
    // Verify CC is no longer handled by sequencer
    EXPECT_FALSE(sendMidiCC(1, 80, 127));
}

TEST_F(SequencerMidiControllerTest, UnmappedMidiCC) {
    // Test that unmapped CCs return false
    EXPECT_FALSE(sendMidiCC(1, 127, 100)); // CC 127 not mapped by default
    EXPECT_FALSE(sendMidiCC(16, 80, 127));  // Channel 16 not mapped by default
    
    // Verify statistics show unmapped CCs
    auto stats = controller_->getStatistics();
    EXPECT_GT(stats.unmapped_ccs.load(), 0);
}

// =============================================================================
// STATISTICS AND DEBUGGING TESTS
// =============================================================================

TEST_F(SequencerMidiControllerTest, StatisticsTracking) {
    auto initial_stats = controller_->getStatistics();
    
    // Send various MIDI commands
    sendMidiCC(1, 80, 127);  // Transport
    sendMidiCC(1, 91, 100);  // Step
    sendMidiCC(1, 120, 64);  // Track
    sendMidiCC(1, 70, 127);  // Parameter lock
    sendMidiCC(1, 61, 80);   // Performance
    sendMidiCC(1, 99, 127);  // Unmapped
    
    auto final_stats = controller_->getStatistics();
    
    // Verify statistics were updated
    EXPECT_GT(final_stats.transport_commands.load(), initial_stats.transport_commands.load());
    EXPECT_GT(final_stats.step_commands.load(), initial_stats.step_commands.load());
    EXPECT_GT(final_stats.track_commands.load(), initial_stats.track_commands.load());
    EXPECT_GT(final_stats.param_lock_commands.load(), initial_stats.param_lock_commands.load());
    EXPECT_GT(final_stats.performance_commands.load(), initial_stats.performance_commands.load());
    EXPECT_GT(final_stats.unmapped_ccs.load(), initial_stats.unmapped_ccs.load());
}

TEST_F(SequencerMidiControllerTest, MidiMappingExport) {
    // Test MIDI mapping export
    std::string mapping_export = controller_->exportMidiMappings();
    
    // Should contain header and mappings
    EXPECT_TRUE(mapping_export.find("# Sequencer MIDI CC Mappings") != std::string::npos);
    EXPECT_TRUE(mapping_export.find("Channel  1 CC 80") != std::string::npos); // Transport play
    EXPECT_TRUE(mapping_export.find("Channel  1 CC 91") != std::string::npos); // Step 1 trigger
    
    // Should be formatted consistently
    EXPECT_TRUE(mapping_export.find("->") != std::string::npos);
}

// =============================================================================
// THREAD SAFETY TESTS
// =============================================================================

TEST_F(SequencerMidiControllerTest, ConcurrentMidiProcessing) {
    std::atomic<int> successful_commands{0};
    std::atomic<bool> stop_test{false};
    
    // Set up expectations for concurrent access
    EXPECT_CALL(*mock_sequencer_, isPlaying()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_sequencer_, play()).Times(AtLeast(1));
    
    // Create multiple threads sending MIDI commands
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&, i]() {
            while (!stop_test.load()) {
                if (controller_->processSequencerMidiCC(0, 80, 127)) {
                    successful_commands.fetch_add(1);
                }
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
    
    // Verify some commands were processed successfully
    EXPECT_GT(successful_commands.load(), 0);
}

// =============================================================================
// INTEGRATION TESTS
// =============================================================================

TEST_F(SequencerMidiControllerTest, CompleteWorkflowIntegration) {
    InSequence workflow_sequence;
    
    // 1. Select track 2
    sendMidiCC(1, 120, 31); // ~Track 2
    EXPECT_EQ(controller_->getCurrentTrack(), 1);
    
    // 2. Start playback
    EXPECT_CALL(*mock_sequencer_, isPlaying()).WillOnce(Return(false));
    EXPECT_CALL(*mock_sequencer_, play()).Times(1);
    sendMidiCC(1, 80, 127);
    
    // 3. Program some steps
    EXPECT_CALL(*mock_sequencer_, toggleStep(1, 0)).Times(1); // Step 1
    EXPECT_CALL(*mock_sequencer_, toggleStep(1, 3)).Times(1); // Step 4
    EXPECT_CALL(*mock_sequencer_, toggleStep(1, 7)).Times(1); // Step 8
    
    sendMidiCC(1, 91, 127);  // Step 1
    sendMidiCC(1, 94, 100);  // Step 4
    sendMidiCC(1, 98, 80);   // Step 8
    
    // 4. Set swing
    EXPECT_CALL(*mock_sequencer_, setSwing(0.25f)).Times(1);
    sendMidiCC(1, 86, 32); // ~25% swing
    
    // 5. Enter parameter lock mode and set a lock
    sendMidiCC(1, 70, 127); // Enter lock mode
    sendMidiCC(1, 71, 50);  // Select parameter 50
    sendMidiCC(1, 90, 16);  // Select step 2
    
    EXPECT_CALL(*mock_sequencer_, setStepParameterLock(1, 1, 
                static_cast<Parameters::ParameterID>(50), 
                ::testing::FloatNear(0.75f, 0.01f))).Times(1);
    sendMidiCC(1, 72, 96);  // Set lock value to ~75%
    
    // 6. Stop playback
    EXPECT_CALL(*mock_sequencer_, stop()).Times(1);
    EXPECT_CALL(*mock_sequencer_, reset()).Times(1);
    sendMidiCC(1, 81, 127);
}

} // Anonymous namespace

/**
 * @brief Main test runner
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
