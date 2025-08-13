#include "EventVisualizerIntegration.h"

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)

#include <lvgl.h>
#include <iostream>
#include <algorithm>

namespace Debug {

// ============================================================================
// EventVisualizerIntegration Implementation
// ============================================================================

EventVisualizerIntegration::EventVisualizerIntegration()
    : visualizer_(nullptr)
    , tracer_(nullptr)
    , update_timer_(nullptr)
    , initialized_(false)
    , update_interval_ms_(50)  // 20 FPS default
    , max_events_per_update_(10)
    , last_processed_event_count_(0)
    , ui_update_count_(0) {
}

EventVisualizerIntegration::~EventVisualizerIntegration() {
    shutdown();
}

bool EventVisualizerIntegration::initialize(lv_obj_t* parent) {
    if (initialized_) {
        return true;
    }
    
    // Initialize visualizer
    visualizer_ = std::make_unique<EventFlowVisualizer>();
    if (!visualizer_->initialize(parent)) {
        std::cerr << "[EventVisualizerIntegration] Failed to initialize visualizer" << std::endl;
        return false;
    }
    
    // Get tracer instance
    tracer_ = &RTEventTracer::getInstance();
    
    // Create update timer
    update_timer_ = lv_timer_create(updateTimerCallback, update_interval_ms_, this);
    if (!update_timer_) {
        std::cerr << "[EventVisualizerIntegration] Failed to create update timer" << std::endl;
        return false;
    }
    
    initialized_ = true;
    
    // Add some default nodes to demonstrate the system
    visualizer_->addNode("MidiClockManager", 50, 50);
    visualizer_->addNode("ClockTab", 250, 50);
    visualizer_->addNode("TransportControl", 450, 50);
    visualizer_->addNode("SettingsManager", 50, 200);
    visualizer_->addNode("HardwareMidiManager", 250, 200);
    
    // Add some default connections
    visualizer_->addConnection("MidiClockManager", "ClockTab");
    visualizer_->addConnection("ClockTab", "TransportControl");
    visualizer_->addConnection("SettingsManager", "ClockTab");
    visualizer_->addConnection("HardwareMidiManager", "ClockTab");
    
    std::cout << "[EventVisualizerIntegration] Initialized successfully" << std::endl;
    return true;
}

void EventVisualizerIntegration::shutdown() {
    if (update_timer_) {
        lv_timer_del(update_timer_);
        update_timer_ = nullptr;
    }
    
    visualizer_.reset();
    tracer_ = nullptr;
    initialized_ = false;
    
    std::cout << "[EventVisualizerIntegration] Shutdown completed" << std::endl;
}

lv_obj_t* EventVisualizerIntegration::getESP32Area() const {
    return visualizer_ ? visualizer_->getESP32Area() : nullptr;
}

void EventVisualizerIntegration::setUpdateInterval(uint32_t interval_ms) {
    update_interval_ms_ = interval_ms;
    if (update_timer_) {
        lv_timer_set_period(update_timer_, interval_ms);
    }
}

void EventVisualizerIntegration::setMaxEventsPerUpdate(uint32_t max_events) {
    max_events_per_update_ = max_events;
}

void EventVisualizerIntegration::pauseVisualization(bool paused) {
    if (update_timer_) {
        if (paused) {
            lv_timer_pause(update_timer_);
        } else {
            lv_timer_resume(update_timer_);
        }
    }
    
    if (visualizer_) {
        visualizer_->setPaused(paused);
    }
}

double EventVisualizerIntegration::getEventProcessingRate() const {
    if (!tracer_) return 0.0;
    
    uint64_t current_count = tracer_->getTotalEvents();
    uint64_t events_processed = current_count - last_processed_event_count_;
    
    // Rate = events per second (assuming 50ms update interval)
    return static_cast<double>(events_processed) * (1000.0 / update_interval_ms_);
}

void EventVisualizerIntegration::processEvents() {
    if (!tracer_ || !visualizer_) {
        return;
    }
    
    RTEventTracer::EventTrace trace;
    uint32_t events_processed = 0;
    
    // Process up to max_events_per_update_ events per timer tick
    while (events_processed < max_events_per_update_ && tracer_->popTrace(trace)) {
        // Convert event to visualizer format
        std::string event_display = eventTypeToString(static_cast<RTEventTracer::EventType>(trace.event_type));
        if (strlen(trace.event_data) > 0) {
            event_display += "(" + std::string(trace.event_data) + ")";
        }
        
        // uint32_t color = eventTypeToColor(static_cast<RTEventTracer::EventType>(trace.event_type));
        // TODO: Use color for visual differentiation in future
        
        // Trace the event in the visualizer
        visualizer_->traceEvent(trace.source_name, trace.target_name, event_display, "");
        
        events_processed++;
    }
    
    if (events_processed > 0) {
        ui_update_count_++;
        last_processed_event_count_ = tracer_->getTotalEvents();
    }
}

std::string EventVisualizerIntegration::eventTypeToString(RTEventTracer::EventType type) {
    switch (type) {
        case RTEventTracer::EventType::RT_EVENT: return "RT";
        case RTEventTracer::EventType::UI_EVENT: return "UI";
        case RTEventTracer::EventType::MIDI_EVENT: return "MIDI";
        case RTEventTracer::EventType::PARAMETER_EVENT: return "Param";
        case RTEventTracer::EventType::CLOCK_EVENT: return "Clock";
        case RTEventTracer::EventType::SETTINGS_EVENT: return "Settings";
        default: return "Unknown";
    }
}

uint32_t EventVisualizerIntegration::eventTypeToColor(RTEventTracer::EventType type) {
    switch (type) {
        case RTEventTracer::EventType::RT_EVENT: return 0xFF6B35;        // Orange - RT events
        case RTEventTracer::EventType::UI_EVENT: return 0x00AAFF;        // Blue - UI events
        case RTEventTracer::EventType::MIDI_EVENT: return 0xFF0080;      // Magenta - MIDI events
        case RTEventTracer::EventType::PARAMETER_EVENT: return 0x80FF00; // Green - Parameter events
        case RTEventTracer::EventType::CLOCK_EVENT: return 0xFFFF00;     // Yellow - Clock events
        case RTEventTracer::EventType::SETTINGS_EVENT: return 0x00FFFF;  // Cyan - Settings events
        default: return 0xFFFFFF;                                        // White - Unknown
    }
}

void EventVisualizerIntegration::updateTimerCallback(lv_timer_t* timer) {
    auto* integration = static_cast<EventVisualizerIntegration*>(lv_timer_get_user_data(timer));
    integration->processEvents();
}

} // namespace Debug

#endif // DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER
