#pragma once

#include "components/threading/RTSafeEventDistributor.h"
#include <unordered_map>
#include <memory>
#include <atomic>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <set>

// Forward declarations for dependencies
class ParameterManager;
class MidiHandler;

// Forward declaration for test mocks (will be available when testing)
class MockParameterManager;
class MockMidiHandler;

namespace RTSafe {

/**
 * @brief Bidirectional MIDI-Parameter Bridge
 * 
 * Provides RT-safe bidirectional synchronization between:
 * - UI Parameter Changes --> MIDI CC Output
 * - MIDI CC Input --> Parameter Updates --> UI Updates
 * 
 * Key Features:
 * - RT-safe operation (no blocking in audio thread)
 * - Feedback loop prevention
 * - Configurable parameter ↔ MIDI CC mappings
 * - Thread-safe parameter updates
 * - Statistics and monitoring
 */
class BidirectionalParameterMidiBridge : public RTObserver, public UIObserver {
public:
    /**
     * @brief MIDI CC to Parameter mapping configuration
     */
    struct ParameterMapping {
        uint32_t parameter_id;          // Internal parameter ID
        uint8_t midi_channel;           // MIDI channel (0-15)
        uint8_t midi_cc_number;         // MIDI CC number (0-127)
        uint8_t min_midi_value;         // Minimum MIDI value (usually 0)
        uint8_t max_midi_value;         // Maximum MIDI value (usually 127)
        float min_parameter_value;      // Minimum parameter value
        float max_parameter_value;      // Maximum parameter value
        bool bidirectional;             // Enable both directions?
        bool enabled;                   // Is this mapping active?
        
        ParameterMapping() 
            : parameter_id(0), midi_channel(0), midi_cc_number(0)
            , min_midi_value(0), max_midi_value(127)
            , min_parameter_value(0.0f), max_parameter_value(1.0f)
            , bidirectional(true), enabled(true) {}
    };
    
    /**
     * @brief Bridge statistics for monitoring (non-atomic for return)
     */
    struct BridgeStatistics {
        uint64_t midi_to_param_events{0};     // MIDI --> Parameter
        uint64_t param_to_midi_events{0};     // Parameter --> MIDI
        uint64_t feedback_loops_prevented{0}; // Feedback prevention
        uint64_t mapping_errors{0};           // Invalid mappings
        uint64_t midi_send_failures{0};       // MIDI output failures
        uint32_t max_processing_time_us{0};   // Max RT processing time
        uint32_t last_processing_time_us{0};  // Last RT processing time
    };

    /**
     * @brief Constructor
     */
    BidirectionalParameterMidiBridge(
        RTSafeEventDistributor* event_distributor,
        ParameterManager* parameter_manager,
        MidiHandler* midi_handler)
        : event_distributor_(event_distributor)
        , parameter_manager_(parameter_manager)
        , midi_handler_(midi_handler) {
        
        if (!event_distributor_ || !parameter_manager_ || !midi_handler_) {
            throw std::invalid_argument("BidirectionalParameterMidiBridge: null dependencies");
        }
    }
    
    /**
     * @brief Initialize the bridge
     */
    bool initialize() {
        if (!event_distributor_ || !parameter_manager_ || !midi_handler_) {
            return false;
        }
        
        // Register as both RT and UI observer
        event_distributor_->addRTObserver(this);
        event_distributor_->addUIObserver(this);
        
        // Set up default mappings for common synthesizer parameters
        setupDefaultMappings();
        
        return true;
    }
    
    /**
     * @brief Shutdown the bridge
     */
    void shutdown() {
        enabled_ = false;
        // Note: Observer removal would need to be added to RTSafeEventDistributor
    }
    
    /**
     * @brief Add parameter ↔ MIDI CC mapping
     */
    bool addParameterMapping(const ParameterMapping& mapping) {
        if (!mapping.enabled || mapping.midi_cc_number > 127) {
            return false;
        }
        
        // Add bidirectional mappings
        cc_to_param_mappings_[mapping.midi_cc_number] = mapping;
        param_to_cc_mappings_[mapping.parameter_id] = mapping;
        
        return true;
    }
    
    /**
     * @brief Remove parameter mapping
     */
    void removeParameterMapping(uint32_t parameter_id) {
        auto it = param_to_cc_mappings_.find(parameter_id);
        if (it != param_to_cc_mappings_.end()) {
            uint8_t cc_number = it->second.midi_cc_number;
            cc_to_param_mappings_.erase(cc_number);
            param_to_cc_mappings_.erase(parameter_id);
        }
    }
    
    /**
     * @brief RT Observer interface - handles RT events
     */
    void handleRTEvent(const RTEvent& event) override {
        if (!enabled_) return;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        switch (event.type) {
            case EventType::CONTROL_CHANGE:
                handleMidiCCInput(event);
                break;
                
            case EventType::PARAMETER_CHANGE:
                handleParameterChange(event);
                break;
                
            default:
                break;
        }
        
        // Track RT timing
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time);
        
        uint32_t duration_us = static_cast<uint32_t>(duration.count());
        last_processing_time_us_ = duration_us;
        
        // Update max processing time atomically
        uint32_t current_max = max_processing_time_us_.load();
        while (duration_us > current_max && 
               !max_processing_time_us_.compare_exchange_weak(current_max, duration_us)) {
            // Retry if another thread updated max_processing_time_us
        }
    }
    
    /**
     * @brief UI Observer interface - handles UI events
     */
    void handleUIEvent(const RTEvent& event) override {
        if (!enabled_) return;
        
        // UI thread can handle more complex operations
        switch (event.type) {
            case EventType::CONTROL_CHANGE:
                handleMidiCCInputUI(event);
                break;
                
            case EventType::PARAMETER_CHANGE:
                handleParameterChangeUI(event);
                break;
                
            default:
                break;
        }
    }
    
    /**
     * @brief Get observer priority (higher = processed first)
     */
    int getPriority() const override {
        return 5; // Medium priority
    }
    
    /**
     * @brief Get bridge statistics
     */
    BridgeStatistics getStatistics() const {
        BridgeStatistics stats;
        stats.midi_to_param_events = midi_to_param_events_.load();
        stats.param_to_midi_events = param_to_midi_events_.load();
        stats.feedback_loops_prevented = feedback_loops_prevented_.load();
        stats.mapping_errors = mapping_errors_.load();
        stats.midi_send_failures = midi_send_failures_.load();
        stats.max_processing_time_us = max_processing_time_us_.load();
        stats.last_processing_time_us = last_processing_time_us_.load();
        return stats;
    }
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics() {
        midi_to_param_events_ = 0;
        param_to_midi_events_ = 0;
        feedback_loops_prevented_ = 0;
        mapping_errors_ = 0;
        midi_send_failures_ = 0;
        max_processing_time_us_ = 0;
        last_processing_time_us_ = 0;
    }
    
    /**
     * @brief Enable/disable the bridge
     */
    void setEnabled(bool enabled) {
        enabled_ = enabled;
    }
    
    bool isEnabled() const {
        return enabled_;
    }
    
    /**
     * @brief Enable/disable feedback loop prevention
     */
    void setFeedbackLoopPrevention(bool prevent) {
        prevent_feedback_loops_ = prevent;
    }
    
    bool isFeedbackLoopPreventionEnabled() const {
        return prevent_feedback_loops_;
    }

protected:
    // Core components (accessible to derived classes for testing)
    RTSafeEventDistributor* event_distributor_;
    ParameterManager* parameter_manager_;
    MidiHandler* midi_handler_;
    
    /**
     * @brief RT-safe wrapper methods for parameter and MIDI operations
     * These can be overridden for testing or different implementations
     */
    virtual void setParameterValueRT(uint32_t parameter_id, float value) {
        // Default implementation - should be overridden in tests
    }
    
    virtual float getParameterValueRT(uint32_t parameter_id) {
        // Default implementation - should be overridden in tests
        return 0.0f;
    }
    
    virtual bool sendControlChangeRT(uint8_t channel, uint8_t cc_number, uint8_t value) {
        // Default implementation - should be overridden in tests
        return false;
    }

private:
    // Parameter mappings (CC number --> mapping)
    std::unordered_map<uint8_t, ParameterMapping> cc_to_param_mappings_;
    std::unordered_map<uint32_t, ParameterMapping> param_to_cc_mappings_;
    
    // Feedback loop prevention
    std::atomic<bool> processing_midi_input_{false};
    std::atomic<bool> processing_param_change_{false};
    
    // Statistics (atomic for thread safety)
    std::atomic<uint64_t> midi_to_param_events_{0};
    std::atomic<uint64_t> param_to_midi_events_{0};
    std::atomic<uint64_t> feedback_loops_prevented_{0};
    std::atomic<uint64_t> mapping_errors_{0};
    std::atomic<uint64_t> midi_send_failures_{0};
    std::atomic<uint32_t> max_processing_time_us_{0};
    std::atomic<uint32_t> last_processing_time_us_{0};
    
    // Configuration
    std::atomic<bool> enabled_{true};
    std::atomic<bool> prevent_feedback_loops_{true};
    
    /**
     * @brief Handle MIDI CC input in RT thread
     */
    void handleMidiCCInput(const RTEvent& event) {
        // Check for feedback loop
        if (prevent_feedback_loops_ && processing_param_change_.load()) {
            feedback_loops_prevented_++;
            return;
        }
        
        // Set feedback prevention flag
        processing_midi_input_ = true;
        
        // Find parameter mapping
        auto it = cc_to_param_mappings_.find(event.data1); // data1 = CC number
        if (it == cc_to_param_mappings_.end()) {
            mapping_errors_++;
            processing_midi_input_ = false;
            return;
        }
        
        const ParameterMapping& mapping = it->second;
        if (!mapping.enabled || !mapping.bidirectional) {
            processing_midi_input_ = false;
            return;
        }
        
        // Convert MIDI value to parameter value
        float param_value = midiValueToParameterValue(event.data2, mapping);
        
        // Update parameter (RT-safe)
        if (parameter_manager_) {
            setParameterValueRT(mapping.parameter_id, param_value);
            midi_to_param_events_++;
        }
        
        processing_midi_input_ = false;
    }
    
    /**
     * @brief Handle parameter change in RT thread
     */
    void handleParameterChange(const RTEvent& event) {
        // Check for feedback loop
        if (prevent_feedback_loops_ && processing_midi_input_.load()) {
            feedback_loops_prevented_++;
            return;
        }
        
        // Set feedback prevention flag
        processing_param_change_ = true;
        
        // Find MIDI mapping
        uint32_t param_id = (static_cast<uint32_t>(event.data1) << 8) | event.data2;
        auto it = param_to_cc_mappings_.find(param_id);
        if (it == param_to_cc_mappings_.end()) {
            processing_param_change_ = false;
            return;
        }
        
        const ParameterMapping& mapping = it->second;
        if (!mapping.enabled || !mapping.bidirectional) {
            processing_param_change_ = false;
            return;
        }
        
        // Get current parameter value
        if (parameter_manager_) {
            float param_value = getParameterValueRT(mapping.parameter_id);
            
            // Convert to MIDI value
            uint8_t midi_value = parameterValueToMidiValue(param_value, mapping);
            
            // Send MIDI CC (RT-safe)
            if (midi_handler_) {
                bool success = sendControlChangeRT(
                    mapping.midi_channel, 
                    mapping.midi_cc_number, 
                    midi_value
                );
                
                if (success) {
                    param_to_midi_events_++;
                } else {
                    midi_send_failures_++;
                }
            }
        }
        
        processing_param_change_ = false;
    }
    
    /**
     * @brief Handle MIDI CC input in UI thread
     */
    void handleMidiCCInputUI(const RTEvent& event) {
        // UI thread can update displays, save presets, etc.
        // This is called after the RT processing is complete
    }
    
    /**
     * @brief Handle parameter change in UI thread
     */
    void handleParameterChangeUI(const RTEvent& event) {
        // UI thread can update displays, save presets, etc.
        // This is called after the RT processing is complete
    }
    
    /**
     * @brief Convert MIDI value (0-127) to parameter value
     */
    float midiValueToParameterValue(uint8_t midi_value, const ParameterMapping& mapping) const {
        // Clamp MIDI value to mapping range
        float clamped_midi = std::max(static_cast<float>(mapping.min_midi_value),
                                    std::min(static_cast<float>(mapping.max_midi_value),
                                           static_cast<float>(midi_value)));
        
        // Normalize to 0-1
        float normalized = (clamped_midi - mapping.min_midi_value) / 
                          (mapping.max_midi_value - mapping.min_midi_value);
        
        // Scale to parameter range
        return mapping.min_parameter_value + 
               (normalized * (mapping.max_parameter_value - mapping.min_parameter_value));
    }
    
    /**
     * @brief Convert parameter value to MIDI value (0-127)
     */
    uint8_t parameterValueToMidiValue(float param_value, const ParameterMapping& mapping) const {
        // Clamp parameter value to mapping range
        float clamped_param = std::max(mapping.min_parameter_value,
                                     std::min(mapping.max_parameter_value, param_value));
        
        // Normalize to 0-1
        float normalized = (clamped_param - mapping.min_parameter_value) / 
                          (mapping.max_parameter_value - mapping.min_parameter_value);
        
        // Scale to MIDI range
        float midi_float = mapping.min_midi_value + 
                          (normalized * (mapping.max_midi_value - mapping.min_midi_value));
        
        return static_cast<uint8_t>(std::round(midi_float));
    }
    
    /**
     * @brief Set up default parameter mappings
     */
    void setupDefaultMappings() {
        // Filter Cutoff --> MIDI CC 74
        ParameterMapping filter_cutoff;
        filter_cutoff.parameter_id = 1001; // Filter Cutoff ID
        filter_cutoff.midi_channel = 0;    // Channel 1 (0-indexed)
        filter_cutoff.midi_cc_number = 74; // Standard filter cutoff CC
        filter_cutoff.min_parameter_value = 20.0f;   // 20 Hz
        filter_cutoff.max_parameter_value = 20000.0f; // 20 kHz
        filter_cutoff.bidirectional = true;
        filter_cutoff.enabled = true;
        addParameterMapping(filter_cutoff);
        
        // Filter Resonance --> MIDI CC 71
        ParameterMapping filter_resonance;
        filter_resonance.parameter_id = 1002; // Filter Resonance ID
        filter_resonance.midi_channel = 0;
        filter_resonance.midi_cc_number = 71; // Standard filter resonance CC
        filter_resonance.min_parameter_value = 0.0f;
        filter_resonance.max_parameter_value = 1.0f;
        filter_resonance.bidirectional = true;
        filter_resonance.enabled = true;
        addParameterMapping(filter_resonance);
        
        // Envelope Attack --> MIDI CC 73
        ParameterMapping env_attack;
        env_attack.parameter_id = 2001; // Envelope Attack ID
        env_attack.midi_channel = 0;
        env_attack.midi_cc_number = 73; // Standard attack time CC
        env_attack.min_parameter_value = 0.001f;  // 1ms
        env_attack.max_parameter_value = 10.0f;   // 10 seconds
        env_attack.bidirectional = true;
        env_attack.enabled = true;
        addParameterMapping(env_attack);
        
        // Master Volume --> MIDI CC 7
        ParameterMapping master_volume;
        master_volume.parameter_id = 4001; // Master Volume ID
        master_volume.midi_channel = 0;
        master_volume.midi_cc_number = 7;  // Standard volume CC
        master_volume.min_parameter_value = 0.0f;
        master_volume.max_parameter_value = 1.0f;
        master_volume.bidirectional = true;
        master_volume.enabled = true;
        addParameterMapping(master_volume);
    }
};

} // namespace RTSafe
