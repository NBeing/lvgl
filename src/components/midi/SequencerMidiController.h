#pragma once

#include "components/parameter/ParameterManager.h"
#include "components/parameter/MidiParameterBridge.h"
#include "components/midi/StepSequencer.h"
#include "components/threading/ThreadSafeSubject.h"
#include <cstdint>
#include <unordered_map>
#include <memory>

namespace MIDI {

/**
 * @brief Provides comprehensive MIDI control for the step sequencer
 * 
 * This class extends the basic parameter control to include sequencer-specific
 * functions like transport control, step programming, track management, and
 * parameter lock operations. It creates a bridge between MIDI CC messages
 * and sequencer operations.
 * 
 * **MIDI CC Mapping Categories:**
 * - **Transport Control (CC 80-89):** Play, Stop, Tempo, Pattern selection
 * - **Step Programming (CC 90-119):** Step triggers, velocity, probability
 * - **Track Control (CC 120-127):** Track mute/solo, selection
 * - **Parameter Locks (CC 70-79):** Lock mode, value setting
 * 
 * **Thread Safety:**
 * - All MIDI processing happens on RT thread
 * - Sequencer operations are queued to appropriate threads
 * - UI updates triggered through observer pattern
 * 
 * **Integration:**
 * - Works alongside existing MidiParameterBridge for general parameters
 * - Extends ParameterManager with sequencer-specific parameter IDs
 * - Compatible with existing UI and parameter lock system
 */
class SequencerMidiController {
public:
    /**
     * @brief Sequencer-specific parameter IDs for MIDI control
     * 
     * These extend the base ParameterID enum with sequencer-specific functions
     * that can be controlled via MIDI CC messages.
     */
    enum class SequencerParameterID : uint32_t {
        // Transport Control (CC 80-89)
        TRANSPORT_PLAY = 10000,         ///< Play/Stop toggle (CC 80)
        TRANSPORT_STOP = 10001,         ///< Stop sequencer (CC 81)
        TRANSPORT_PAUSE = 10002,        ///< Pause/Continue (CC 82)
        TRANSPORT_BPM = 10003,          ///< Tempo control (CC 83)
        PATTERN_SELECT = 10004,         ///< Pattern selection (CC 84)
        PATTERN_LENGTH = 10005,         ///< Pattern length (CC 85)
        SWING_AMOUNT = 10006,           ///< Swing timing (CC 86)
        
        // Track Control (CC 120-127)
        TRACK_SELECT = 10010,           ///< Current track selection (CC 120)
        TRACK_MUTE = 10011,            ///< Mute current track (CC 121)
        TRACK_SOLO = 10012,            ///< Solo current track (CC 122)
        TRACK_TRANSPOSE = 10013,        ///< Transpose current track (CC 123)
        
        // Step Programming (CC 90-119) - Relative to current track
        STEP_SELECT = 10020,            ///< Current step selection (CC 90)
        STEP_TRIGGER_01 = 10021,        ///< Step 1 trigger (CC 91)
        STEP_TRIGGER_02 = 10022,        ///< Step 2 trigger (CC 92)
        STEP_TRIGGER_03 = 10023,        ///< Step 3 trigger (CC 93)
        STEP_TRIGGER_04 = 10024,        ///< Step 4 trigger (CC 94)
        STEP_TRIGGER_05 = 10025,        ///< Step 5 trigger (CC 95)
        STEP_TRIGGER_06 = 10026,        ///< Step 6 trigger (CC 96)
        STEP_TRIGGER_07 = 10027,        ///< Step 7 trigger (CC 97)
        STEP_TRIGGER_08 = 10028,        ///< Step 8 trigger (CC 98)
        STEP_TRIGGER_09 = 10029,        ///< Step 9 trigger (CC 99)
        STEP_TRIGGER_10 = 10030,        ///< Step 10 trigger (CC 100)
        STEP_TRIGGER_11 = 10031,        ///< Step 11 trigger (CC 101)
        STEP_TRIGGER_12 = 10032,        ///< Step 12 trigger (CC 102)
        STEP_TRIGGER_13 = 10033,        ///< Step 13 trigger (CC 103)
        STEP_TRIGGER_14 = 10034,        ///< Step 14 trigger (CC 104)
        STEP_TRIGGER_15 = 10035,        ///< Step 15 trigger (CC 105)
        STEP_TRIGGER_16 = 10036,        ///< Step 16 trigger (CC 106)
        
