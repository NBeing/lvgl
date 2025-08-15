#pragma once

#include "ParameterChangeEvent.h"
#include "ParameterRegistry.h"
#include "ParameterObserver.h"
#include "components/threading/LockFreeQueue.h"
#include "components/threading/ThreadSafeSubject.h"
#include <vector>
#include <unordered_map>
#include <atomic>
#include <memory>

namespace Parameters {

/**
 * @brief Central parameter manager and event dispatcher for real-time audio applications
 * 
 * This is the heart of the unified parameter system. It receives parameter change events 
 * from any source (touch UI, MIDI controllers, automation, sequencer parameter locks) and 
 * routes them to the appropriate processors and observers based on parameter metadata.
 * 
 * **Thread Safety:**
 * - RT-safe parameter updates via lock-free queues
 * - Atomic parameter value storage for zero-copy reads
 * - Separate RT and UI observer notification paths
 * 
 * **Key Features:**
 * - MIDI learn functionality for hardware controller mapping
 * - Automation recording and playback
 * - Preset capture and restoration
 * - Performance statistics and monitoring
 * - Parameter lock support for step sequencers
 * 
 * **Dependencies:**
 * - `ParameterRegistry`: Parameter metadata and validation
 * - `ParameterObserver`: Observer pattern for parameter changes
 * - `LockFreeQueue`: RT-safe event queuing
 * - `ThreadSafeSubject`: UI thread event distribution
 * 
 * **Usage Example:**
 * ```cpp
 * auto& mgr = ParameterManager::getInstance();
 * mgr.setParameter(ParameterID::FILTER_CUTOFF, 0.75f, ParameterSource::MIDI);
 * mgr.addObserver(myAudioEngineObserver);
 * ```
 * 
 * @see ParameterObserver For parameter change notification
 * @see ParameterRegistry For parameter definitions and metadata
 * @see ParameterChangeEvent For event structure details
 */
class ParameterManager {
public:
    /**
     * @brief Get the singleton instance of ParameterManager
     * @return Reference to the global parameter manager instance
     * @note Thread-safe singleton implementation
     */
    static ParameterManager& getInstance();
    
    /**
     * @brief Initialize the parameter system
     * @note Must be called before using any parameter functionality
     * @note Should be called from the main thread during application startup
     */
    void initialize();
    
    /**
     * @brief Shutdown the parameter system and cleanup resources
     * @note Stops all processing threads and clears observers
     * @note Should be called during application shutdown
     */
    /**
     * @brief Shutdown the parameter system and cleanup resources
     * @note Stops all processing threads and clears observers
     * @note Should be called during application shutdown
     */
    void shutdown();
    
    // =============================================================================
    // OBSERVER MANAGEMENT
    // =============================================================================
    
    /**
     * @brief Add a parameter observer for change notifications
     * @param observer Shared pointer to observer (RT or UI type automatically detected)
     * @note RT observers receive notifications via lock-free queue
     * @note UI observers receive notifications via thread-safe subject
     * @see RTParameterObserver For real-time audio thread observers
     * @see UIParameterObserver For user interface thread observers
     */
    void addObserver(std::shared_ptr<ParameterObserver> observer);
    
    /**
     * @brief Remove a parameter observer
     * @param observer Observer to remove (must match exactly)
     * @note Safe to call from any thread
     */
    void removeObserver(std::shared_ptr<ParameterObserver> observer);
    
    // =============================================================================
    // PARAMETER CHANGE PROCESSING
    // =============================================================================
    
    /**
     * @brief Process a parameter change event (main entry point)
     * @param event Complete parameter change event with source, timing, etc.
     * @note Thread-safe, can be called from any thread
     * @note Events are routed to appropriate queues based on parameter metadata
     */
    /**
     * @brief Process a parameter change event (main entry point)
     * @param event Complete parameter change event with source, timing, etc.
     * @note Thread-safe, can be called from any thread
     * @note Events are routed to appropriate queues based on parameter metadata
     */
    void processParameterChange(const ParameterChangeEvent& event);
    
    // =============================================================================
    // DIRECT PARAMETER SETTING
    // =============================================================================
    
    /**
     * @brief Set parameter value using normalized range (0.0-1.0)
     * @param id Parameter identifier from ParameterRegistry
     * @param normalized_value Value in normalized range (0.0 = min, 1.0 = max)
     * @param source Source of the parameter change (UI, MIDI, automation, etc.)
     * @note More efficient than processParameterChange for simple value updates
     * @note Thread-safe, can be called from any thread
     */
    void setParameter(ParameterID id, float normalized_value, ParameterSource source = ParameterSource::INTERNAL);
    
    /**
     * @brief Set parameter value using real-world units
     * @param id Parameter identifier from ParameterRegistry
     * @param real_value Value in parameter's native units (Hz, dB, etc.)
     * @param source Source of the parameter change
     * @note Automatically converts to normalized range using parameter metadata
     * @note Thread-safe, can be called from any thread
     */
    void setParameterFromReal(ParameterID id, float real_value, ParameterSource source = ParameterSource::INTERNAL);
    
    // =============================================================================
    // PARAMETER VALUE QUERIES
    // =============================================================================
    
    /**
     * @brief Get current parameter value in normalized range (0.0-1.0)
     * @param id Parameter identifier
     * @return Current value in normalized range
     * @note Thread-safe, zero-copy read from atomic storage
     * @note Returns 0.0 if parameter doesn't exist
     */
    float getParameterNormalized(ParameterID id) const;
    
    /**
     * @brief Get current parameter value in real-world units
     * @param id Parameter identifier
     * @return Current value in parameter's native units (Hz, dB, etc.)
     * @note Converts from normalized using parameter metadata
     * @note Thread-safe, zero-copy read from atomic storage
     */
    float getParameterReal(ParameterID id) const;
    
    /**
     * @brief Get formatted display string for parameter value
     * @param id Parameter identifier
     * @return Human-readable string with units (e.g., "440.0 Hz", "12.5 dB")
     * @note Uses parameter metadata for formatting and units
     * @note Thread-safe
     */
    /**
     * @brief Get formatted display string for parameter value
     * @param id Parameter identifier
     * @return Human-readable string with units (e.g., "440.0 Hz", "12.5 dB")
     * @note Uses parameter metadata for formatting and units
     * @note Thread-safe
     */
    std::string getParameterDisplayValue(ParameterID id) const;
    
    // =============================================================================
    // MIDI LEARN SYSTEM
    // =============================================================================
    
    /**
     * @brief Start MIDI learn mode for a parameter
     * @param id Parameter to learn MIDI CC mapping for
     * @note Next MIDI CC message received will be mapped to this parameter
     * @note Only one parameter can be in learn mode at a time
     * @see stopMidiLearn To cancel learn mode
     */
    void startMidiLearn(ParameterID id);
    
    /**
     * @brief Stop MIDI learn mode
     * @note Cancels any active MIDI learn session
     * @note Safe to call even if not in learn mode
     */
    void stopMidiLearn();
    
    /**
     * @brief Check if currently in MIDI learn mode
     * @return true if a parameter is waiting for MIDI CC assignment
     * @note Thread-safe atomic read
     */
    bool isMidiLearning() const { return midi_learn_parameter_id_.load() != 0; }
    
    /**
     * @brief Get the parameter currently in MIDI learn mode
     * @return Parameter ID waiting for MIDI assignment, or 0 if none
     * @note Thread-safe atomic read
     */
    ParameterID getMidiLearnParameter() const { return midi_learn_parameter_id_.load(); }
    
    // =============================================================================
    // MIDI MAPPING MANAGEMENT
    // =============================================================================
    
    /**
     * @brief Manually assign a MIDI CC to a parameter
     * @param parameter_id Target parameter for the mapping
     * @param channel MIDI channel (1-16)
     * @param cc MIDI CC number (0-127)
     * @note Overwrites any existing mapping for this MIDI CC
     * @note Thread-safe
     */
    void assignMidiCC(ParameterID parameter_id, uint8_t channel, uint8_t cc);
    
    /**
     * @brief Remove MIDI CC mapping
     * @param channel MIDI channel (1-16)
     * @param cc MIDI CC number (0-127)
     * @note Safe to call even if no mapping exists
     * @note Thread-safe
     */
    void removeMidiCC(uint8_t channel, uint8_t cc);
    
    /**
     * @brief Get parameter mapped to a specific MIDI CC
     * @param channel MIDI channel (1-16)
     * @param cc MIDI CC number (0-127)
     * @return Parameter ID mapped to this CC, or 0 if none
     * @note Thread-safe
     */
    /**
     * @brief Get parameter mapped to a specific MIDI CC
     * @param channel MIDI channel (1-16)
     * @param cc MIDI CC number (0-127)
     * @return Parameter ID mapped to this CC, or 0 if none
     * @note Thread-safe
     */
    ParameterID getMidiMapping(uint8_t channel, uint8_t cc) const;
    
    // =============================================================================
    // AUTOMATION SUPPORT (For Sequencer Parameter Locks)
    // =============================================================================
    
    /**
     * @brief Record a parameter change for automation playback
     * @param event Parameter change event to record
     * @note Used by sequencer parameter locks and automation recording
     * @note Events are timestamped for precise playback timing
     * @see playbackAutomation For playing back recorded automation
     */
    void recordParameterChange(const ParameterChangeEvent& event);
    
    /**
     * @brief Play back recorded automation events
     * @param events Vector of parameter change events to play back
     * @note Events should be sorted by timestamp for proper playback
     * @note Used by sequencer for parameter lock playback
     * @note Thread-safe, can be called from sequencer thread
     */
    void playbackAutomation(const std::vector<ParameterChangeEvent>& events);
    
    // =============================================================================
    // PRESET SUPPORT
    // =============================================================================
    
    /**
     * @brief Capture current state of all parameters
     * @return Map of parameter IDs to their current normalized values
     * @note Creates a snapshot that can be saved as a preset
     * @note Thread-safe, reads from atomic parameter storage
     * @see loadPresetState To restore a captured state
     */
    std::unordered_map<ParameterID, float> captureCurrentState() const;
    
    /**
     * @brief Load parameter values from a preset
     * @param state Map of parameter IDs to normalized values
     * @note Updates all parameters specified in the state map
     * @note Thread-safe, sends parameter change events
     * @note Source will be marked as ParameterSource::PRESET
     */
    void loadPresetState(const std::unordered_map<ParameterID, float>& state);
    
    // =============================================================================
    // REAL-TIME THREAD PROCESSING
    // =============================================================================
    
    /**
     * @brief Process pending RT events (call from real-time audio thread)
     * @note Must be called regularly from the audio thread (e.g., audio callback)
     * @note Processes events from the RT-safe lock-free queue
     * @note Never blocks or allocates memory
     * @warning Only call from the designated real-time thread
     */
    void processRTEvents();
    
    /**
     * @brief Process pending UI events (call from UI thread)
     * @note Must be called regularly from the UI thread (e.g., LVGL timer)
     * @note Processes events from the thread-safe UI event queue
     * @note May allocate memory for UI updates
     * @warning Only call from the main UI thread
     */
    /**
     * @brief Process pending UI events (call from UI thread)
     * @note Must be called regularly from the UI thread (e.g., LVGL timer)
     * @note Processes events from the thread-safe UI event queue
     * @note May allocate memory for UI updates
     * @warning Only call from the main UI thread
     */
    void processUIEvents();
    
    // =============================================================================
    // PERFORMANCE STATISTICS
    // =============================================================================
    
    /**
     * @brief Performance and usage statistics for monitoring
     * @note All counters are atomic for thread-safe access
     * @note Useful for debugging and performance optimization
     */
    struct Statistics {
        std::atomic<uint64_t> events_processed{0};      ///< Total events processed
        std::atomic<uint64_t> rt_events_processed{0};   ///< RT-thread events processed
        std::atomic<uint64_t> ui_events_processed{0};   ///< UI-thread events processed
        std::atomic<uint64_t> events_dropped{0};        ///< Events dropped due to full queues
        std::atomic<uint32_t> current_rt_queue_size{0}; ///< Current RT queue occupancy
        std::atomic<uint32_t> current_ui_queue_size{0}; ///< Current UI queue occupancy
        std::atomic<uint32_t> max_rt_queue_size{0};     ///< Maximum RT queue size seen
        std::atomic<uint32_t> max_ui_queue_size{0};     ///< Maximum UI queue size seen
    };
    
