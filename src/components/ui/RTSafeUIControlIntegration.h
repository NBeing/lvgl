#pragma once

#include "components/threading/RTSafeEventDistributor.h"
#include "components/parameter/RTSafeParameterManager.h"
#include <functional>
#include <unordered_map>
#include <atomic>
#include <memory>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <string>

namespace RTSafe {

/**
 * @brief RT-Safe UI Control Integration
 * 
 * Connects UI controls (dials, sliders, buttons) to the RT-safe parameter system.
 * Provides thread-safe bidirectional synchronization between UI and audio engine.
 * 
 * Features:
 * - RT-safe parameter updates from UI thread
 * - Automatic UI control updates from MIDI/automation
 * - Smooth value interpolation for UI responsiveness
 * - Parameter validation and range checking
 * - Live performance optimizations
 */
class RTSafeUIControlIntegration : public RTObserver, public UIObserver {
public:
    /**
     * @brief UI Control types supported
     */
    enum class ControlType {
        DIAL,           // Rotary knob/dial
        SLIDER,         // Linear slider/fader
        BUTTON,         // Momentary button
        TOGGLE,         // Toggle switch
        DISPLAY         // Read-only display
    };
    
    /**
     * @brief UI Control configuration
     */
    struct ControlConfig {
        uint32_t control_id;              // Unique control ID
        uint32_t parameter_id;            // Associated parameter ID
        ControlType type;                 // Control type
        float min_value;                  // Minimum value
        float max_value;                  // Maximum value
        float default_value;              // Default/center value
        float step_size;                  // Step size for discrete controls
        bool logarithmic;                 // Use logarithmic scaling
        bool enabled;                     // Is control enabled
        std::string label;                // Control label/name
        std::string units;                // Value units (Hz, dB, %, etc.)
        
        ControlConfig() 
            : control_id(0), parameter_id(0), type(ControlType::DIAL)
            , min_value(0.0f), max_value(1.0f), default_value(0.5f)
            , step_size(0.01f), logarithmic(false), enabled(true)
            , label("Control"), units("") {}
    };
    
    /**
     * @brief UI Control state
     */
    struct ControlState {
        float current_value;              // Current control value
        float target_value;               // Target value (for smooth updates)
        float display_value;              // Value shown in UI
        bool needs_update;                // UI needs refresh
        bool user_interaction;            // User is currently interacting
        uint32_t last_update_time_ms;    // Last update timestamp
        
        ControlState() 
            : current_value(0.0f), target_value(0.0f), display_value(0.0f)
            , needs_update(false), user_interaction(false)
            , last_update_time_ms(0) {}
    };
    
    /**
     * @brief Statistics for monitoring (non-atomic for return)
     */
    struct IntegrationStatistics {
        uint64_t ui_to_param_updates{0};     // UI --> Parameter
        uint64_t param_to_ui_updates{0};     // Parameter --> UI
        uint64_t smooth_interpolations{0};   // Smooth value changes
        uint64_t validation_failures{0};     // Value validation failures
        uint32_t active_controls{0};         // Currently active controls
        uint32_t max_update_time_us{0};      // Max update processing time
        uint32_t last_update_time_us{0};     // Last update processing time
    };
    
    /**
     * @brief UI update callback function type
     */
    using UIUpdateCallback = std::function<void(uint32_t control_id, float value, bool immediate)>;

private:
    // Core components
    RTSafeEventDistributor* event_distributor_;
    RTSafeParameterManager* parameter_manager_;
    
    // Control management
    std::unordered_map<uint32_t, ControlConfig> control_configs_;     // Control ID --> Config
    std::unordered_map<uint32_t, ControlState> control_states_;       // Control ID --> State
    std::unordered_map<uint32_t, uint32_t> param_to_control_map_;     // Parameter ID --> Control ID
    
    // UI callbacks
    UIUpdateCallback ui_update_callback_;
    
    // Smooth interpolation settings
    std::atomic<float> interpolation_speed_{0.1f};    // 0.0 = instant, 1.0 = very slow
    std::atomic<bool> smooth_updates_enabled_{true};
    
    // Statistics (atomic for thread safety)
    std::atomic<uint64_t> ui_to_param_updates_{0};
    std::atomic<uint64_t> param_to_ui_updates_{0};
    std::atomic<uint64_t> smooth_interpolations_{0};
    std::atomic<uint64_t> validation_failures_{0};
    std::atomic<uint32_t> active_controls_{0};
    std::atomic<uint32_t> max_update_time_us_{0};
    std::atomic<uint32_t> last_update_time_us_{0};
    
    // Configuration
    std::atomic<bool> enabled_{true};
    std::atomic<uint32_t> update_rate_hz_{60};        // UI update rate
    
public:
    /**
     * @brief Constructor
     */
    RTSafeUIControlIntegration(RTSafeEventDistributor* event_distributor, RTSafeParameterManager* parameter_manager)
        : event_distributor_(event_distributor), parameter_manager_(parameter_manager) {
        
        if (!event_distributor_) {
            throw std::invalid_argument("RTSafeUIControlIntegration: null event distributor");
        }
        if (!parameter_manager_) {
            throw std::invalid_argument("RTSafeUIControlIntegration: null parameter manager");
        }
    }
    
    /**
     * @brief Initialize the integration
     */
    bool initialize() {
        if (!event_distributor_) {
            return false;
        }
        
        // Register as both RT and UI observer
        event_distributor_->addRTObserver(this);
        event_distributor_->addUIObserver(this);
        
        return true;
    }
    
    /**
     * @brief Shutdown the integration
     */
    void shutdown() {
        enabled_ = false;
        // Note: Observer removal would need to be added to RTSafeEventDistributor
    }
    
    /**
     * @brief Add UI control configuration
     */
    bool addControl(const ControlConfig& config) {
        if (config.control_id == 0 || config.parameter_id == 0) {
            return false;
        }
        
        // Add control configuration
        control_configs_[config.control_id] = config;
        
        // Initialize control state
        ControlState state;
        state.current_value = config.default_value;
        state.target_value = config.default_value;
        state.display_value = config.default_value;
        control_states_[config.control_id] = state;
        
        // Add parameter mapping
        param_to_control_map_[config.parameter_id] = config.control_id;
        
        active_controls_++;
        
        return true;
    }
    
    /**
     * @brief Remove UI control
     */
    void removeControl(uint32_t control_id) {
        auto it = control_configs_.find(control_id);
        if (it != control_configs_.end()) {
            // Remove parameter mapping
            param_to_control_map_.erase(it->second.parameter_id);
            
                        // Remove control
            control_configs_.erase(control_id);
            control_states_.erase(control_id);
            
            active_controls_--;
        }
    }
    
    /**
     * @brief Update control value from UI thread (user interaction)
     */
    bool updateControlFromUI(uint32_t control_id, float raw_value, bool immediate = false) {
        if (!enabled_) return false;
        
        auto config_it = control_configs_.find(control_id);
        auto state_it = control_states_.find(control_id);
        
        if (config_it == control_configs_.end() || state_it == control_states_.end()) {
            validation_failures_++;
            return false;
        }
        
        const ControlConfig& config = config_it->second;
        ControlState& state = state_it->second;
        
        // Validate and clamp value
        float validated_value = validateAndClampValue(raw_value, config);
        
        // Update control state
        state.user_interaction = true;
        state.target_value = validated_value;
        
        if (immediate || !smooth_updates_enabled_) {
            state.current_value = validated_value;
            state.display_value = validated_value;
        }
        
        state.needs_update = true;
        state.last_update_time_ms = getCurrentTimeMs();
        
        // Send parameter change event to RT system
        RTEvent param_event = RTEvent::parameterChange(
            config.parameter_id >> 8, 
            config.parameter_id & 0xFF
        );
        
        if (event_distributor_) {
            event_distributor_->notifyRTObservers(param_event);
            ui_to_param_updates_++;
        }
        
        return true;
    }
    
    /**
     * @brief Set UI update callback
     */
    void setUIUpdateCallback(UIUpdateCallback callback) {
        ui_update_callback_ = callback;
    }
    
    /**
     * @brief Process UI updates (call from UI thread regularly)
     */
    void processUIUpdates() {
        if (!enabled_) return;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (auto& [control_id, state] : control_states_) {
            if (state.needs_update) {
                processControlUpdate(control_id, state);
            }
        }
        
        // Track processing time
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time);
        
        uint32_t duration_us = static_cast<uint32_t>(duration.count());
        last_update_time_us_ = duration_us;
        
        // Update max processing time atomically
        uint32_t current_max = max_update_time_us_.load();
        while (duration_us > current_max && 
               !max_update_time_us_.compare_exchange_weak(current_max, duration_us)) {
            // Retry
        }
    }
    
    /**
     * @brief RT Observer interface - handles RT events
     */
    void handleRTEvent(const RTEvent& event) override {
        if (!enabled_) return;
        
        // RT thread should not update UI directly - queue for UI thread
        // This is handled by the UI observer
    }
    
    /**
     * @brief UI Observer interface - handles UI events
     */
    void handleUIEvent(const RTEvent& event) override {
        if (!enabled_) return;
        
        switch (event.type) {
            case EventType::PARAMETER_CHANGE:
                handleParameterChangeFromRT(event);
                break;
                
            case EventType::CONTROL_CHANGE:
                handleMidiControlChange(event);
                break;
                
            default:
                break;
        }
    }
    
    /**
     * @brief Get observer priority
     */
    int getPriority() const override {
        return 3; // Lower priority than bridge
    }
    
    /**
     * @brief Get control value
     */
    float getControlValue(uint32_t control_id) const {
        auto it = control_states_.find(control_id);
        return (it != control_states_.end()) ? it->second.current_value : 0.0f;
    }
    
    /**
     * @brief Get control display value
     */
    float getControlDisplayValue(uint32_t control_id) const {
        auto it = control_states_.find(control_id);
        return (it != control_states_.end()) ? it->second.display_value : 0.0f;
    }
    
    /**
     * @brief Check if control needs UI update
     */
    bool controlNeedsUpdate(uint32_t control_id) const {
        auto it = control_states_.find(control_id);
        return (it != control_states_.end()) ? it->second.needs_update : false;
    }
    
    /**
     * @brief Mark control as updated
     */
    void markControlUpdated(uint32_t control_id) {
        auto it = control_states_.find(control_id);
        if (it != control_states_.end()) {
            it->second.needs_update = false;
        }
    }
    
    /**
     * @brief Get integration statistics
     */
    IntegrationStatistics getStatistics() const {
        IntegrationStatistics stats;
        stats.ui_to_param_updates = ui_to_param_updates_.load();
        stats.param_to_ui_updates = param_to_ui_updates_.load();
        stats.smooth_interpolations = smooth_interpolations_.load();
        stats.validation_failures = validation_failures_.load();
        stats.active_controls = active_controls_.load();
        stats.max_update_time_us = max_update_time_us_.load();
        stats.last_update_time_us = last_update_time_us_.load();
        return stats;
    }
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics() {
        ui_to_param_updates_ = 0;
        param_to_ui_updates_ = 0;
        smooth_interpolations_ = 0;
        validation_failures_ = 0;
        max_update_time_us_ = 0;
        last_update_time_us_ = 0;
    }
    
    /**
     * @brief Enable/disable smooth updates
     */
    void setSmoothUpdatesEnabled(bool enabled) {
        smooth_updates_enabled_ = enabled;
    }
    
    bool isSmoothUpdatesEnabled() const {
        return smooth_updates_enabled_;
    }
    
    /**
     * @brief Set interpolation speed (0.0 = instant, 1.0 = very slow)
     */
    void setInterpolationSpeed(float speed) {
        interpolation_speed_ = std::max(0.0f, std::min(1.0f, speed));
    }
    
    float getInterpolationSpeed() const {
        return interpolation_speed_;
    }
    
    /**
     * @brief Clear user interaction flag for a control (for testing)
     */
    void clearUserInteraction(uint32_t control_id) {
        auto it = control_states_.find(control_id);
        if (it != control_states_.end()) {
            it->second.user_interaction = false;
        }
    }

private:
    /**
     * @brief Handle parameter change from RT thread
     */
    void handleParameterChangeFromRT(const RTEvent& event) {
        // Find control associated with this parameter
        uint32_t param_id = (static_cast<uint32_t>(event.data1) << 8) | event.data2;
        
        auto it = param_to_control_map_.find(param_id);
        if (it != param_to_control_map_.end()) {
            uint32_t control_id = it->second;
            
            // Update control from parameter change
            updateControlFromParameter(control_id, param_id);
            param_to_ui_updates_++;
        }
    }
    
    /**
     * @brief Handle MIDI control change
     */
    void handleMidiControlChange(const RTEvent& event) {
        // MIDI CC changes are handled by the bridge and result in parameter changes
        // which then trigger UI updates through handleParameterChangeFromRT
    }
    
    /**
     * @brief Update control from parameter change
     */
    void updateControlFromParameter(uint32_t control_id, uint32_t param_id) {
        auto config_it = control_configs_.find(control_id);
        auto state_it = control_states_.find(control_id);
        
        if (config_it == control_configs_.end() || state_it == control_states_.end()) {
            return;
        }
        
        const ControlConfig& config = config_it->second;
        ControlState& state = state_it->second;
        
        // Skip update if user is currently interacting with control
        if (state.user_interaction) {
            return;
        }
        
        // Get parameter value from the actual parameter manager
        float param_value = parameter_manager_->getParameterTarget(param_id);
        
        // Convert parameter value to control value
        float control_value = parameterToControlValue(param_value, config);
        
        // Update control state
        state.target_value = control_value;
        state.needs_update = true;
        state.last_update_time_ms = getCurrentTimeMs();
    }
    
    /**
     * @brief Process individual control update
     */
    void processControlUpdate(uint32_t control_id, ControlState& state) {
        const ControlConfig& config = control_configs_[control_id];
        
        // Apply smooth interpolation if enabled
        if (smooth_updates_enabled_ && !state.user_interaction) {
            float interpolation_factor = 1.0f - interpolation_speed_.load();
            
            float diff = state.target_value - state.current_value;
            if (std::abs(diff) > 0.001f) {  // Only interpolate if significant difference
                state.current_value += diff * interpolation_factor;
                smooth_interpolations_++;
                // Keep needs_update true so we continue interpolating
                state.needs_update = true;
            } else {
                state.current_value = state.target_value;
                // We've reached the target, no more updates needed
                state.needs_update = false;
            }
        } else {
            state.current_value = state.target_value;
            // Immediate update, clear needs_update
            state.needs_update = false;
        }
        
        // Update display value
        state.display_value = controlToDisplayValue(state.current_value, config);
        
        // Call UI update callback
        if (ui_update_callback_) {
            // immediate = true if user interaction OR if we've reached the target value
            bool immediate = state.user_interaction || (std::abs(state.current_value - state.target_value) < 0.001f);
            ui_update_callback_(control_id, state.display_value, immediate);
        }
        
        // Reset user interaction flag after a timeout
        uint32_t current_time = getCurrentTimeMs();
        if (state.user_interaction && 
            (current_time - state.last_update_time_ms) > 100) { // 100ms timeout
            state.user_interaction = false;
        }
    }
    
    /**
     * @brief Validate and clamp control value
     */
    float validateAndClampValue(float value, const ControlConfig& config) const {
        // Apply step size if configured (in 0-1 range)
        if (config.step_size > 0.0f) {
            // Convert step size from parameter range to control range
            float normalized_step = config.step_size / (config.max_value - config.min_value);
            value = std::round(value / normalized_step) * normalized_step;
        }
        
        // Clamp to 0-1 range (control values are always normalized)
        return std::max(0.0f, std::min(1.0f, value));
    }
    
    /**
     * @brief Convert parameter value to control value
     */
    float parameterToControlValue(float param_value, const ControlConfig& config) const {
        // Clamp parameter value to valid range
        param_value = std::max(config.min_value, std::min(config.max_value, param_value));
        
        if (config.logarithmic) {
            // Use proper logarithmic scaling for frequency ranges
            // Formula: control_value = log(param_value/min_value) / log(max_value/min_value)
            if (config.min_value > 0.0f && config.max_value > config.min_value && param_value > 0.0f) {
                float ratio = config.max_value / config.min_value;
                float control_value = std::log(param_value / config.min_value) / std::log(ratio);
                return std::max(0.0f, std::min(1.0f, control_value));
            } else {
                // Fallback to linear if invalid range
                return (param_value - config.min_value) / (config.max_value - config.min_value);
            }
        }
        
        // Linear scaling to 0-1 range
        return (param_value - config.min_value) / (config.max_value - config.min_value);
    }
    
    /**
     * @brief Convert control value to display value
     */
    float controlToDisplayValue(float control_value, const ControlConfig& config) const {
        // Clamp control_value to prevent numerical issues
        control_value = std::max(0.0f, std::min(1.0f, control_value));
        
        float normalized_value = control_value;
        
        // Apply inverse logarithmic scaling if configured
        if (config.logarithmic) {
            // Use proper logarithmic scaling for frequency ranges
            // Formula: freq = min_freq * (max_freq/min_freq)^control_value
            if (config.min_value > 0.0f && config.max_value > config.min_value) {
                float ratio = config.max_value / config.min_value;
                float display_value = config.min_value * std::pow(ratio, control_value);
                return display_value;
            } else {
                // Fallback to linear if invalid range
                normalized_value = control_value;
            }
        }
        
        // Linear scaling to parameter range
        float display_value = config.min_value + normalized_value * (config.max_value - config.min_value);
        return display_value;
    }
    
    /**
     * @brief Get current time in milliseconds
     */
    uint32_t getCurrentTimeMs() const {
        auto now = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        return static_cast<uint32_t>(ms.count());
    }
};

} // namespace RTSafe