        STEP_VELOCITY = 10040,          ///< Current step velocity (CC 107)
        STEP_PROBABILITY = 10041,       ///< Current step probability (CC 108)
        STEP_MICRO_TIMING = 10042,      ///< Current step micro-timing (CC 109)
        STEP_NOTE = 10043,             ///< Current step note (CC 110)
        STEP_LENGTH = 10044,           ///< Current step length (CC 111)
        
        // Parameter Lock Control (CC 70-79)
        PARAM_LOCK_MODE = 10050,        ///< Enter/exit parameter lock mode (CC 70)
        PARAM_LOCK_PARAMETER = 10051,   ///< Select parameter to lock (CC 71)
        PARAM_LOCK_VALUE = 10052,       ///< Set parameter lock value (CC 72)
        PARAM_LOCK_CLEAR = 10053,       ///< Clear current parameter lock (CC 73)
        PARAM_LOCK_CLEAR_ALL = 10054,   ///< Clear all locks for current step (CC 74)
        PARAM_LOCK_COPY = 10055,        ///< Copy locks from current step (CC 75)
        PARAM_LOCK_PASTE = 10056,       ///< Paste locks to current step (CC 76)
        
        // Performance Controls (CC 60-69)
        FILL_MODE = 10060,             ///< Activate fill mode (CC 60)
        MUTE_ALL = 10061,              ///< Global mute toggle (CC 61)
        SOLO_CLEAR = 10062,            ///< Clear all solo states (CC 62)
        PATTERN_CHAIN = 10063,         ///< Chain to next pattern (CC 63)
        RANDOMIZE_TRACK = 10064,       ///< Randomize current track (CC 64)
        SHIFT_TRACK_LEFT = 10065,      ///< Shift track pattern left (CC 65)
        SHIFT_TRACK_RIGHT = 10066,     ///< Shift track pattern right (CC 66)
        CLEAR_TRACK = 10067,           ///< Clear current track (CC 67)
        COPY_TRACK = 10068,            ///< Copy current track (CC 68)
        PASTE_TRACK = 10069            ///< Paste to current track (CC 69)
    };
    
    /**
     * @brief MIDI control event for sequencer operations
     * 
     * Contains information about MIDI-initiated sequencer changes,
     * used for event tracing and debugging.
     */
    struct SequencerMidiEvent {
        SequencerParameterID parameter_id;  ///< Sequencer function being controlled
        float normalized_value;             ///< MIDI value converted to 0.0-1.0
        uint8_t midi_channel;              ///< Source MIDI channel
        uint8_t midi_cc;                   ///< Source MIDI CC number
        uint8_t raw_midi_value;            ///< Original MIDI value (0-127)
        uint32_t timestamp;                ///< Event timestamp
        
        SequencerMidiEvent(SequencerParameterID param, float value, uint8_t channel, 
                          uint8_t cc, uint8_t raw_value)
            : parameter_id(param), normalized_value(value), midi_channel(channel),
              midi_cc(cc), raw_midi_value(raw_value), timestamp(lv_tick_get()) {}
    };
    
    /**
     * @brief Get singleton instance of sequencer MIDI controller
     * @return Reference to singleton instance
     */
    static SequencerMidiController& getInstance();
    
    /**
     * @brief Initialize sequencer MIDI controller
     * @param sequencer Shared pointer to step sequencer
     * @param param_manager Shared pointer to parameter manager
     * @note Must be called before processing MIDI messages
     */
    void initialize(std::shared_ptr<StepSequencer> sequencer,
                   std::shared_ptr<Parameters::ParameterManager> param_manager);
    
    /**
     * @brief Shutdown and cleanup resources
     */
    void shutdown();
    
    // =============================================================================
    // MIDI PROCESSING (Called from RT Thread)
    // =============================================================================
    