    /**
     * @brief Get current performance statistics
     * @return Reference to statistics structure
     * @note Thread-safe, all fields are atomic
     * @note Use for performance monitoring and debugging
     */
    /**
     * @brief Get current performance statistics
     * @return Reference to statistics structure
     * @note Thread-safe, all fields are atomic
     * @note Use for performance monitoring and debugging
     */
    const Statistics& getStatistics() const { return stats_; }
    
private:
    /**
     * @brief Private constructor for singleton pattern
     * @note Use getInstance() to access the parameter manager
     */
    ParameterManager() = default;
    
    // =============================================================================
    // INTERNAL EVENT ROUTING
    // =============================================================================
    
    /**
     * @brief Route parameter event to appropriate queue based on metadata
     * @param event Parameter change event to route
     * @note Called internally by processParameterChange()
     */
    void routeParameterEvent(const ParameterChangeEvent& event);
    
    /**
     * @brief Notify all real-time observers of parameter change
     * @param event Parameter change event
     * @note Uses lock-free queue for RT-safe notification
     */
    void notifyRTObservers(const ParameterChangeEvent& event);
    
    /**
     * @brief Notify all UI observers of parameter change
     * @param event Parameter change event
     * @note Uses thread-safe subject for UI notification
     */
    void notifyUIObservers(const ParameterChangeEvent& event);
    
    /**
     * @brief Handle incoming MIDI CC events and map to parameters
     * @param event MIDI CC event to process
     * @note Looks up parameter mapping and forwards to observers
     */
    void handleMidiCCEvent(const ParameterChangeEvent& event);
    
    // =============================================================================
    // OBSERVER STORAGE
    // =============================================================================
    
    /**
     * @brief Real-time observers (audio engine, DSP components)
     * @note Receive notifications via lock-free queue
     */
    std::vector<std::shared_ptr<RTParameterObserver>> rt_observers_;
    
    /**
     * @brief UI observers (controls, displays, visualizers)
     * @note Receive notifications via thread-safe subject
     */
    /**
     * @brief UI observers (controls, displays, visualizers)
     * @note Receive notifications via thread-safe subject
     */
    std::vector<std::shared_ptr<UIParameterObserver>> ui_observers_;
    
    // =============================================================================
    // EVENT QUEUES AND THREADING
    // =============================================================================
    
    /**
     * @brief Maximum number of events in RT queue before dropping
     * @note Sized for high-throughput MIDI and sequencer automation
     */
    static constexpr size_t MAX_RT_EVENTS = 1024;
    
    /**
     * @brief Maximum number of events in UI queue before dropping
     * @note Larger than RT queue since UI updates are less critical
     */
    static constexpr size_t MAX_UI_EVENTS = 2048;
    
    /**
     * @brief Lock-free queue for real-time parameter events
     * @note Used for audio thread communication - never blocks
     */
    LockFreeQueue<ParameterChangeEvent, MAX_RT_EVENTS> rt_event_queue_;
    
    /**
     * @brief Thread-safe subject for UI parameter events
     * @note Used for UI thread communication - may block briefly
     */
    ThreadSafeSubject<ParameterChangeEvent> ui_event_subject_;
    
    // =============================================================================
    // PARAMETER VALUE STORAGE
    // =============================================================================
    
    /**
     * @brief Current parameter values storage
     * @note Atomic values for thread-safe zero-copy reads
     * @note Key: ParameterID, Value: Normalized parameter value (0.0-1.0)
     */
    mutable std::unordered_map<ParameterID, std::atomic<float>> parameter_values_;
    
    // =============================================================================
    // MIDI LEARN AND MAPPING
    // =============================================================================
    
    /**
     * @brief Parameter currently in MIDI learn mode
     * @note Atomic for thread-safe access, 0 = no learning active
     */
    std::atomic<ParameterID> midi_learn_parameter_id_{0};
    
    /**
     * @brief MIDI CC to parameter mappings
     * @note Key: (channel << 8 | cc), Value: parameter_id
     * @note Allows mapping any CC on any channel to any parameter
     */
    std::unordered_map<uint16_t, ParameterID> midi_cc_mappings_;
    
    // =============================================================================
    // SYSTEM STATE
    // =============================================================================
    
    /**
     * @brief Performance and usage statistics
     * @note All fields are atomic for thread-safe monitoring
     */
    Statistics stats_;
    
    /**
     * @brief System initialization state
     * @note Ensures proper startup/shutdown sequencing
     */
    std::atomic<bool> initialized_{false};
};

} // namespace Parameters
