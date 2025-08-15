#include "SequencerMidiController.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace MIDI {

SequencerMidiController& SequencerMidiController::getInstance() {
    static SequencerMidiController instance;
    return instance;
}

void SequencerMidiController::initialize(std::shared_ptr<StepSequencer> sequencer,
                                       std::shared_ptr<Parameters::ParameterManager> param_manager) {
    if (initialized_.load()) {
        std::cout << "[SequencerMidiController] Already initialized, skipping..." << std::endl;
        return;
    }
    
    sequencer_ = sequencer;
    param_manager_ = param_manager;
    
    // Setup default MIDI mappings
    setupDefaultMidiMappings();
    
    // Initialize state
    current_track_.store(0);
    current_step_.store(0);
    param_lock_mode_.store(false);
    selected_parameter_.store(0);
    
    initialized_.store(true);
    
    std::cout << "[SequencerMidiController] ⚡ Sequencer MIDI controller initialized" << std::endl;
    std::cout << "[SequencerMidiController] 🎛️ Default CC mappings loaded" << std::endl;
}

void SequencerMidiController::shutdown() {
    if (!initialized_.load()) return;
    
    {
        std::lock_guard<std::mutex> lock(mappings_mutex_);
        sequencer_midi_mappings_.clear();
    }
    
    param_lock_clipboard_.clear();
    
    sequencer_.reset();
    param_manager_.reset();
    
    initialized_.store(false);
    
    std::cout << "[SequencerMidiController] 💤 Sequencer MIDI controller shutdown" << std::endl;
}

// =============================================================================
// MIDI PROCESSING
// =============================================================================

bool SequencerMidiController::processSequencerMidiCC(uint8_t channel, uint8_t cc, uint8_t value) {
    if (!initialized_.load()) return false;
    
    // Look up sequencer parameter mapping
    SequencerParameterID param_id = getSequencerMidiMapping(channel + 1, cc);
    if (param_id == static_cast<SequencerParameterID>(0)) {
        stats_.unmapped_ccs.fetch_add(1);
        return false; // Not a sequencer CC, let general parameter system handle it
    }
    
    float normalized_value = midiToNormalized(value);
    
    // Log the event for debugging
    SequencerMidiEvent event(param_id, normalized_value, channel + 1, cc, value);
    logSequencerMidiEvent(event);
    
    // Route to appropriate handler based on parameter category
    uint32_t param_uint = static_cast<uint32_t>(param_id);
    
    if (param_uint >= 10000 && param_uint <= 10009) {
        // Transport Control (10000-10009)
        processTransportControl(param_id, normalized_value, value);
        stats_.transport_commands.fetch_add(1);
    }
    else if (param_uint >= 10010 && param_uint <= 10019) {
        // Track Control (10010-10019)
        processTrackControl(param_id, normalized_value, value);
        stats_.track_commands.fetch_add(1);
    }
    else if (param_uint >= 10020 && param_uint <= 10049) {
        // Step Programming (10020-10049)
        processStepControl(param_id, normalized_value, value);
        stats_.step_commands.fetch_add(1);
    }
    else if (param_uint >= 10050 && param_uint <= 10059) {
        // Parameter Lock Control (10050-10059)
        processParameterLockControl(param_id, normalized_value, value);
        stats_.param_lock_commands.fetch_add(1);
    }
    else if (param_uint >= 10060 && param_uint <= 10069) {
        // Performance Controls (10060-10069)
        processPerformanceControl(param_id, normalized_value, value);
        stats_.performance_commands.fetch_add(1);
    }
    
    return true; // Handled by sequencer controller
}

bool SequencerMidiController::processSequencerNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (!initialized_.load() || !sequencer_) return false;
    
    int current_track = current_track_.load();
    int current_step = current_step_.load();
    
    // Set the note for the current step
    auto step_data = sequencer_->getStep(current_track, current_step);
    step_data.note = note;
    step_data.velocity = velocity;
    step_data.active = true;
    
    sequencer_->setStep(current_track, current_step, step_data);
    
    std::cout << "[SequencerMidiController] 🎹 Note " << (int)note 
              << " (vel " << (int)velocity << ") set for Track " << current_track 
              << " Step " << (current_step + 1) << std::endl;
    
    // Advance to next step for easy programming
    current_step_.store((current_step + 1) % sequencer_->getPatternLength());
    
    return true;
}

bool SequencerMidiController::processSequencerNoteOff(uint8_t channel, uint8_t note) {
    // Note off doesn't need special handling for step programming
    return false;
}

// =============================================================================
// CONTROL PROCESSORS
// =============================================================================

void SequencerMidiController::processTransportControl(SequencerParameterID param_id, 
                                                     float normalized_value, uint8_t raw_value) {
    if (!sequencer_) return;
    
    switch (param_id) {
        case SequencerParameterID::TRANSPORT_PLAY:
            if (isMidiTrigger(raw_value)) {
                if (sequencer_->isPlaying()) {
                    sequencer_->stop();
                    std::cout << "[SequencerMidiController] ⏹️ MIDI Stop" << std::endl;
                } else {
                    sequencer_->play();
                    std::cout << "[SequencerMidiController] ▶️ MIDI Play" << std::endl;
                }
            }
            break;
            
        case SequencerParameterID::TRANSPORT_STOP:
            if (isMidiTrigger(raw_value)) {
                sequencer_->stop();
                sequencer_->reset();
                std::cout << "[SequencerMidiController] ⏹️ MIDI Stop & Reset" << std::endl;
            }
            break;
            
        case SequencerParameterID::TRANSPORT_PAUSE:
            if (isMidiTrigger(raw_value)) {
                if (sequencer_->isPlaying()) {
                    sequencer_->stop();
                    std::cout << "[SequencerMidiController] ⏸️ MIDI Pause" << std::endl;
                } else {
                    sequencer_->play();
                    std::cout << "[SequencerMidiController] ▶️ MIDI Continue" << std::endl;
                }
            }
            break;
            
        case SequencerParameterID::PATTERN_SELECT:
            // Pattern selection (0-15 patterns)
            {
                int pattern_id = static_cast<int>(normalized_value * 15.0f);
                std::cout << "[SequencerMidiController] 🎛️ MIDI Pattern Select: " << pattern_id << std::endl;
                // Note: Would need to implement pattern switching in sequencer
            }
            break;
            
        case SequencerParameterID::PATTERN_LENGTH:
            // Pattern length (1-16 steps)
            {
                int length = static_cast<int>(normalized_value * 15.0f) + 1;
                sequencer_->setPatternLength(length);
                std::cout << "[SequencerMidiController] 📏 MIDI Pattern Length: " << length << " steps" << std::endl;
            }
            break;
            
        case SequencerParameterID::SWING_AMOUNT:
            // Swing amount (0.0-1.0)
            sequencer_->setSwing(normalized_value);
            std::cout << "[SequencerMidiController] 🎵 MIDI Swing: " 
                      << std::fixed << std::setprecision(2) << (normalized_value * 100.0f) << "%" << std::endl;
            break;
            
        default:
            break;
    }
}

void SequencerMidiController::processTrackControl(SequencerParameterID param_id, 
                                                 float normalized_value, uint8_t raw_value) {
    if (!sequencer_) return;
    
    switch (param_id) {
        case SequencerParameterID::TRACK_SELECT:
            // Track selection (0-7)
            {
                int track_id = static_cast<int>(normalized_value * 7.0f);
                setCurrentTrack(track_id);
                std::cout << "[SequencerMidiController] 🎚️ MIDI Track Select: " << (track_id + 1) << std::endl;
            }
            break;
            
        case SequencerParameterID::TRACK_MUTE:
            if (isMidiTrigger(raw_value)) {
                int track_id = current_track_.load();
                bool current_mute = sequencer_->getTrack(track_id).muted.load();
                sequencer_->muteTrack(track_id, !current_mute);
                std::cout << "[SequencerMidiController] 🔇 MIDI Track " << (track_id + 1) 
                          << (current_mute ? " Unmuted" : " Muted") << std::endl;
            }
            break;
            
        case SequencerParameterID::TRACK_SOLO:
            if (isMidiTrigger(raw_value)) {
                int track_id = current_track_.load();
                bool current_solo = sequencer_->getTrack(track_id).solo.load();
                sequencer_->soloTrack(track_id, !current_solo);
                std::cout << "[SequencerMidiController] 🎯 MIDI Track " << (track_id + 1) 
                          << (current_solo ? " Solo Off" : " Solo On") << std::endl;
            }
            break;
            
        case SequencerParameterID::TRACK_TRANSPOSE:
            // Transpose in semitones (-12 to +12)
            {
                int semitones = static_cast<int>((normalized_value - 0.5f) * 24.0f);
                int track_id = current_track_.load();
                sequencer_->transposeTrack(track_id, semitones);
                std::cout << "[SequencerMidiController] 🎼 MIDI Track " << (track_id + 1) 
                          << " Transpose: " << (semitones >= 0 ? "+" : "") << semitones << " semitones" << std::endl;
            }
            break;
            
        default:
            break;
    }
}

void SequencerMidiController::processStepControl(SequencerParameterID param_id, 
                                                float normalized_value, uint8_t raw_value) {
    if (!sequencer_) return;
    
    int current_track = current_track_.load();
    int current_step = current_step_.load();
    
    // Check if it's a step trigger (CC 91-106 for steps 1-16)
    int step_index = getStepFromParameterID(param_id);
    if (step_index >= 0) {
        // Step trigger control
        if (isMidiTrigger(raw_value)) {
            sequencer_->toggleStep(current_track, step_index);
            auto step_data = sequencer_->getStep(current_track, step_index);
            std::cout << "[SequencerMidiController] 🎵 MIDI Step " << (step_index + 1) 
                      << " Track " << (current_track + 1) 
                      << (step_data.active ? " ON" : " OFF") << std::endl;
        }
        return;
    }
    
    // Other step controls affect current step
    switch (param_id) {
        case SequencerParameterID::STEP_SELECT:
            // Step selection (0-15)
            {
                int step_id = static_cast<int>(normalized_value * 15.0f);
                setCurrentStep(step_id);
                std::cout << "[SequencerMidiController] 🎚️ MIDI Step Select: " << (step_id + 1) << std::endl;
            }
            break;
            
        case SequencerParameterID::STEP_VELOCITY:
            // Step velocity (0-127)
            {
                auto step_data = sequencer_->getStep(current_track, current_step);
                step_data.velocity = static_cast<uint8_t>(normalized_value * 127.0f);
                sequencer_->setStep(current_track, current_step, step_data);
                std::cout << "[SequencerMidiController] 🔊 MIDI Step " << (current_step + 1) 
                          << " Velocity: " << (int)step_data.velocity << std::endl;
            }
            break;
            
        case SequencerParameterID::STEP_NOTE:
            // Step note (0-127)
            {
                auto step_data = sequencer_->getStep(current_track, current_step);
                step_data.note = static_cast<uint8_t>(normalized_value * 127.0f);
                sequencer_->setStep(current_track, current_step, step_data);
                std::cout << "[SequencerMidiController] 🎹 MIDI Step " << (current_step + 1) 
                          << " Note: " << (int)step_data.note << std::endl;
            }
            break;
            
        case SequencerParameterID::STEP_LENGTH:
            // Step length (1-24 ticks)
            {
                auto step_data = sequencer_->getStep(current_track, current_step);
                step_data.length = static_cast<uint8_t>(normalized_value * 23.0f) + 1;
                sequencer_->setStep(current_track, current_step, step_data);
                std::cout << "[SequencerMidiController] ⏱️ MIDI Step " << (current_step + 1) 
                          << " Length: " << (int)step_data.length << " ticks" << std::endl;
            }
            break;
            
        default:
            break;
    }
}

