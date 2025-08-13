#pragma once

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)

#include "EventFlowVisualizer.h"
#include "RTEventTracer.h"
#include <lvgl.h>
#include <memory>

namespace Debug {

/**
 * @brief Manager for the complete event visualization system
 * 
 * Coordinates between the RT-safe event tracer and the visual display.
 * Acts as the main interface for integrating event visualization into applications.
 */
class EventVisualizerManager {
private:
    static EventVisualizerManager* instance_;
    std::unique_ptr<EventFlowVisualizer> visualizer_;
    lv_obj_t* container_;
    lv_obj_t* event_log_area_;
    lv_timer_t* update_timer_;
    bool initialized_;
    uint32_t last_event_count_;
    
    EventVisualizerManager() : container_(nullptr), event_log_area_(nullptr), update_timer_(nullptr), initialized_(false), last_event_count_(0) {}
    
public:
    static EventVisualizerManager* getInstance() {
        if (!instance_) {
            instance_ = new EventVisualizerManager();
        }
        return instance_;
    }
    
    /**
     * @brief Initialize the event visualizer in the given container
     * @param container LVGL container where the visualizer should be displayed
     * @return true if initialization successful
     */
    bool initialize(lv_obj_t* container) {
        if (initialized_) {
            return true; // Already initialized
        }
        
        if (!container) {
            return false;
        }
        
        container_ = container;
        
        // Create the event flow visualizer
        visualizer_ = std::make_unique<EventFlowVisualizer>();
        
        // Initialize the visualizer with the container
        if (!visualizer_->initialize(container)) {
            return false;
        }
        
        // Set up some example components for visualization
        setupExampleComponents();
        
        // Start real-time event update timer
        update_timer_ = lv_timer_create(updateEventsCallback, 100, this); // 10Hz updates
        
        // Inject a test event to verify the system is working
        auto& tracer = Debug::RTEventTracer::getInstance();
        tracer.traceRTEvent("TestSystem", "EventVisualizer", "systemStarted", "init", 
                           Debug::RTEventTracer::EventType::UI_EVENT, 
                           Debug::RTEventTracer::Priority::NORMAL);
        
        initialized_ = true;
        return true;
    }
    
    /**
     * @brief Get the event flow visualizer instance
     */
    EventFlowVisualizer* getVisualizer() {
        return visualizer_.get();
    }
    
    /**
     * @brief Check if the manager is initialized
     */
    bool isInitialized() const {
        return initialized_;
    }
    
private:
    void setupExampleComponents() {
        if (!visualizer_) return;
        
        // Add some simple test content first to make sure we can see something
        lv_obj_t* test_label = lv_label_create(container_);
        lv_label_set_text(test_label, "🎵 Event Visualizer Active\n\nSynthesizer Architecture:");
        lv_obj_set_style_text_color(test_label, lv_color_hex(0x00FF88), 0);
        lv_obj_set_pos(test_label, 20, 50);
        
        // Add some visual component boxes
        lv_obj_t* midi_box = lv_obj_create(container_);
        lv_obj_set_size(midi_box, 120, 60);
        lv_obj_set_pos(midi_box, 50, 120);
        lv_obj_set_style_bg_color(midi_box, lv_color_hex(0x4CAF50), 0);
        lv_obj_set_style_border_color(midi_box, lv_color_hex(0x2E7D32), 0);
        lv_obj_set_style_border_width(midi_box, 2, 0);
        lv_obj_set_style_radius(midi_box, 8, 0);
        
        lv_obj_t* midi_label = lv_label_create(midi_box);
        lv_label_set_text(midi_label, "MIDI\nClock");
        lv_obj_set_style_text_color(midi_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(midi_label);
        
        // Parameter box
        lv_obj_t* param_box = lv_obj_create(container_);
        lv_obj_set_size(param_box, 120, 60);
        lv_obj_set_pos(param_box, 200, 120);
        lv_obj_set_style_bg_color(param_box, lv_color_hex(0x2196F3), 0);
        lv_obj_set_style_border_color(param_box, lv_color_hex(0x1565C0), 0);
        lv_obj_set_style_border_width(param_box, 2, 0);
        lv_obj_set_style_radius(param_box, 8, 0);
        
        lv_obj_t* param_label = lv_label_create(param_box);
        lv_label_set_text(param_label, "Parameter\nManager");
        lv_obj_set_style_text_color(param_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(param_label);
        
        // Clock Tab box
        lv_obj_t* clock_box = lv_obj_create(container_);
        lv_obj_set_size(clock_box, 120, 60);
        lv_obj_set_pos(clock_box, 125, 220);
        lv_obj_set_style_bg_color(clock_box, lv_color_hex(0xFF9800), 0);
        lv_obj_set_style_border_color(clock_box, lv_color_hex(0xF57C00), 0);
        lv_obj_set_style_border_width(clock_box, 2, 0);
        lv_obj_set_style_radius(clock_box, 8, 0);
        
        lv_obj_t* clock_label = lv_label_create(clock_box);
        lv_label_set_text(clock_label, "Clock\nTab");
        lv_obj_set_style_text_color(clock_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(clock_label);
        
        // Add some connection lines (simple lines for now)
        createConnectionLine(110, 150, 185, 180); // MIDI -> Clock
        createConnectionLine(260, 150, 185, 200); // Param -> Clock
        
        // Create real-time event log area
        event_log_area_ = lv_obj_create(container_);
        lv_obj_set_size(event_log_area_, 300, 150);
        lv_obj_set_pos(event_log_area_, 20, 300);
        lv_obj_set_style_bg_color(event_log_area_, lv_color_hex(0x1a1a1a), 0);
        lv_obj_set_style_border_color(event_log_area_, lv_color_hex(0x444444), 0);
        lv_obj_set_style_border_width(event_log_area_, 1, 0);
        lv_obj_set_style_radius(event_log_area_, 4, 0);
        lv_obj_set_style_pad_all(event_log_area_, 5, 0);
        
        lv_obj_t* log_title = lv_label_create(event_log_area_);
        lv_label_set_text(log_title, "🔴 LIVE EVENT STREAM");
        lv_obj_set_style_text_color(log_title, lv_color_hex(0xFF4444), 0);
        lv_obj_set_pos(log_title, 5, 5);
        
                // Try to add the actual event flow connections
        visualizer_->addConnection("HardwareMidiBackend", "ParameterManager");
        visualizer_->addConnection("ParameterControl", "MidiControlIntegration");
        visualizer_->addConnection("MidiClockManager", "ClockTab");
        visualizer_->addConnection("ParameterManager", "ClockTab");
        visualizer_->addConnection("SettingsManager", "ClockTab");
        visualizer_->addConnection("MidiControlIntegration", "UnifiedMidiManager");
        visualizer_->addConnection("ParameterManager", "MidiHandler");
        
        // Add parameter lock connections
        visualizer_->addConnection("StepSequencer", "ParameterLockManager");
        visualizer_->addConnection("ParameterLockManager", "ParameterManager");
        visualizer_->addConnection("ParameterLockManager", "StepSequencer");
    }
    
    void createConnectionLine(int x1, int y1, int x2, int y2) {
        static lv_point_precise_t line_points[2];
        line_points[0].x = x1;
        line_points[0].y = y1;
        line_points[1].x = x2;
        line_points[1].y = y2;
        
        lv_obj_t* line = lv_line_create(container_);
        lv_line_set_points(line, line_points, 2);
        lv_obj_set_style_line_width(line, 3, 0);
        lv_obj_set_style_line_color(line, lv_color_hex(0x00FF88), 0);
        lv_obj_set_style_line_opa(line, LV_OPA_70, 0);
    }
    
    // Method declarations for implementations in .cpp file
    void updateRealTimeEvents();
    static void updateEventsCallback(lv_timer_t* timer);
    void createComponentBox(int x, int y, int w, int h, uint32_t color, const char* title, const char* tooltip);
};

} // namespace Debug

#endif // DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER
