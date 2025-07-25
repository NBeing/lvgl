#pragma once

#include "components/midi/StepSequencer.h"
#include "components/midi/ThreadedMidiClockManager.h"
#include <memory>

/**
 * @brief Simple UI controls for the step sequencer
 * 
 * This demonstrates how UI components can interact with the
 * threaded sequencer system safely.
 */
class SequencerControls {
public:
    explicit SequencerControls(std::shared_ptr<MIDI::StepSequencer> sequencer);
    
    // Transport controls
    void playSequencer();
    void stopSequencer();
    void resetSequencer();
    
    // Pattern controls
    void clearPattern();
    void randomizeTrack(int track_id, float probability = 0.5f);
    void toggleStep(int track, int step);
    
    // Track controls
    void muteTrack(int track_id);
    void soloTrack(int track_id);
    void setTrackTranspose(int track_id, int semitones);
    
    // Global controls
    void setBPM(float bpm);
    void setSwing(float swing);
    void setPatternLength(int steps);
    
    // Status queries
    bool isSequencerPlaying() const;
    int getCurrentStep() const;
    int getPatternLength() const;
    bool isTrackMuted(int track_id) const;
    bool isTrackSoloed(int track_id) const;
    
    // Demo patterns
    void loadDrumPattern();
    void loadBassPattern();
    void loadMelodyPattern();

private:
    std::shared_ptr<MIDI::StepSequencer> sequencer_;
};

// Implementation inline for simplicity
inline SequencerControls::SequencerControls(std::shared_ptr<MIDI::StepSequencer> sequencer)
    : sequencer_(sequencer) {
}

inline void SequencerControls::playSequencer() {
    // Use the threaded clock manager to start transport
    MIDI::ThreadedMidiClockManager::getInstance().play();
}

inline void SequencerControls::stopSequencer() {
    // Use the threaded clock manager to stop transport
    MIDI::ThreadedMidiClockManager::getInstance().stop_transport();
}

inline void SequencerControls::resetSequencer() {
    if (sequencer_) {
        sequencer_->reset();
    }
}

inline void SequencerControls::clearPattern() {
    if (sequencer_) {
        sequencer_->clearPattern();
    }
}

inline void SequencerControls::randomizeTrack(int track_id, float probability) {
    if (sequencer_) {
        sequencer_->randomizePattern(track_id, probability);
    }
}

inline void SequencerControls::toggleStep(int track, int step) {
    if (sequencer_) {
        sequencer_->toggleStep(track, step);
    }
}

inline void SequencerControls::muteTrack(int track_id) {
    if (sequencer_) {
        bool current_mute = sequencer_->getTrack(track_id).muted.load();
        sequencer_->muteTrack(track_id, !current_mute);
    }
}

inline void SequencerControls::soloTrack(int track_id) {
    if (sequencer_) {
        bool current_solo = sequencer_->getTrack(track_id).solo.load();
        sequencer_->soloTrack(track_id, !current_solo);
    }
}

inline void SequencerControls::setTrackTranspose(int track_id, int semitones) {
    if (sequencer_) {
        sequencer_->transposeTrack(track_id, semitones);
    }
}

inline void SequencerControls::setBPM(float bpm) {
    MIDI::ThreadedMidiClockManager::getInstance().setBPM(bpm);
}

inline void SequencerControls::setSwing(float swing) {
    if (sequencer_) {
        sequencer_->setSwing(swing);
    }
}

inline void SequencerControls::setPatternLength(int steps) {
    if (sequencer_) {
        sequencer_->setPatternLength(steps);
    }
}

inline bool SequencerControls::isSequencerPlaying() const {
    return sequencer_ ? sequencer_->isPlaying() : false;
}

inline int SequencerControls::getCurrentStep() const {
    return sequencer_ ? sequencer_->getCurrentStep() : 0;
}

inline int SequencerControls::getPatternLength() const {
    return sequencer_ ? sequencer_->getPatternLength() : 16;
}

inline bool SequencerControls::isTrackMuted(int track_id) const {
    return sequencer_ ? sequencer_->getTrack(track_id).muted.load() : false;
}

inline bool SequencerControls::isTrackSoloed(int track_id) const {
    return sequencer_ ? sequencer_->getTrack(track_id).solo.load() : false;
}

inline void SequencerControls::loadDrumPattern() {
    if (!sequencer_) return;
    
    // Clear existing pattern
    sequencer_->clearPattern();
    
    // Track 0: Kick (MIDI note 36)
    sequencer_->setStep(0, 0, MIDI::StepSequencer::Step(36, 127, true));  // Strong downbeat
    sequencer_->setStep(0, 6, MIDI::StepSequencer::Step(36, 90));         // Syncopated kick
    sequencer_->setStep(0, 8, MIDI::StepSequencer::Step(36, 110));
    sequencer_->setStep(0, 14, MIDI::StepSequencer::Step(36, 90));
    
    // Track 1: Snare (MIDI note 38)
    sequencer_->setStep(1, 4, MIDI::StepSequencer::Step(38, 120));
    sequencer_->setStep(1, 12, MIDI::StepSequencer::Step(38, 120));
    
    // Track 2: Closed Hi-hat (MIDI note 42)
    for (int i = 1; i < 16; i += 2) {
        sequencer_->setStep(2, i, MIDI::StepSequencer::Step(42, 70 + (i % 3) * 10));
    }
    
    // Track 3: Open Hi-hat (MIDI note 46)
    sequencer_->setStep(3, 7, MIDI::StepSequencer::Step(46, 90));
    sequencer_->setStep(3, 15, MIDI::StepSequencer::Step(46, 80));
    
    // Set all to drum channel
    for (int i = 0; i < 4; ++i) {
        sequencer_->getTrack(i).channel = 10;
    }
}

inline void SequencerControls::loadBassPattern() {
    if (!sequencer_) return;
    
    // Track 4: Bass line
    sequencer_->setStep(4, 0, MIDI::StepSequencer::Step(36, 110));   // C2
    sequencer_->setStep(4, 3, MIDI::StepSequencer::Step(36, 80));    // C2 ghost
    sequencer_->setStep(4, 6, MIDI::StepSequencer::Step(43, 100));   // G2
    sequencer_->setStep(4, 8, MIDI::StepSequencer::Step(36, 110));   // C2
    sequencer_->setStep(4, 10, MIDI::StepSequencer::Step(38, 90));   // D2
    sequencer_->setStep(4, 12, MIDI::StepSequencer::Step(40, 100));  // E2
    sequencer_->setStep(4, 14, MIDI::StepSequencer::Step(43, 95));   // G2
    
    sequencer_->getTrack(4).channel = 2; // Bass channel
}

inline void SequencerControls::loadMelodyPattern() {
    if (!sequencer_) return;
    
    // Track 5: Simple melody
    sequencer_->setStep(5, 0, MIDI::StepSequencer::Step(60, 90));    // C4
    sequencer_->setStep(5, 4, MIDI::StepSequencer::Step(64, 85));    // E4
    sequencer_->setStep(5, 8, MIDI::StepSequencer::Step(67, 90));    // G4
    sequencer_->setStep(5, 12, MIDI::StepSequencer::Step(64, 85));   // E4
    
    sequencer_->getTrack(5).channel = 3; // Melody channel
}