void SequencerMidiController::processParameterLockControl(SequencerParameterID param_id, 
                                                         float normalized_value, uint8_t raw_value) {
    switch (param_id) {
        case SequencerParameterID::PARAM_LOCK_MODE:
            if (isMidiTrigger(raw_value)) {
                bool current_mode = param_lock_mode_.load();
                param_lock_mode_.store(!current_mode);
                std::cout << "[SequencerMidiController] 🔒 MIDI Parameter Lock Mode: " 
                          << (!current_mode ? "ON" : "OFF") << std::endl;
            }
            break;
            
        case SequencerParameterID::PARAM_LOCK_PARAMETER:
            // Select parameter to lock (0-255)
            {
                uint32_t param_id_value = static_cast<uint32_t>(normalized_value * 255.0f);
                selected_parameter_.store(param_id_value);
                std::cout << "[SequencerMidiController] 🎛️ MIDI Selected Parameter: " << param_id_value << std::endl;
            }
            break;
            
        case SequencerParameterID::PARAM_LOCK_VALUE:
            // Set parameter lock value (0.0-1.0)
            if (param_lock_mode_.load() && sequencer_) {
                uint32_t param_id_value = selected_parameter_.load();
                if (param_id_value > 0) {
                    auto param_id = static_cast<Parameters::ParameterID>(param_id_value);
                    int track = current_track_.load();
                    int step = current_step_.load();
                    
                    sequencer_->setStepParameterLock(track, step, param_id, normalized_value);
                    std::cout << "[SequencerMidiController] 🔒 MIDI Parameter Lock Set: "
                              << "Track " << (track + 1) << " Step " << (step + 1)
                              << " Param " << param_id_value << " = " 
                              << std::fixed << std::setprecision(3) << normalized_value << std::endl;
                }
            }
            break;
            
        case SequencerParameterID::PARAM_LOCK_CLEAR:
            if (isMidiTrigger(raw_value) && sequencer_) {
                uint32_t param_id_value = selected_parameter_.load();
                if (param_id_value > 0) {
                    auto param_id = static_cast<Parameters::ParameterID>(param_id_value);
                    int track = current_track_.load();
                    int step = current_step_.load();
                    
                    sequencer_->clearStepParameterLock(track, step, param_id);
                    std::cout << "[SequencerMidiController] 🚫 MIDI Parameter Lock Cleared: "
                              << "Track " << (track + 1) << " Step " << (step + 1)
                              << " Param " << param_id_value << std::endl;
                }
            }
            break;
            
        case SequencerParameterID::PARAM_LOCK_CLEAR_ALL:
            if (isMidiTrigger(raw_value) && sequencer_) {
                int track = current_track_.load();
                int step = current_step_.load();
                
                sequencer_->clearAllStepParameterLocks(track, step);
                std::cout << "[SequencerMidiController] 🧹 MIDI All Parameter Locks Cleared: "
                          << "Track " << (track + 1) << " Step " << (step + 1) << std::endl;
            }
            break;
            
        case SequencerParameterID::PARAM_LOCK_COPY:
            if (isMidiTrigger(raw_value) && sequencer_) {
                int track = current_track_.load();
                int step = current_step_.load();
                
                // Copy parameter locks to clipboard
                auto locked_params = sequencer_->getStepParameterLocks(track, step);
                param_lock_clipboard_.clear();
                
                for (auto param_id : locked_params) {
                    float value = sequencer_->getStepParameterLock(track, step, param_id);
                    param_lock_clipboard_[param_id] = value;
                }
                
                std::cout << "[SequencerMidiController] 📋 MIDI Parameter Locks Copied: "
                          << param_lock_clipboard_.size() << " locks from Track " 
                          << (track + 1) << " Step " << (step + 1) << std::endl;
            }
            break;
            
        case SequencerParameterID::PARAM_LOCK_PASTE:
            if (isMidiTrigger(raw_value) && sequencer_) {
                int track = current_track_.load();
                int step = current_step_.load();
                
                // Paste parameter locks from clipboard
                for (const auto& [param_id, value] : param_lock_clipboard_) {
                    sequencer_->setStepParameterLock(track, step, param_id, value);
                }
                
                std::cout << "[SequencerMidiController] 📌 MIDI Parameter Locks Pasted: "
                          << param_lock_clipboard_.size() << " locks to Track " 
                          << (track + 1) << " Step " << (step + 1) << std::endl;
            }
            break;
            
        default:
            break;
    }
}

void SequencerMidiController::processPerformanceControl(SequencerParameterID param_id, 
                                                       float normalized_value, uint8_t raw_value) {
    if (!sequencer_) return;
    
    switch (param_id) {
        case SequencerParameterID::MUTE_ALL:
            if (isMidiTrigger(raw_value)) {
                // Toggle mute for all tracks
                for (int i = 0; i < StepSequencer::MAX_TRACKS; ++i) {
                    bool current_mute = sequencer_->getTrack(i).muted.load();
                    sequencer_->muteTrack(i, !current_mute);
                }
                std::cout << "[SequencerMidiController] 🔇 MIDI Mute All Tracks" << std::endl;
            }
            break;
            
        case SequencerParameterID::SOLO_CLEAR:
            if (isMidiTrigger(raw_value)) {
                // Clear all solo states
                for (int i = 0; i < StepSequencer::MAX_TRACKS; ++i) {
                    sequencer_->soloTrack(i, false);
                }
                std::cout << "[SequencerMidiController] 🎯 MIDI Clear All Solo" << std::endl;
            }
            break;
            
        case SequencerParameterID::RANDOMIZE_TRACK:
            if (isMidiTrigger(raw_value)) {
                int track = current_track_.load();
                sequencer_->randomizePattern(track, 0.5f); // 50% probability
                std::cout << "[SequencerMidiController] 🎲 MIDI Randomize Track " << (track + 1) << std::endl;
            }
            break;
            
        case SequencerParameterID::SHIFT_TRACK_LEFT:
            if (isMidiTrigger(raw_value)) {
                int track = current_track_.load();
                sequencer_->shiftPattern(track, -1); // Shift left
                std::cout << "[SequencerMidiController] ⬅️ MIDI Shift Track " << (track + 1) << " Left" << std::endl;
            }
            break;
            
        case SequencerParameterID::SHIFT_TRACK_RIGHT:
            if (isMidiTrigger(raw_value)) {
                int track = current_track_.load();
                sequencer_->shiftPattern(track, 1); // Shift right
                std::cout << "[SequencerMidiController] ➡️ MIDI Shift Track " << (track + 1) << " Right" << std::endl;
            }
            break;
            
        case SequencerParameterID::CLEAR_TRACK:
            if (isMidiTrigger(raw_value)) {
                int track = current_track_.load();
                for (int step = 0; step < StepSequencer::MAX_STEPS; ++step) {
                    sequencer_->clearStep(track, step);
                }
                std::cout << "[SequencerMidiController] 🧹 MIDI Clear Track " << (track + 1) << std::endl;
            }
            break;
            
        default:
            break;
    }
}

// =============================================================================
// MIDI MAPPING MANAGEMENT
// =============================================================================

