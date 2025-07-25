#pragma once

#include <cstdint>
#include <chrono>

namespace Parameters {

/**
 * @brief Parameter identifier type
 * 
 * Each parameter in the system has a unique ID that identifies
 * what parameter is being changed (filter cutoff, resonance, etc.)
 */
using ParameterID = uint32_t;

/**
 * @brief Source of parameter change
 * 
 * Tracks where the parameter change originated from to enable
 * source-specific behavior and prevent feedback loops.
 */
enum class ParameterSource {
    TOUCH_INPUT,        // Touch screen interaction
    MIDI_CC,           // MIDI Control Change message
    MIDI_NRPN,         // MIDI Non-Registered Parameter Number
    AUTOMATION,        // Automation playback
    SEQUENCER,         // Step sequencer
    NETWORK_OSC,       // Network OSC message
    PRESET_LOAD,       // Loading a preset
    MIDI_LEARN,        // MIDI learn assignment
    UNDO_REDO,         // Undo/redo operation
    INTERNAL           // Internal system change
};

/**
 * @brief Unified parameter change event
 * 
 * This is the core event type that represents any parameter change
 * in the system, regardless of the input source. All parameter
 * changes flow through this unified event system.
 */
struct ParameterChangeEvent {
    ParameterID parameter_id;           // Which parameter is changing
    float normalized_value;             // Always 0.0f - 1.0f range
    ParameterSource source;             // Where the change came from
    uint64_t timestamp_us;              // When the change occurred (microseconds)
    uint8_t midi_channel;               // MIDI channel (if applicable)
    uint8_t midi_cc;                    // MIDI CC number (if applicable)
    bool is_gesture_start;              // Start of continuous gesture (touch/knob)
    bool is_gesture_end;                // End of continuous gesture
    
    // Constructors
    ParameterChangeEvent() 
        : parameter_id(0)
        , normalized_value(0.0f)
        , source(ParameterSource::INTERNAL)
        , timestamp_us(0)
        , midi_channel(0)
        , midi_cc(0)
        , is_gesture_start(false)
        , is_gesture_end(false) {}
    
    ParameterChangeEvent(ParameterID id, float value, ParameterSource src)
        : parameter_id(id)
        , normalized_value(value)
        , source(src)
        , timestamp_us(getCurrentTimestamp())
        , midi_channel(0)
        , midi_cc(0)
        , is_gesture_start(false)
        , is_gesture_end(false) {}
    
    ParameterChangeEvent(ParameterID id, float value, ParameterSource src, uint8_t channel, uint8_t cc)
        : parameter_id(id)
        , normalized_value(value)
        , source(src)
        , timestamp_us(getCurrentTimestamp())
        , midi_channel(channel)
        , midi_cc(cc)
        , is_gesture_start(false)
        , is_gesture_end(false) {}
    
    // Factory methods for common sources
    static ParameterChangeEvent fromTouch(ParameterID id, float value) {
        return ParameterChangeEvent(id, value, ParameterSource::TOUCH_INPUT);
    }
    
    static ParameterChangeEvent fromMidiCC(ParameterID id, float value, uint8_t channel, uint8_t cc) {
        return ParameterChangeEvent(id, value, ParameterSource::MIDI_CC, channel, cc);
    }
    
    static ParameterChangeEvent fromAutomation(ParameterID id, float value) {
        return ParameterChangeEvent(id, value, ParameterSource::AUTOMATION);
    }
    
    static ParameterChangeEvent fromPreset(ParameterID id, float value) {
        return ParameterChangeEvent(id, value, ParameterSource::PRESET_LOAD);
    }
    
private:
    static uint64_t getCurrentTimestamp() {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
    }
};

} // namespace Parameters