    /**
     * @brief Process MIDI CC message for sequencer control
     * @param channel MIDI channel (0-15)
     * @param cc MIDI CC number (0-127)
     * @param value MIDI CC value (0-127)
     * @return true if CC was handled by sequencer controller
     * @note Called from RT thread, must be lock-free
     * @note Returns false if CC should be handled by general parameter system
     */
    bool processSequencerMidiCC(uint8_t channel, uint8_t cc, uint8_t value);
    
    /**
     * @brief Process MIDI Note On for step programming
     * @param channel MIDI channel (0-15)
     * @param note MIDI note number (0-127)
     * @param velocity MIDI velocity (0-127)
     * @return true if note was handled for step programming
     * @note Can be used for chromatic step programming
     */
    bool processSequencerNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    
    /**
     * @brief Process MIDI Note Off for step programming
     * @param channel MIDI channel (0-15)
     * @param note MIDI note number (0-127)
     * @return true if note was handled for step programming
     */
    bool processSequencerNoteOff(uint8_t channel, uint8_t note);
    
    // =============================================================================
    // MIDI MAPPING MANAGEMENT
    // =============================================================================
    
    /**
     * @brief Setup default MIDI CC mappings for sequencer control
     * @note Creates standard mapping for common MIDI controllers
     * @note Can be customized or overridden by user
     */
    void setupDefaultMidiMappings();
    
    /**
     * @brief Assign MIDI CC to sequencer parameter
     * @param channel MIDI channel (1-16)
     * @param cc MIDI CC number (0-127)
     * @param param_id Sequencer parameter to control
     * @note Thread-safe, can be called from UI thread
     */
    void assignSequencerMidiCC(uint8_t channel, uint8_t cc, SequencerParameterID param_id);
    
    /**
     * @brief Remove MIDI CC mapping
     * @param channel MIDI channel (1-16)
     * @param cc MIDI CC number (0-127)
     * @note Thread-safe
     */
    void removeSequencerMidiCC(uint8_t channel, uint8_t cc);
    
    /**
     * @brief Get sequencer parameter mapped to MIDI CC
     * @param channel MIDI channel (1-16)
     * @param cc MIDI CC number (0-127)
     * @return Sequencer parameter ID, or 0 if not mapped
     * @note Thread-safe query
     */
    SequencerParameterID getSequencerMidiMapping(uint8_t channel, uint8_t cc) const;
    
    // =============================================================================
    // SEQUENCER STATE MANAGEMENT
    // =============================================================================
    
    /**
     * @brief Set current track for step programming
     * @param track_id Track number (0-7)
     * @note Affects which track receives step programming MIDI
     */
    void setCurrentTrack(int track_id);
    
    /**
     * @brief Set current step for parameter editing
     * @param step_id Step number (0-15)
     * @note Affects which step receives parameter edits
     */
    void setCurrentStep(int step_id);
    
    /**
     * @brief Get current track for MIDI programming
     * @return Currently selected track (0-7)
     */
    int getCurrentTrack() const { return current_track_.load(); }
    
    /**
     * @brief Get current step for MIDI programming
     * @return Currently selected step (0-15)
     */
    int getCurrentStep() const { return current_step_.load(); }
    
    /**
     * @brief Check if parameter lock mode is active
     * @return true if in parameter lock editing mode
     */
    bool isParameterLockModeActive() const { return param_lock_mode_.load(); }
    
    // =============================================================================
    // STATISTICS AND DEBUGGING
    // =============================================================================
    
    /**
     * @brief MIDI control statistics
     */
    struct Statistics {
        std::atomic<uint64_t> transport_commands{0};      ///< Transport CC messages processed
        std::atomic<uint64_t> step_commands{0};           ///< Step programming CC messages
        std::atomic<uint64_t> track_commands{0};          ///< Track control CC messages
        std::atomic<uint64_t> param_lock_commands{0};     ///< Parameter lock CC messages
        std::atomic<uint64_t> performance_commands{0};    ///< Performance CC messages
        std::atomic<uint64_t> unmapped_ccs{0};           ///< Unhandled CC messages
        std::atomic<uint64_t> midi_learn_assignments{0};  ///< MIDI learn operations
    };
    
    /**
     * @brief Get MIDI control statistics
     * @return Reference to statistics structure
     */
    const Statistics& getStatistics() const { return stats_; }
    
    /**
     * @brief Print debugging information about sequencer MIDI control
     */
    void printSequencerMidiStatistics() const;
    
    /**
     * @brief Export current MIDI mappings as text
     * @return String description of all active mappings
     */
    std::string exportMidiMappings() const;

private:
    SequencerMidiController() = default;
    ~SequencerMidiController() = default;
    
    // =============================================================================
    // INTERNAL STATE
    // =============================================================================
    
    std::shared_ptr<StepSequencer> sequencer_;
    std::shared_ptr<Parameters::ParameterManager> param_manager_;
    std::atomic<bool> initialized_{false};
    
    // Current editing context
    std::atomic<int> current_track_{0};           ///< Currently selected track
    std::atomic<int> current_step_{0};            ///< Currently selected step
    std::atomic<bool> param_lock_mode_{false};    ///< Parameter lock editing mode
    std::atomic<uint32_t> selected_parameter_{0}; ///< Parameter selected for locking
    
    // MIDI mapping storage
    std::unordered_map<uint16_t, SequencerParameterID> sequencer_midi_mappings_;
    mutable std::mutex mappings_mutex_;
    
    // Parameter lock copy buffer
    std::unordered_map<Parameters::ParameterID, float> param_lock_clipboard_;
    
    // Statistics
    Statistics stats_;
    
    // =============================================================================
    // INTERNAL PROCESSING METHODS
    // =============================================================================
    
    /**
     * @brief Process transport control MIDI CC
     * @param param_id Transport parameter being controlled
     * @param normalized_value MIDI value as 0.0-1.0
     * @param raw_value Original MIDI value (0-127)
     */
    void processTransportControl(SequencerParameterID param_id, float normalized_value, uint8_t raw_value);
    
    /**
     * @brief Process track control MIDI CC
     * @param param_id Track parameter being controlled
     * @param normalized_value MIDI value as 0.0-1.0
     * @param raw_value Original MIDI value (0-127)
     */
    void processTrackControl(SequencerParameterID param_id, float normalized_value, uint8_t raw_value);
    
    /**
     * @brief Process step programming MIDI CC
     * @param param_id Step parameter being controlled
     * @param normalized_value MIDI value as 0.0-1.0
     * @param raw_value Original MIDI value (0-127)
     */
    void processStepControl(SequencerParameterID param_id, float normalized_value, uint8_t raw_value);
    
    /**
     * @brief Process parameter lock MIDI CC
     * @param param_id Parameter lock function being controlled
     * @param normalized_value MIDI value as 0.0-1.0
     * @param raw_value Original MIDI value (0-127)
     */
    void processParameterLockControl(SequencerParameterID param_id, float normalized_value, uint8_t raw_value);
    
    /**
     * @brief Process performance control MIDI CC
     * @param param_id Performance function being controlled
     * @param normalized_value MIDI value as 0.0-1.0
     * @param raw_value Original MIDI value (0-127)
     */
    void processPerformanceControl(SequencerParameterID param_id, float normalized_value, uint8_t raw_value);
    
    /**
     * @brief Convert MIDI CC key to mapping key
     * @param channel MIDI channel (1-16)
     * @param cc MIDI CC number (0-127)
     * @return 16-bit mapping key
     */
    uint16_t getMappingKey(uint8_t channel, uint8_t cc) const;
    
    /**
     * @brief Convert MIDI value to normalized float
     * @param midi_value MIDI value (0-127)
     * @return Normalized value (0.0-1.0)
     */
    float midiToNormalized(uint8_t midi_value) const;
    
    /**
     * @brief Check if MIDI value represents a trigger (>= 64)
     * @param midi_value MIDI value (0-127)
     * @return true if value represents a trigger/on state
     */
    bool isMidiTrigger(uint8_t midi_value) const;
    
    /**
     * @brief Get step index from sequencer parameter ID
     * @param param_id Step trigger parameter ID
     * @return Step index (0-15) or -1 if not a step parameter
     */
    int getStepFromParameterID(SequencerParameterID param_id) const;
    
    /**
     * @brief Log sequencer MIDI event for debugging
     * @param event Sequencer MIDI event to log
     */
    void logSequencerMidiEvent(const SequencerMidiEvent& event) const;
};

} // namespace MIDI