void SequencerMidiController::setupDefaultMidiMappings() {
    std::lock_guard<std::mutex> lock(mappings_mutex_);
    
    // Transport Control (CC 80-89)
    sequencer_midi_mappings_[getMappingKey(1, 80)] = SequencerParameterID::TRANSPORT_PLAY;
    sequencer_midi_mappings_[getMappingKey(1, 81)] = SequencerParameterID::TRANSPORT_STOP;
    sequencer_midi_mappings_[getMappingKey(1, 82)] = SequencerParameterID::TRANSPORT_PAUSE;
    sequencer_midi_mappings_[getMappingKey(1, 83)] = SequencerParameterID::TRANSPORT_BPM;
    sequencer_midi_mappings_[getMappingKey(1, 84)] = SequencerParameterID::PATTERN_SELECT;
    sequencer_midi_mappings_[getMappingKey(1, 85)] = SequencerParameterID::PATTERN_LENGTH;
    sequencer_midi_mappings_[getMappingKey(1, 86)] = SequencerParameterID::SWING_AMOUNT;
    
    // Track Control (CC 120-123)
    sequencer_midi_mappings_[getMappingKey(1, 120)] = SequencerParameterID::TRACK_SELECT;
    sequencer_midi_mappings_[getMappingKey(1, 121)] = SequencerParameterID::TRACK_MUTE;
    sequencer_midi_mappings_[getMappingKey(1, 122)] = SequencerParameterID::TRACK_SOLO;
    sequencer_midi_mappings_[getMappingKey(1, 123)] = SequencerParameterID::TRACK_TRANSPOSE;
    
    // Step Programming (CC 90-106 for step selection + individual steps)
    sequencer_midi_mappings_[getMappingKey(1, 90)] = SequencerParameterID::STEP_SELECT;
    sequencer_midi_mappings_[getMappingKey(1, 91)] = SequencerParameterID::STEP_TRIGGER_01;
    sequencer_midi_mappings_[getMappingKey(1, 92)] = SequencerParameterID::STEP_TRIGGER_02;
    sequencer_midi_mappings_[getMappingKey(1, 93)] = SequencerParameterID::STEP_TRIGGER_03;
    sequencer_midi_mappings_[getMappingKey(1, 94)] = SequencerParameterID::STEP_TRIGGER_04;
    sequencer_midi_mappings_[getMappingKey(1, 95)] = SequencerParameterID::STEP_TRIGGER_05;
    sequencer_midi_mappings_[getMappingKey(1, 96)] = SequencerParameterID::STEP_TRIGGER_06;
    sequencer_midi_mappings_[getMappingKey(1, 97)] = SequencerParameterID::STEP_TRIGGER_07;
    sequencer_midi_mappings_[getMappingKey(1, 98)] = SequencerParameterID::STEP_TRIGGER_08;
    sequencer_midi_mappings_[getMappingKey(1, 99)] = SequencerParameterID::STEP_TRIGGER_09;
    sequencer_midi_mappings_[getMappingKey(1, 100)] = SequencerParameterID::STEP_TRIGGER_10;
    sequencer_midi_mappings_[getMappingKey(1, 101)] = SequencerParameterID::STEP_TRIGGER_11;
    sequencer_midi_mappings_[getMappingKey(1, 102)] = SequencerParameterID::STEP_TRIGGER_12;
    sequencer_midi_mappings_[getMappingKey(1, 103)] = SequencerParameterID::STEP_TRIGGER_13;
    sequencer_midi_mappings_[getMappingKey(1, 104)] = SequencerParameterID::STEP_TRIGGER_14;
    sequencer_midi_mappings_[getMappingKey(1, 105)] = SequencerParameterID::STEP_TRIGGER_15;
    sequencer_midi_mappings_[getMappingKey(1, 106)] = SequencerParameterID::STEP_TRIGGER_16;
    
    // Step editing (CC 107-111)
    sequencer_midi_mappings_[getMappingKey(1, 107)] = SequencerParameterID::STEP_VELOCITY;
    sequencer_midi_mappings_[getMappingKey(1, 108)] = SequencerParameterID::STEP_PROBABILITY;
    sequencer_midi_mappings_[getMappingKey(1, 109)] = SequencerParameterID::STEP_MICRO_TIMING;
    sequencer_midi_mappings_[getMappingKey(1, 110)] = SequencerParameterID::STEP_NOTE;
    sequencer_midi_mappings_[getMappingKey(1, 111)] = SequencerParameterID::STEP_LENGTH;
    
    // Parameter Lock Control (CC 70-76)
    sequencer_midi_mappings_[getMappingKey(1, 70)] = SequencerParameterID::PARAM_LOCK_MODE;
    sequencer_midi_mappings_[getMappingKey(1, 71)] = SequencerParameterID::PARAM_LOCK_PARAMETER;
    sequencer_midi_mappings_[getMappingKey(1, 72)] = SequencerParameterID::PARAM_LOCK_VALUE;
    sequencer_midi_mappings_[getMappingKey(1, 73)] = SequencerParameterID::PARAM_LOCK_CLEAR;
    sequencer_midi_mappings_[getMappingKey(1, 74)] = SequencerParameterID::PARAM_LOCK_CLEAR_ALL;
    sequencer_midi_mappings_[getMappingKey(1, 75)] = SequencerParameterID::PARAM_LOCK_COPY;
    sequencer_midi_mappings_[getMappingKey(1, 76)] = SequencerParameterID::PARAM_LOCK_PASTE;
    
    // Performance Controls (CC 60-69)
    sequencer_midi_mappings_[getMappingKey(1, 60)] = SequencerParameterID::FILL_MODE;
    sequencer_midi_mappings_[getMappingKey(1, 61)] = SequencerParameterID::MUTE_ALL;
    sequencer_midi_mappings_[getMappingKey(1, 62)] = SequencerParameterID::SOLO_CLEAR;
    sequencer_midi_mappings_[getMappingKey(1, 63)] = SequencerParameterID::PATTERN_CHAIN;
    sequencer_midi_mappings_[getMappingKey(1, 64)] = SequencerParameterID::RANDOMIZE_TRACK;
    sequencer_midi_mappings_[getMappingKey(1, 65)] = SequencerParameterID::SHIFT_TRACK_LEFT;
    sequencer_midi_mappings_[getMappingKey(1, 66)] = SequencerParameterID::SHIFT_TRACK_RIGHT;
    sequencer_midi_mappings_[getMappingKey(1, 67)] = SequencerParameterID::CLEAR_TRACK;
    sequencer_midi_mappings_[getMappingKey(1, 68)] = SequencerParameterID::COPY_TRACK;
    sequencer_midi_mappings_[getMappingKey(1, 69)] = SequencerParameterID::PASTE_TRACK;
    
    std::cout << "[SequencerMidiController] 📋 Loaded " << sequencer_midi_mappings_.size() 
              << " default MIDI CC mappings" << std::endl;
}

void SequencerMidiController::assignSequencerMidiCC(uint8_t channel, uint8_t cc, 
                                                   SequencerParameterID param_id) {
    std::lock_guard<std::mutex> lock(mappings_mutex_);
    uint16_t key = getMappingKey(channel, cc);
    sequencer_midi_mappings_[key] = param_id;
    
    std::cout << "[SequencerMidiController] Assigned MIDI CC" << (int)cc 
              << " Ch" << (int)channel << " --> Sequencer Parameter " 
              << static_cast<uint32_t>(param_id) << std::endl;
}

void SequencerMidiController::removeSequencerMidiCC(uint8_t channel, uint8_t cc) {
    std::lock_guard<std::mutex> lock(mappings_mutex_);
    uint16_t key = getMappingKey(channel, cc);
    sequencer_midi_mappings_.erase(key);
}

SequencerMidiController::SequencerParameterID 
SequencerMidiController::getSequencerMidiMapping(uint8_t channel, uint8_t cc) const {
    std::lock_guard<std::mutex> lock(mappings_mutex_);
    uint16_t key = getMappingKey(channel, cc);
    auto it = sequencer_midi_mappings_.find(key);
    return (it != sequencer_midi_mappings_.end()) ? it->second : static_cast<SequencerParameterID>(0);
}

// =============================================================================
// STATE MANAGEMENT
// =============================================================================

void SequencerMidiController::setCurrentTrack(int track_id) {
    if (track_id >= 0 && track_id < StepSequencer::MAX_TRACKS) {
        current_track_.store(track_id);
    }
}

void SequencerMidiController::setCurrentStep(int step_id) {
    if (step_id >= 0 && step_id < StepSequencer::MAX_STEPS) {
        current_step_.store(step_id);
    }
}

// =============================================================================
// UTILITY METHODS
// =============================================================================

uint16_t SequencerMidiController::getMappingKey(uint8_t channel, uint8_t cc) const {
    return (static_cast<uint16_t>(channel) << 8) | cc;
}

float SequencerMidiController::midiToNormalized(uint8_t midi_value) const {
    return static_cast<float>(midi_value) / 127.0f;
}

bool SequencerMidiController::isMidiTrigger(uint8_t midi_value) const {
    return midi_value >= 64; // MIDI values >= 64 are considered "on"
}

int SequencerMidiController::getStepFromParameterID(SequencerParameterID param_id) const {
    uint32_t param_uint = static_cast<uint32_t>(param_id);
    if (param_uint >= 10021 && param_uint <= 10036) {
        return param_uint - 10021; // Returns 0-15 for steps 1-16
    }
    return -1;
}

void SequencerMidiController::logSequencerMidiEvent(const SequencerMidiEvent& event) const {
    // Only log in debug builds to avoid console spam
    #ifdef DEBUG_SEQUENCER_MIDI
    std::cout << "[SequencerMidiController] 🎛️ MIDI Event: Ch" << (int)event.midi_channel 
              << " CC" << (int)event.midi_cc << " (" << (int)event.raw_midi_value 
              << ") --> Sequencer Param " << static_cast<uint32_t>(event.parameter_id)
              << " (" << std::fixed << std::setprecision(3) << event.normalized_value << ")" << std::endl;
    #endif
}

// =============================================================================
// DEBUGGING AND STATISTICS
// =============================================================================

void SequencerMidiController::printSequencerMidiStatistics() const {
    std::cout << "\n🎛️ ===== SEQUENCER MIDI CONTROL STATISTICS =====" << std::endl;
    std::cout << "Transport Commands:     " << stats_.transport_commands.load() << std::endl;
    std::cout << "Step Commands:          " << stats_.step_commands.load() << std::endl;
    std::cout << "Track Commands:         " << stats_.track_commands.load() << std::endl;
    std::cout << "Parameter Lock Commands:" << stats_.param_lock_commands.load() << std::endl;
    std::cout << "Performance Commands:   " << stats_.performance_commands.load() << std::endl;
    std::cout << "Unmapped CCs:           " << stats_.unmapped_ccs.load() << std::endl;
    std::cout << "MIDI Learn Assignments: " << stats_.midi_learn_assignments.load() << std::endl;
    
    std::cout << "\n🎚️ Current State:" << std::endl;
    std::cout << "Current Track:          " << (current_track_.load() + 1) << std::endl;
    std::cout << "Current Step:           " << (current_step_.load() + 1) << std::endl;
    std::cout << "Parameter Lock Mode:    " << (param_lock_mode_.load() ? "ON" : "OFF") << std::endl;
    std::cout << "Selected Parameter:     " << selected_parameter_.load() << std::endl;
    
    {
        std::lock_guard<std::mutex> lock(mappings_mutex_);
        std::cout << "Active MIDI Mappings:   " << sequencer_midi_mappings_.size() << std::endl;
    }
    std::cout << "=============================================\n" << std::endl;
}

std::string SequencerMidiController::exportMidiMappings() const {
    std::lock_guard<std::mutex> lock(mappings_mutex_);
    std::ostringstream oss;
    
    oss << "# Sequencer MIDI CC Mappings\n";
    oss << "# Format: Channel CC -> Parameter ID\n\n";
    
    for (const auto& [key, param_id] : sequencer_midi_mappings_) {
        uint8_t channel = (key >> 8) & 0xFF;
        uint8_t cc = key & 0xFF;
        oss << "Channel " << std::setw(2) << (int)channel 
            << " CC" << std::setw(3) << (int)cc 
            << " -> Parameter " << std::setw(5) << static_cast<uint32_t>(param_id) << "\n";
    }
    
    return oss.str();
}

} // namespace MIDI
