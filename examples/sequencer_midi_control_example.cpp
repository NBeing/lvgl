#include "components/midi/SequencerMidiController.h"
#include "components/midi/StepSequencer.h"
#include "components/parameter/ParameterManager.h"
#include "components/midi/EnhancedRTClockManager.h"
#include <iostream>
#include <thread>
#include <chrono>

/**
 * @brief Complete example of MIDI-controlled step sequencer
 * 
 * This example demonstrates how to set up and use the comprehensive
 * MIDI control system for the step sequencer. It shows:
 * 
 * 1. System initialization and setup
 * 2. Custom MIDI mapping configuration
 * 3. Simulated MIDI input processing
 * 4. Live performance workflow
 * 5. Parameter lock programming
 * 6. Statistics monitoring
 */

class SequencerMidiExample {
private:
    // Core components
    std::shared_ptr<MIDI::StepSequencer> sequencer_;
    std::shared_ptr<Parameters::ParameterManager> param_manager_;
    MIDI::SequencerMidiController* midi_controller_;
    
    // MIDI processing
    bool running_;
    
public:
    SequencerMidiExample() : running_(false) {
        std::cout << "🎛️ Initializing MIDI-Controlled Step Sequencer..." << std::endl;
        
        // Initialize core components
        initializeComponents();
        
        // Setup custom MIDI mappings
        setupCustomMidiMappings();
        
        std::cout << "✅ MIDI sequencer system ready!" << std::endl;
    }
    
    ~SequencerMidiExample() {
        shutdown();
    }
    
private:
    void initializeComponents() {
        // Create parameter manager
        param_manager_ = std::make_shared<Parameters::ParameterManager>();
        param_manager_->initialize();
        
        // Create step sequencer
        sequencer_ = std::make_shared<MIDI::StepSequencer>();
        sequencer_->setParameterManager(param_manager_);
        
        // Initialize MIDI controller
        midi_controller_ = &MIDI::SequencerMidiController::getInstance();
        midi_controller_->initialize(sequencer_, param_manager_);
        
        std::cout << "🔧 Core components initialized" << std::endl;
    }
    
    void setupCustomMidiMappings() {
        // Custom mappings for Elektron Digitakt-style workflow
        std::cout << "🎚️ Setting up custom MIDI mappings..." << std::endl;
        
        // Track selection on different channel
        midi_controller_->assignSequencerMidiCC(
            2, 1, // Channel 2, CC 1 (Mod Wheel)
            MIDI::SequencerMidiController::SequencerParameterID::TRACK_SELECT
        );
        
        // Pattern selection on encoder
        midi_controller_->assignSequencerMidiCC(
            1, 16, // Channel 1, CC 16
            MIDI::SequencerMidiController::SequencerParameterID::PATTERN_SELECT
        );
        
        // Performance controls on channel 3
        midi_controller_->assignSequencerMidiCC(
            3, 10, // Channel 3, CC 10
            MIDI::SequencerMidiController::SequencerParameterID::RANDOMIZE_TRACK
        );
        
        midi_controller_->assignSequencerMidiCC(
            3, 11, // Channel 3, CC 11
            MIDI::SequencerMidiController::SequencerParameterID::SHIFT_TRACK_LEFT
        );
        
        midi_controller_->assignSequencerMidiCC(
            3, 12, // Channel 3, CC 12
            MIDI::SequencerMidiController::SequencerParameterID::SHIFT_TRACK_RIGHT
        );
        
        std::cout << "📋 Custom MIDI mappings configured" << std::endl;
    }
    
public:
    /**
     * @brief Demonstrate basic transport control via MIDI
     */
    void demonstrateTransportControl() {
        std::cout << "\n🎵 === TRANSPORT CONTROL DEMO ===" << std::endl;
        
        // Start playback via MIDI
        std::cout << "▶️ Starting playback via MIDI (CC 80)..." << std::endl;
        simulateMidiCC(1, 80, 127); // Transport Play
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Set swing via MIDI
        std::cout << "🎵 Setting swing to 25% via MIDI (CC 86)..." << std::endl;
        simulateMidiCC(1, 86, 32); // 25% swing
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Change pattern length via MIDI
        std::cout << "📏 Setting pattern length to 8 steps via MIDI (CC 85)..." << std::endl;
        simulateMidiCC(1, 85, 63); // 8 steps
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Stop playback via MIDI
        std::cout << "⏹️ Stopping playback via MIDI (CC 81)..." << std::endl;
        simulateMidiCC(1, 81, 127); // Transport Stop
    }
    
    /**
     * @brief Demonstrate step programming via MIDI
     */
    void demonstrateStepProgramming() {
        std::cout << "\n🎛️ === STEP PROGRAMMING DEMO ===" << std::endl;
        
        // Select track 1
        std::cout << "🎚️ Selecting track 1 via MIDI (CC 120)..." << std::endl;
        simulateMidiCC(1, 120, 0);
        
        // Program steps using individual step triggers
        std::cout << "🥁 Programming drum pattern via MIDI:" << std::endl;
        
        // Kick on steps 1, 5, 9, 13
        std::cout << "  - Kick: Steps 1, 5, 9, 13" << std::endl;
        simulateMidiCC(1, 91, 127);  // Step 1
        simulateMidiCC(1, 95, 127);  // Step 5
        simulateMidiCC(1, 99, 127);  // Step 9
        simulateMidiCC(1, 103, 127); // Step 13
        
        // Select track 2 for snare
        std::cout << "🎚️ Selecting track 2..." << std::endl;
        simulateMidiCC(1, 120, 18); // ~Track 2
        
        // Snare on steps 5, 13
        std::cout << "  - Snare: Steps 5, 13" << std::endl;
        simulateMidiCC(1, 95, 127);  // Step 5
        simulateMidiCC(1, 103, 127); // Step 13
        
        // Program step velocity and note via current step selection
        std::cout << "🎛️ Programming step details:" << std::endl;
        simulateMidiCC(1, 90, 32);   // Select step 5 (~25%)
        simulateMidiCC(1, 107, 100); // Set velocity to ~80%
        simulateMidiCC(1, 110, 64);  // Set note to E4
    }
    
    /**
     * @brief Demonstrate note input step programming
     */
    void demonstrateNoteInputProgramming() {
        std::cout << "\n🎹 === NOTE INPUT PROGRAMMING DEMO ===" << std::endl;
        
        // Select track 3 for melodic content
        std::cout << "🎚️ Selecting track 3 for melody..." << std::endl;
        simulateMidiCC(1, 120, 36); // ~Track 3
        
        // Select starting step
        std::cout << "📍 Starting at step 1..." << std::endl;
        simulateMidiCC(1, 90, 0); // Step 1
        
        // Program a melodic sequence using note input
        std::cout << "🎹 Programming melody via note input:" << std::endl;
        
        // C Major arpeggio
        simulateNoteOn(1, 60, 100); // C4 - Step 1
        std::cout << "  - Step 1: C4 (60)" << std::endl;
        
        simulateNoteOn(1, 64, 90);  // E4 - Step 2 (auto-advanced)
        std::cout << "  - Step 2: E4 (64)" << std::endl;
        
        simulateNoteOn(1, 67, 110); // G4 - Step 3
        std::cout << "  - Step 3: G4 (67)" << std::endl;
        
        simulateNoteOn(1, 72, 95);  // C5 - Step 4
        std::cout << "  - Step 4: C5 (72)" << std::endl;
        
        std::cout << "✅ Melody programmed with automatic step advancement" << std::endl;
    }
    
    /**
     * @brief Demonstrate parameter lock programming
     */
    void demonstrateParameterLocks() {
        std::cout << "\n🔒 === PARAMETER LOCK DEMO ===" << std::endl;
        
        // Enter parameter lock mode
        std::cout << "🔓 Entering parameter lock mode..." << std::endl;
        simulateMidiCC(1, 70, 127); // Enter lock mode
        
        // Select a parameter to lock (filter cutoff)
        std::cout << "🎛️ Selecting filter cutoff parameter..." << std::endl;
        simulateMidiCC(1, 71, 50); // Parameter ID 50 (example)
        
        // Select step to lock
        std::cout << "📍 Selecting step 3 for parameter lock..." << std::endl;
        simulateMidiCC(1, 90, 16); // Step 3 (~12.5% of range)
        
        // Set parameter lock value
        std::cout << "🔒 Setting parameter lock to 75%..." << std::endl;
        simulateMidiCC(1, 72, 96); // 75% value
        
        // Lock another step with different value
        std::cout << "📍 Selecting step 7..." << std::endl;
        simulateMidiCC(1, 90, 48); // Step 7 (~37.5% of range)
        
        std::cout << "🔒 Setting parameter lock to 25%..." << std::endl;
        simulateMidiCC(1, 72, 32); // 25% value
        
        // Copy locks to another step
        std::cout << "📋 Copying parameter locks..." << std::endl;
        simulateMidiCC(1, 75, 127); // Copy current step locks
        
        std::cout << "📍 Selecting step 11..." << std::endl;
        simulateMidiCC(1, 90, 80); // Step 11 (~62.5% of range)
        
        std::cout << "📌 Pasting parameter locks..." << std::endl;
        simulateMidiCC(1, 76, 127); // Paste locks
        
        // Exit parameter lock mode
        std::cout << "🔓 Exiting parameter lock mode..." << std::endl;
        simulateMidiCC(1, 70, 0); // Exit lock mode
    }
    
    /**
     * @brief Demonstrate performance controls
     */
    void demonstratePerformanceControls() {
        std::cout << "\n🎪 === PERFORMANCE CONTROLS DEMO ===" << std::endl;
        
        // Select track 1
        midi_controller_->setCurrentTrack(0);
        
        // Randomize current track
        std::cout << "🎲 Randomizing track 1..." << std::endl;
        simulateMidiCC(1, 64, 127); // Randomize track
        
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        // Shift pattern left
        std::cout << "⬅️ Shifting pattern left..." << std::endl;
        simulateMidiCC(1, 65, 127); // Shift left
        
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        // Shift pattern right
        std::cout << "➡️ Shifting pattern right..." << std::endl;
        simulateMidiCC(1, 66, 127); // Shift right
        
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        // Mute all tracks
        std::cout << "🔇 Muting all tracks..." << std::endl;
        simulateMidiCC(1, 61, 127); // Mute all
        
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        // Clear all solo
        std::cout << "🎯 Clearing all solo states..." << std::endl;
        simulateMidiCC(1, 62, 127); // Clear solo
    }
    
    /**
     * @brief Demonstrate custom MIDI mappings
     */
    void demonstrateCustomMappings() {
        std::cout << "\n⚙️ === CUSTOM MAPPINGS DEMO ===" << std::endl;
        
        // Use custom track selection mapping (Channel 2, CC 1)
        std::cout << "🎚️ Using custom track selection (Ch2, CC1)..." << std::endl;
        simulateMidiCC(2, 1, 64); // ~Track 4
        
        // Use custom pattern selection mapping (Channel 1, CC 16)
        std::cout << "🎛️ Using custom pattern selection (Ch1, CC16)..." << std::endl;
        simulateMidiCC(1, 16, 32); // ~Pattern 2
        
        // Use custom performance controls (Channel 3)
        std::cout << "🎲 Using custom randomize control (Ch3, CC10)..." << std::endl;
        simulateMidiCC(3, 10, 127); // Randomize
        
        std::cout << "⬅️ Using custom shift left (Ch3, CC11)..." << std::endl;
        simulateMidiCC(3, 11, 127); // Shift left
        
        std::cout << "➡️ Using custom shift right (Ch3, CC12)..." << std::endl;
        simulateMidiCC(3, 12, 127); // Shift right
    }
    
    /**
     * @brief Show statistics and mapping information
     */
    void showStatistics() {
        std::cout << "\n📊 === STATISTICS AND MAPPINGS ===" << std::endl;
        
        // Print detailed statistics
        midi_controller_->printSequencerMidiStatistics();
        
        // Export and display current mappings
        std::cout << "\n📋 Current MIDI Mappings:" << std::endl;
        std::string mappings = midi_controller_->exportMidiMappings();
        std::cout << mappings << std::endl;
    }
    
    /**
     * @brief Run complete demonstration
     */
    void runDemo() {
        std::cout << "🎛️ === COMPLETE MIDI SEQUENCER DEMO ===" << std::endl;
        std::cout << "This demo shows all MIDI control capabilities" << std::endl;
        std::cout << "=============================================" << std::endl;
        
        // Run all demonstrations
        demonstrateTransportControl();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        demonstrateStepProgramming();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        demonstrateNoteInputProgramming();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        demonstrateParameterLocks();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        demonstratePerformanceControls();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        demonstrateCustomMappings();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        showStatistics();
        
        std::cout << "\n🎉 === DEMO COMPLETE ===" << std::endl;
        std::cout << "All MIDI control features demonstrated successfully!" << std::endl;
        std::cout << "The sequencer is now fully MIDI controllable." << std::endl;
    }

private:
    /**
     * @brief Simulate MIDI CC input
     */
    void simulateMidiCC(uint8_t channel, uint8_t cc, uint8_t value) {
        bool handled = midi_controller_->processSequencerMidiCC(channel - 1, cc, value);
        if (!handled) {
            std::cout << "⚠️ MIDI CC" << (int)cc << " Ch" << (int)channel 
                      << " not handled by sequencer (value=" << (int)value << ")" << std::endl;
        }
        
        // Small delay to simulate real MIDI timing
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    /**
     * @brief Simulate MIDI Note On input
     */
    void simulateNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
        bool handled = midi_controller_->processSequencerNoteOn(channel - 1, note, velocity);
        if (!handled) {
            std::cout << "⚠️ MIDI Note " << (int)note << " Ch" << (int)channel 
                      << " not handled by sequencer (velocity=" << (int)velocity << ")" << std::endl;
        }
        
        // Small delay to simulate real MIDI timing
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    void shutdown() {
        if (midi_controller_) {
            midi_controller_->shutdown();
        }
        
        if (param_manager_) {
            param_manager_->shutdown();
        }
        
        std::cout << "💤 MIDI sequencer system shutdown complete" << std::endl;
    }
};

/**
 * @brief Main example entry point
 */
int main() {
    try {
        SequencerMidiExample example;
        example.runDemo();
        
        std::cout << "\n✨ Press Enter to exit..." << std::endl;
        std::cin.get();
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}
