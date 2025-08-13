#include "StepSequencer.h"
#include "ParameterLockManager.h"
#include <algorithm>
#include <random>
#include <iostream>

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
#include "debug/RTEventTracer.h"
#endif

namespace MIDI {

StepSequencer::StepSequencer() 
    : last_triggered_track_(-1), last_triggered_step_(-1) {
    // Initialize tracks with default settings
    for (int i = 0; i < MAX_TRACKS; ++i) {
        tracks_[i].channel = i + 1; // MIDI channels 1-8
    }
    
    // Initialize parameter lock manager
    parameter_lock_manager_ = std::make_unique<ParameterLockManager>();
}

void StepSequencer::onEvent(const ClockEvent& event) {
    switch (event.type) {
        case ClockEvent::START:
            std::cout << "[Sequencer] ▶️ START - Beginning sequence playback" << std::endl;
            reset();
            playing_.store(true);
            enqueueSequencerEvent(SequencerEvent::STEP_ADVANCE, -1, 0);
            break;
            
        case ClockEvent::STOP:
            std::cout << "[Sequencer] ⏹️ STOP - Stopping sequence playback" << std::endl;
            playing_.store(false);
            releaseAllNotes();
            break;
            
        case ClockEvent::CONTINUE:
            std::cout << "[Sequencer] ⏯️ CONTINUE - Resuming sequence playback" << std::endl;
            playing_.store(true);
            break;
            
        case ClockEvent::TICK:
            if (playing_.load()) {
                processClockTick(event.tick_count);
            }
            break;
    }
}

void StepSequencer::processClockTick(int tick_count) {
    current_tick_.store(tick_count);
    
    // Calculate current step (6 ticks per 16th note = 6 ticks per step)
    int base_step = (tick_count / 6) % pattern_length_.load();
    int tick_in_step = tick_count % 6;
    
    // Apply swing to determine actual step timing
    int actual_step = base_step;
    bool is_step_trigger = false;
    
    if (swing_amount_.load() > 0.0f && (base_step % 2 == 1)) {
        // Apply swing to off-beats (steps 1, 3, 5, etc.)
        int swing_delay = static_cast<int>(swing_amount_.load() * 2.0f); // 0-2 tick delay
        is_step_trigger = (tick_in_step == swing_delay);
    } else {
        // Straight timing for on-beats
        is_step_trigger = (tick_in_step == 0);
    }
    
    if (is_step_trigger && actual_step != current_step_.load()) {
        current_step_.store(actual_step);
        processStepTriggers(actual_step);
        enqueueSequencerEvent(SequencerEvent::STEP_ADVANCE, -1, actual_step);
        
        // Check for pattern completion
        if (actual_step == 0) {
            enqueueSequencerEvent(SequencerEvent::PATTERN_COMPLETE);
        }
    }
    
    // Process active notes (check for note-offs)
    processActiveNotes(tick_count);
}

void StepSequencer::processStepTriggers(int step) {
    // First, restore parameter locks from the previous step
    if (last_triggered_track_ >= 0 && last_triggered_step_ >= 0) {
        parameter_lock_manager_->restoreParametersFromStep(last_triggered_track_, last_triggered_step_);
    }
    
    for (int track_id = 0; track_id < MAX_TRACKS; ++track_id) {
        const auto& track = tracks_[track_id];
        
        // Skip muted tracks, unless soloed
        bool any_solo = std::any_of(tracks_.begin(), tracks_.end(), 
                                   [](const Track& t) { return t.solo.load(); });
        if (any_solo && !track.solo.load()) continue;
        if (track.muted.load() && !track.solo.load()) continue;
        
        const auto& step_data = track.steps[step];
        if (step_data.active) {
            // Apply parameter locks for this step BEFORE triggering the note
            if (!step_data.parameter_locks.empty()) {
                parameter_lock_manager_->applyStepParameterLocks(track_id, step, step_data.parameter_locks);
                last_triggered_track_ = track_id;
                last_triggered_step_ = step;
                
                #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
                std::string lock_data = "T" + std::to_string(track_id) + "S" + std::to_string(step) + 
                                       " Locks:" + std::to_string(step_data.parameter_locks.size());
                TRACE_PARAMETER_EVENT("StepSequencer", "ParameterLockManager", "applyStepLocks", lock_data.c_str());
                #endif
            }
            
            triggerNote(track_id, step, step_data);
        }
    }
}

void StepSequencer::triggerNote(int track_id, int step_id, const Step& step) {
    const auto& track = tracks_[track_id];
    
    // Calculate final note with transpose
    int final_note = step.note + track.transpose.load();
    final_note = std::clamp(final_note, 0, 127);
    
    // Calculate final velocity with accent
    int final_velocity = step.velocity;
    if (step.accent) {
        final_velocity = std::min(127, final_velocity + 20); // Accent adds velocity
    }
    
    // Schedule note-off
    int current_tick = current_tick_.load();
    int note_off_tick = current_tick + step.length;
    active_notes_.emplace_back(final_note, track.channel, note_off_tick);
    
    // Generate note-on event
    SequencerEvent note_on(SequencerEvent::NOTE_ON, track_id, step_id);
    note_on.note = final_note;
    note_on.velocity = final_velocity;
    note_on.channel = track.channel;
    sequencer_subject_.enqueueEvent(note_on);
    
    std::cout << "[Sequencer]   Track " << track_id + 1 
              << " Step " << step_id + 1 
              << " Note " << static_cast<int>(final_note) 
              << " Vel " << final_velocity 
              << " Ch " << static_cast<int>(track.channel) << std::endl;
}

void StepSequencer::processActiveNotes(int current_tick) {
    // Check for notes that need to be released
    auto it = active_notes_.begin();
    while (it != active_notes_.end()) {
        if (current_tick >= it->end_tick) {
            // Time to release this note
            releaseNote(it->note, it->channel);
            it = active_notes_.erase(it);
        } else {
            ++it;
        }
    }
}

void StepSequencer::releaseNote(uint8_t note, uint8_t channel) {
    SequencerEvent note_off(SequencerEvent::NOTE_OFF);
    note_off.note = note;
    note_off.velocity = 0;
    note_off.channel = channel;
    sequencer_subject_.enqueueEvent(note_off);
}

void StepSequencer::releaseAllNotes() {
    for (const auto& active_note : active_notes_) {
        releaseNote(active_note.note, active_note.channel);
    }
    active_notes_.clear();
}

// Observer management
void StepSequencer::addSequencerObserver(TypedObserver<SequencerEvent>* observer) {
    sequencer_subject_.addObserver(observer);
}

void StepSequencer::removeSequencerObserver(TypedObserver<SequencerEvent>* observer) {
    sequencer_subject_.removeObserver(observer);
}

void StepSequencer::processSequencerEvents() {
    sequencer_subject_.processQueuedEvents();
}

// Control methods
void StepSequencer::play() {
    playing_.store(true);
}

void StepSequencer::stop() {
    playing_.store(false);
    releaseAllNotes();
}

void StepSequencer::reset() {
    current_step_.store(0);
    current_tick_.store(0);
    releaseAllNotes();
}

// Step programming
void StepSequencer::setStep(int track, int step, const Step& step_data) {
    if (track >= 0 && track < MAX_TRACKS && step >= 0 && step < MAX_STEPS) {
        tracks_[track].steps[step] = step_data;
    }
}

void StepSequencer::clearStep(int track, int step) {
    if (track >= 0 && track < MAX_TRACKS && step >= 0 && step < MAX_STEPS) {
        tracks_[track].steps[step] = Step(); // Reset to default
    }
}

void StepSequencer::toggleStep(int track, int step) {
    if (track >= 0 && track < MAX_TRACKS && step >= 0 && step < MAX_STEPS) {
        auto& step_data = tracks_[track].steps[step];
        step_data.active = !step_data.active;
    }
}

const StepSequencer::Step& StepSequencer::getStep(int track, int step) const {
    static const Step empty_step;
    if (track >= 0 && track < MAX_TRACKS && step >= 0 && step < MAX_STEPS) {
        return tracks_[track].steps[step];
    }
    return empty_step;
}

// Track control
void StepSequencer::muteTrack(int track_id, bool mute) {
    if (track_id >= 0 && track_id < MAX_TRACKS) {
        tracks_[track_id].muted.store(mute);
    }
}

void StepSequencer::soloTrack(int track_id, bool solo) {
    if (track_id >= 0 && track_id < MAX_TRACKS) {
        tracks_[track_id].solo.store(solo);
    }
}

void StepSequencer::transposeTrack(int track_id, int semitones) {
    if (track_id >= 0 && track_id < MAX_TRACKS) {
        tracks_[track_id].transpose.store(std::clamp(semitones, -24, 24));
    }
}

// Pattern control
void StepSequencer::setPatternLength(int steps) {
    pattern_length_.store(std::clamp(steps, 1, MAX_STEPS));
}

void StepSequencer::setSwing(float swing_amount) {
    swing_amount_.store(std::clamp(swing_amount, 0.0f, 1.0f));
}

// Pattern manipulation
void StepSequencer::clearPattern() {
    for (auto& track : tracks_) {
        for (auto& step : track.steps) {
            step = Step(); // Reset to default
        }
    }
}

void StepSequencer::randomizePattern(int track_id, float probability) {
    if (track_id < 0 || track_id >= MAX_TRACKS) return;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> prob_dist(0.0, 1.0);
    std::uniform_int_distribution<> note_dist(36, 84); // C2 to C6
    std::uniform_int_distribution<> vel_dist(80, 127);
    
    auto& track = tracks_[track_id];
    for (auto& step : track.steps) {
        if (prob_dist(gen) < probability) {
            step.active = true;
            step.note = note_dist(gen);
            step.velocity = vel_dist(gen);
            step.accent = prob_dist(gen) < 0.2f; // 20% chance of accent
        } else {
            step.active = false;
        }
    }
}

void StepSequencer::shiftPattern(int track_id, int steps) {
    if (track_id < 0 || track_id >= MAX_TRACKS) return;
    
    auto& track = tracks_[track_id];
    if (steps == 0) return;
    
    // Normalize shift amount
    steps = steps % MAX_STEPS;
    if (steps < 0) steps += MAX_STEPS;
    
    // Create temporary copy and shift
    auto temp_steps = track.steps;
    for (int i = 0; i < MAX_STEPS; ++i) {
        track.steps[i] = temp_steps[(i + MAX_STEPS - steps) % MAX_STEPS];
    }
}

void StepSequencer::enqueueSequencerEvent(SequencerEvent::Type type, int track, int step) {
    SequencerEvent event(type, track, step);
    sequencer_subject_.enqueueEvent(event);
}

// ============================================================================
// Parameter Lock API Implementation
// ============================================================================

void StepSequencer::setParameterManager(std::shared_ptr<Parameters::ParameterManager> param_manager) {
    if (parameter_lock_manager_) {
        parameter_lock_manager_->setParameterManager(param_manager);
    }
}

void StepSequencer::setStepParameterLock(int track, int step, ParameterID param_id, float value) {
    if (track < 0 || track >= MAX_TRACKS || step < 0 || step >= MAX_STEPS) return;
    
    // Set lock in the step data
    tracks_[track].steps[step].lockParameter(param_id, value);
    
    // Also store in the parameter lock manager for global operations
    if (parameter_lock_manager_) {
        parameter_lock_manager_->setStepParameterLock(track, step, param_id, value);
    }
    
    #if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)
    std::string lock_data = "T" + std::to_string(track) + "S" + std::to_string(step) + 
                           " P" + std::to_string(static_cast<int>(param_id)) + ":" + std::to_string(value);
    TRACE_PARAMETER_EVENT("StepSequencer", "ParameterLockManager", "setParameterLock", lock_data.c_str());
    #endif
}

void StepSequencer::clearStepParameterLock(int track, int step, ParameterID param_id) {
    if (track < 0 || track >= MAX_TRACKS || step < 0 || step >= MAX_STEPS) return;
    
    // Clear from step data
    tracks_[track].steps[step].unlockParameter(param_id);
    
    // Clear from parameter lock manager
    if (parameter_lock_manager_) {
        parameter_lock_manager_->clearStepParameterLock(track, step, param_id);
    }
}

void StepSequencer::clearAllStepParameterLocks(int track, int step) {
    if (track < 0 || track >= MAX_TRACKS || step < 0 || step >= MAX_STEPS) return;
    
    // Clear from step data
    tracks_[track].steps[step].parameter_locks.clear();
    
    // Clear from parameter lock manager
    if (parameter_lock_manager_) {
        parameter_lock_manager_->clearStepLocks(track, step);
    }
}

bool StepSequencer::hasStepParameterLock(int track, int step, ParameterID param_id) const {
    if (track < 0 || track >= MAX_TRACKS || step < 0 || step >= MAX_STEPS) return false;
    
    return tracks_[track].steps[step].hasParameterLock(param_id);
}

float StepSequencer::getStepParameterLock(int track, int step, ParameterID param_id) const {
    if (track < 0 || track >= MAX_TRACKS || step < 0 || step >= MAX_STEPS) return 0.0f;
    
    return tracks_[track].steps[step].getLockedParameterValue(param_id);
}

std::vector<ParameterID> StepSequencer::getStepParameterLocks(int track, int step) const {
    std::vector<ParameterID> locked_params;
    
    if (track >= 0 && track < MAX_TRACKS && step >= 0 && step < MAX_STEPS) {
        const auto& step_data = tracks_[track].steps[step];
        for (const auto& [param_id, value] : step_data.parameter_locks) {
            locked_params.push_back(param_id);
        }
    }
    
    return locked_params;
}

void StepSequencer::copyStepParameterLocks(int src_track, int src_step, int dest_track, int dest_step) {
    if (src_track < 0 || src_track >= MAX_TRACKS || src_step < 0 || src_step >= MAX_STEPS ||
        dest_track < 0 || dest_track >= MAX_TRACKS || dest_step < 0 || dest_step >= MAX_STEPS) {
        return;
    }
    
    // Copy parameter locks in step data
    const auto& src_locks = tracks_[src_track].steps[src_step].parameter_locks;
    tracks_[dest_track].steps[dest_step].parameter_locks = src_locks;
    
    // Copy in parameter lock manager
    if (parameter_lock_manager_) {
        parameter_lock_manager_->copyStepLocks(src_track, src_step, dest_track, dest_step);
    }
}

size_t StepSequencer::getTotalParameterLocks() const {
    if (parameter_lock_manager_) {
        return parameter_lock_manager_->getTotalParameterLocks();
    }
    return 0;
}

void StepSequencer::clearAllParameterLocks() {
    // Clear all parameter locks from all steps
    for (int track = 0; track < MAX_TRACKS; ++track) {
        for (int step = 0; step < MAX_STEPS; ++step) {
            tracks_[track].steps[step].parameter_locks.clear();
        }
    }
    
    // Clear from parameter lock manager
    if (parameter_lock_manager_) {
        parameter_lock_manager_->clearAllParameterLocks();
    }
}

} // namespace MIDI
