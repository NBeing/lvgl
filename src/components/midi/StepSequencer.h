#pragma once

#include "components/midi/MidiEvents.h"
#include "components/threading/ThreadSafeSubject.h"
#include <array>
#include <atomic>
#include <vector>
#include <cstdint>

namespace MIDI {

/**
 * @brief Step sequencer that synchronizes with MIDI clock
 * 
 * Features:
 * - 16 steps per pattern
 * - Multiple tracks (up to 8)
 * - Per-step velocity and note settings
 * - Pattern chaining and loop points
 * - Thread-safe operation
 */
class StepSequencer : public TypedObserver<ClockEvent> {
public:
    static constexpr int MAX_STEPS = 16;
    static constexpr int MAX_TRACKS = 8;
    static constexpr int PPQN = 24; // MIDI clock pulses per quarter note
    
    struct Step {
        bool active = false;        // Is this step triggered?
        uint8_t note = 60;         // MIDI note number (C4 = 60)
        uint8_t velocity = 100;    // MIDI velocity (0-127)
        uint8_t length = 6;        // Note length in clock ticks (6 = 1/16th note)
        bool accent = false;       // Accent flag (increases velocity)
        
        Step() = default;
        Step(uint8_t n, uint8_t v = 100, bool a = false) 
            : active(true), note(n), velocity(v), accent(a) {}
    };
    
    struct Track {
        std::array<Step, MAX_STEPS> steps;
        uint8_t channel = 1;       // MIDI channel (1-16)
        std::atomic<bool> muted{false};
        std::atomic<bool> solo{false};
        std::atomic<int> transpose{0}; // Transpose in semitones
        
        Track() = default;
    };
    
    struct SequencerEvent {
        enum Type {
            NOTE_ON,
            NOTE_OFF,
            STEP_ADVANCE,
            PATTERN_COMPLETE
        };
        
        Type type;
        int track_id;
        int step_id;
        uint8_t note;
        uint8_t velocity;
        uint8_t channel;
        std::chrono::steady_clock::time_point timestamp;
        
        SequencerEvent(Type t, int track = -1, int step = -1) 
            : type(t), track_id(track), step_id(step), note(0), velocity(0), channel(1),
              timestamp(std::chrono::steady_clock::now()) {}
        
        // Default constructor for queue usage
        SequencerEvent() 
            : type(NOTE_ON), track_id(0), step_id(0), note(0), velocity(0), channel(1),
              timestamp(std::chrono::steady_clock::now()) {}
    };

public:
    StepSequencer();
    ~StepSequencer() = default;
    
    // Observer interface - receives clock events
    void onEvent(const ClockEvent& event) override;
    
    // Observer management for sequencer events
    void addSequencerObserver(TypedObserver<SequencerEvent>* observer);
    void removeSequencerObserver(TypedObserver<SequencerEvent>* observer);
    void processSequencerEvents(); // Call from UI thread
    
    // Pattern control
    void play();
    void stop();
    void reset();
    bool isPlaying() const { return playing_.load(); }
    
    // Step programming
    void setStep(int track, int step, const Step& step_data);
    void clearStep(int track, int step);
    void toggleStep(int track, int step);
    const Step& getStep(int track, int step) const;
    
    // Track control
    Track& getTrack(int track_id) { return tracks_[track_id]; }
    const Track& getTrack(int track_id) const { return tracks_[track_id]; }
    void muteTrack(int track_id, bool mute = true);
    void soloTrack(int track_id, bool solo = true);
    void transposeTrack(int track_id, int semitones);
    
    // Pattern control
    void setPatternLength(int steps);
    int getPatternLength() const { return pattern_length_.load(); }
    void setSwing(float swing_amount); // 0.0 = straight, 1.0 = maximum swing
    float getSwing() const { return swing_amount_.load(); }
    
    // Current position
    int getCurrentStep() const { return current_step_.load(); }
    int getCurrentTick() const { return current_tick_.load(); }
    
    // Pattern data
    void clearPattern();
    void randomizePattern(int track_id, float probability = 0.5f);
    void shiftPattern(int track_id, int steps); // Rotate pattern left/right
    
private:
    // Core sequencer state
    std::atomic<bool> playing_{false};
    std::atomic<int> current_step_{0};
    std::atomic<int> current_tick_{0};
    std::atomic<int> pattern_length_{16};
    std::atomic<float> swing_amount_{0.0f};
    
    // Track data
    std::array<Track, MAX_TRACKS> tracks_;
    
    // Event management
    ThreadSafeSubject<SequencerEvent> sequencer_subject_;
    
    // Active notes tracking (for note-off events)
    struct ActiveNote {
        uint8_t note;
        uint8_t channel;
        int end_tick; // When to send note-off
        
        ActiveNote(uint8_t n, uint8_t c, int end) 
            : note(n), channel(c), end_tick(end) {}
    };
    std::vector<ActiveNote> active_notes_;
    
    // Clock processing
    void processClockTick(int tick_count);
    
    // Step timing with swing
    int getStepTick(int step) const;
    bool isStepTime(int tick) const;
    
    // Event generation
    void processStepTriggers(int step);
    void processActiveNotes(int current_tick);
    void enqueueSequencerEvent(SequencerEvent::Type type, int track = -1, int step = -1);
    
    // Note management
    void triggerNote(int track_id, int step_id, const Step& step);
    void releaseNote(uint8_t note, uint8_t channel);
    void releaseAllNotes();
    
    // Swing calculation
    int applySwing(int tick, int step) const;
};

} // namespace MIDI
