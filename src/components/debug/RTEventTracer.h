#pragma once

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)

#include <atomic>
#include <string>
#include <chrono>
#include <cstring>
#include <type_traits>

namespace Debug {

/**
 * @brief RT-safe event tracer with lock-free circular buffer
 * 
 * Provides high-performance, real-time safe event tracing for observer
 * pattern debugging. Events are captured in RT threads without blocking
 * and consumed by UI thread for visualization.
 */
class RTEventTracer {
public:
    struct EventTrace {
        char source_name[32];
        char target_name[32];
        char event_name[32];
        char event_data[64];
        uint64_t timestamp_us;
        uint32_t thread_id;
        uint8_t event_type;  // 0=RT, 1=UI, 2=MIDI, 3=Parameter
        uint8_t priority;    // 0=Low, 1=Normal, 2=High, 3=Critical
    };

    enum class EventType : uint8_t {
        RT_EVENT = 0,
        UI_EVENT = 1,
        MIDI_EVENT = 2,
        PARAMETER_EVENT = 3,
        CLOCK_EVENT = 4,
        SETTINGS_EVENT = 5
    };

    enum class Priority : uint8_t {
        LOW = 0,
        NORMAL = 1,
        HIGH = 2,
        CRITICAL = 3
    };

private:
    static constexpr size_t BUFFER_SIZE = 8192;  // Must be power of 2
    static constexpr size_t BUFFER_MASK = BUFFER_SIZE - 1;
    
    // Lock-free circular buffer
    alignas(64) std::atomic<size_t> write_index_{0};
    alignas(64) std::atomic<size_t> read_index_{0};
    EventTrace trace_buffer_[BUFFER_SIZE];
    
    // Statistics
    std::atomic<uint64_t> total_events_{0};
    std::atomic<uint64_t> dropped_events_{0};
    std::atomic<bool> tracing_enabled_{true};
    
public:
    static RTEventTracer& getInstance();
    
    // RT-safe event tracing (called from RT threads)
    void traceRTEvent(const char* source, const char* target, 
                     const char* event_name, const char* data = "",
                     EventType type = EventType::RT_EVENT,
                     Priority priority = Priority::NORMAL);
    
    // Template version for easy parameter conversion (only for numeric types)
    template<typename T>
    void traceRTEvent(const char* source, const char* target, 
                     const char* event_name, T data,
                     EventType type = EventType::RT_EVENT,
                     Priority priority = Priority::NORMAL,
                     typename std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr) {
        traceRTEvent(source, target, event_name, std::to_string(data).c_str(), type, priority);
    }
    
    // Overload for string types (no conversion needed)
    void traceRTEvent(const char* source, const char* target, 
                     const char* event_name, const std::string& data,
                     EventType type = EventType::RT_EVENT,
                     Priority priority = Priority::NORMAL) {
        traceRTEvent(source, target, event_name, data.c_str(), type, priority);
    }
    
    // UI thread consumption (called from UI thread)
    bool popTrace(EventTrace& trace);
    size_t getAvailableTraces() const;
    
    // Statistics and control
    uint64_t getTotalEvents() const { return total_events_.load(); }
    uint64_t getDroppedEvents() const { return dropped_events_.load(); }
    void setTracingEnabled(bool enabled) { tracing_enabled_ = enabled; }
    bool isTracingEnabled() const { return tracing_enabled_.load(); }
    
    // Performance monitoring
    double getBufferUtilization() const;
    void resetStatistics();

private:
    RTEventTracer() = default;
    ~RTEventTracer() = default;
    RTEventTracer(const RTEventTracer&) = delete;
    RTEventTracer& operator=(const RTEventTracer&) = delete;
    
    uint64_t getCurrentTimeMicros() const;
    uint32_t getCurrentThreadId() const;
    void safeCopyString(char* dest, const char* src, size_t max_len);
};

} // namespace Debug

// Convenience macros for event tracing
#define TRACE_RT_EVENT(source, target, event, data) \
    Debug::RTEventTracer::getInstance().traceRTEvent(source, target, event, data, Debug::RTEventTracer::EventType::RT_EVENT)

#define TRACE_UI_EVENT(source, target, event, data) \
    Debug::RTEventTracer::getInstance().traceRTEvent(source, target, event, data, Debug::RTEventTracer::EventType::UI_EVENT)

#define TRACE_MIDI_EVENT(source, target, event, data) \
    Debug::RTEventTracer::getInstance().traceRTEvent(source, target, event, data, Debug::RTEventTracer::EventType::MIDI_EVENT)

#define TRACE_PARAMETER_EVENT(source, target, event, data) \
    Debug::RTEventTracer::getInstance().traceRTEvent(source, target, event, data, Debug::RTEventTracer::EventType::PARAMETER_EVENT)

#define TRACE_CLOCK_EVENT(source, target, event, data) \
    Debug::RTEventTracer::getInstance().traceRTEvent(source, target, event, data, Debug::RTEventTracer::EventType::CLOCK_EVENT)

#define TRACE_SETTINGS_EVENT(source, target, event, data) \
    Debug::RTEventTracer::getInstance().traceRTEvent(source, target, event, data, Debug::RTEventTracer::EventType::SETTINGS_EVENT)

#else

// No-op macros when visualizer is disabled
#define TRACE_RT_EVENT(source, target, event, data)
#define TRACE_UI_EVENT(source, target, event, data)
#define TRACE_MIDI_EVENT(source, target, event, data)
#define TRACE_PARAMETER_EVENT(source, target, event, data)
#define TRACE_CLOCK_EVENT(source, target, event, data)
#define TRACE_SETTINGS_EVENT(source, target, event, data)

#endif // DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER
