#pragma once

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)

#include "EventFlowVisualizer.h"
#include "RTEventTracer.h"
#include <lvgl.h>
#include <memory>

namespace Debug {

/**
 * @brief Integration layer that connects RT event tracing to visualization
 * 
 * This class bridges the RT-safe event tracer with the LVGL-based visualizer,
 * processing events from the lock-free buffer and updating the visual graph.
 */
class EventVisualizerIntegration {
private:
    std::unique_ptr<EventFlowVisualizer> visualizer_;
    RTEventTracer* tracer_;
    lv_timer_t* update_timer_;
    
    // Processing control
    bool initialized_;
    uint32_t update_interval_ms_;
    uint32_t max_events_per_update_;
    
    // Statistics
    uint64_t last_processed_event_count_;
    uint64_t ui_update_count_;
    
public:
    EventVisualizerIntegration();
    ~EventVisualizerIntegration();
    
    // Lifecycle management
    bool initialize(lv_obj_t* parent);
    void shutdown();
    
    // Get access to ESP32 area for main app
    lv_obj_t* getESP32Area() const;
    
    // Control
    void setUpdateInterval(uint32_t interval_ms);
    void setMaxEventsPerUpdate(uint32_t max_events);
    void pauseVisualization(bool paused);
    
    // Statistics
    uint64_t getUIUpdateCount() const { return ui_update_count_; }
    double getEventProcessingRate() const;
    
private:
    void processEvents();
    std::string eventTypeToString(RTEventTracer::EventType type);
    uint32_t eventTypeToColor(RTEventTracer::EventType type);
    
    // Timer callback
    static void updateTimerCallback(lv_timer_t* timer);
};

} // namespace Debug

// Convenience macros for traced callback registration
#define SET_TRACED_CALLBACK(manager, method, callback, target_name) \
    do { \
        TRACE_UI_EVENT(#manager, target_name, #method " registration", ""); \
        auto traced_callback = [callback](auto... args) { \
            TRACE_CLOCK_EVENT(#manager, target_name, #method, ""); \
            return callback(args...); \
        }; \
        manager.method(traced_callback); \
    } while(0)

#define ADD_TRACED_OBSERVER(manager, observer_name, callback) \
    do { \
        TRACE_UI_EVENT(#manager, observer_name, "observer registration", ""); \
        auto traced_callback = [callback](auto... args) { \
            TRACE_SETTINGS_EVENT(#manager, observer_name, "setting changed", ""); \
            return callback(args...); \
        }; \
        manager.addObserver(observer_name, traced_callback); \
    } while(0)

#else

// No-op macros when visualizer is disabled
#define SET_TRACED_CALLBACK(manager, method, callback, target_name) \
    manager.method(callback)

#define ADD_TRACED_OBSERVER(manager, observer_name, callback) \
    manager.addObserver(observer_name, callback)

#endif // DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER
