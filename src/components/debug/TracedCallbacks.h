#pragma once

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)

#include "RTEventTracer.h"
#include <functional>
#include <string>

namespace Debug {

/**
 * @brief Macro system for traced callback registration
 * 
 * Provides macros to replace normal callback registrations with traced versions
 * that automatically log event flow through the observer system.
 */

// Helper class for callback tracing
class CallbackTracer {
public:
    // Template wrapper for callbacks with different signatures
    template<typename CallbackType>
    static auto wrapCallback(const char* source, const char* target, const char* event_name, CallbackType&& callback) {
        return [source, target, event_name, callback = std::forward<CallbackType>(callback)](auto&&... args) {
            // Trace the event before calling callback
            std::string data = formatCallbackData(args...);
            RTEventTracer::getInstance().traceRTEvent(source, target, event_name, data.c_str(), 
                                                     RTEventTracer::EventType::UI_EVENT);
            
            // Call original callback
            return callback(std::forward<decltype(args)>(args)...);
        };
    }
    
    // Helper for clock tick callbacks (high frequency - different type)
    template<typename CallbackType>
    static auto wrapClockCallback(const char* source, const char* target, const char* event_name, CallbackType&& callback) {
        return [source, target, event_name, callback = std::forward<CallbackType>(callback)](auto&&... args) {
            // Trace the event before calling callback
            std::string data = formatCallbackData(args...);
            RTEventTracer::getInstance().traceRTEvent(source, target, event_name, data.c_str(), 
                                                     RTEventTracer::EventType::CLOCK_EVENT);
            
            // Call original callback
            return callback(std::forward<decltype(args)>(args)...);
        };
    }
    
    // Helper for MIDI callbacks
    template<typename CallbackType>
    static auto wrapMidiCallback(const char* source, const char* target, const char* event_name, CallbackType&& callback) {
        return [source, target, event_name, callback = std::forward<CallbackType>(callback)](auto&&... args) {
            // Trace the event before calling callback
            std::string data = formatCallbackData(args...);
            RTEventTracer::getInstance().traceRTEvent(source, target, event_name, data.c_str(), 
                                                     RTEventTracer::EventType::MIDI_EVENT);
            
            // Call original callback
            return callback(std::forward<decltype(args)>(args)...);
        };
    }
    
    // Helper for settings callbacks
    template<typename CallbackType>
    static auto wrapSettingsCallback(const char* source, const char* target, const char* event_name, CallbackType&& callback) {
        return [source, target, event_name, callback = std::forward<CallbackType>(callback)](auto&&... args) {
            // Trace the event before calling callback
            std::string data = formatCallbackData(args...);
            RTEventTracer::getInstance().traceRTEvent(source, target, event_name, data.c_str(), 
                                                     RTEventTracer::EventType::SETTINGS_EVENT);
            
            // Call original callback
            return callback(std::forward<decltype(args)>(args)...);
        };
    }
    
private:
    // Format callback arguments into string for tracing
    template<typename... Args>
    static std::string formatCallbackData(Args&&... args) {
        std::string result;
        ((result += formatArg(args) + ","), ...);
        if (!result.empty()) result.pop_back(); // Remove trailing comma
        return result;
    }
    
    // Format individual arguments
    template<typename T>
    static std::string formatArg(T&& arg) {
        if constexpr (std::is_integral_v<std::decay_t<T>>) {
            return std::to_string(arg);
        } else if constexpr (std::is_floating_point_v<std::decay_t<T>>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
            return arg;
        } else {
            return "complex_type";
        }
    }
};

} // namespace Debug

// Traced callback registration macros for MidiClockManager
#define TRACED_CLOCK_TICK_CALLBACK(manager, callback, target_name) \
    manager.setClockTickCallback(Debug::CallbackTracer::wrapClockCallback("MidiClockManager", target_name, "onClockTick", callback))

#define TRACED_TRANSPORT_CHANGED_CALLBACK(manager, callback, target_name) \
    manager.setTransportChangedCallback(Debug::CallbackTracer::wrapCallback("MidiClockManager", target_name, "onTransportChanged", callback))

#define TRACED_BPM_CHANGED_CALLBACK(manager, callback, target_name) \
    manager.setBPMChangedCallback(Debug::CallbackTracer::wrapCallback("MidiClockManager", target_name, "onBPMChanged", callback))

// Traced observer registration macros for SettingsManager
#define TRACED_SETTINGS_OBSERVER(manager, observer_name, callback) \
    manager.addObserver(observer_name, Debug::CallbackTracer::wrapSettingsCallback("SettingsManager", observer_name, "onSettingChanged", callback))

// Traced callback registration macros for TransportControl
#define TRACED_TRANSPORT_CONTROL_CALLBACK(control, callback, target_name) \
    control->setTransportChangedCallback(Debug::CallbackTracer::wrapCallback("TransportControl", target_name, "onTransportStateChanged", callback))

// Traced callback registration macros for MIDI events
#define TRACED_MIDI_INPUT_CALLBACK(handler, callback, target_name) \
    handler.setMidiInputCallback(Debug::CallbackTracer::wrapMidiCallback("MidiHandler", target_name, "onMidiInput", callback))

#define TRACED_MIDI_OUTPUT_CALLBACK(handler, callback, target_name) \
    handler.setMidiOutputCallback(Debug::CallbackTracer::wrapMidiCallback("MidiHandler", target_name, "onMidiOutput", callback))

#else

// No-op macros when visualizer is disabled - use original callback registration
#define TRACED_CLOCK_TICK_CALLBACK(manager, callback, target_name) \
    manager.setClockTickCallback(callback)

#define TRACED_TRANSPORT_CHANGED_CALLBACK(manager, callback, target_name) \
    manager.setTransportChangedCallback(callback)

#define TRACED_BPM_CHANGED_CALLBACK(manager, callback, target_name) \
    manager.setBPMChangedCallback(callback)

#define TRACED_SETTINGS_OBSERVER(manager, observer_name, callback) \
    manager.addObserver(observer_name, callback)

#define TRACED_TRANSPORT_CONTROL_CALLBACK(control, callback, target_name) \
    control->setTransportChangedCallback(callback)

#define TRACED_MIDI_INPUT_CALLBACK(handler, callback, target_name) \
    handler.setMidiInputCallback(callback)

#define TRACED_MIDI_OUTPUT_CALLBACK(handler, callback, target_name) \
    handler.setMidiOutputCallback(callback)

#endif // DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER
