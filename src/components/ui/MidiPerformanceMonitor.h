#include <lvgl.h>
#include <vector>
#include <chrono>
#include "components/ui/Tab.h"

class MidiPerformanceMonitor : public Tab {
private:
    lv_obj_t* latency_chart_;
    lv_obj_t* throughput_label_;
    lv_obj_t* drop_rate_label_;
    lv_obj_t* thread_status_label_;
    
    // Performance metrics
    std::vector<float> latency_history_;
    std::atomic<uint32_t> messages_per_second_{0};
    std::atomic<uint32_t> dropped_messages_{0};
    std::atomic<uint32_t> total_messages_{0};
    
    // Update timer
    lv_timer_t* update_timer_;
    
public:
    MidiPerformanceMonitor() : Tab("MIDI Performance") {}
    
    void create(lv_obj_t* parent) override {
        container_ = lv_obj_create(parent);
        lv_obj_set_size(container_, LV_PCT(100), LV_PCT(100));
        
        createLatencyChart();
        createMetricsDisplay();
        createThreadStatus();
        
        // Start update timer (every 100ms)
        update_timer_ = lv_timer_create([](lv_timer_t* timer) {
            auto* monitor = static_cast<MidiPerformanceMonitor*>(timer->user_data);
            monitor->updateDisplay();
        }, 100, this);
    }
    
private:
    void createLatencyChart() {
        // Create chart for latency visualization
        latency_chart_ = lv_chart_create(container_);
        lv_obj_set_size(latency_chart_, 300, 150);
        lv_obj_align(latency_chart_, LV_ALIGN_TOP_LEFT, 10, 10);
        
        lv_chart_set_type(latency_chart_, LV_CHART_TYPE_LINE);
        lv_chart_set_range(latency_chart_, LV_CHART_AXIS_PRIMARY_Y, 0, 5000); // 0-5ms
        lv_chart_set_point_count(latency_chart_, 50); // Last 50 measurements
        
        // Add series for latency
        lv_chart_series_t* ser = lv_chart_add_series(latency_chart_, 
                                                     lv_color_hex(0x00FF00), 
                                                     LV_CHART_AXIS_PRIMARY_Y);
        
        // Label
        lv_obj_t* chart_label = lv_label_create(container_);
        lv_label_set_text(chart_label, "MIDI Latency (μs)");
        lv_obj_align_to(chart_label, latency_chart_, LV_ALIGN_OUT_TOP_MID, 0, -5);
    }
    
    void createMetricsDisplay() {
        // Throughput display
        throughput_label_ = lv_label_create(container_);
        lv_label_set_text(throughput_label_, "Throughput: 0 msg/s");
        lv_obj_align(throughput_label_, LV_ALIGN_TOP_RIGHT, -10, 10);
        
        // Drop rate display
        drop_rate_label_ = lv_label_create(container_);
        lv_label_set_text(drop_rate_label_, "Drop Rate: 0.0%");
        lv_obj_align_to(drop_rate_label_, throughput_label_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
        
        // Style for warning conditions
        static lv_style_t warning_style;
        lv_style_init(&warning_style);
        lv_style_set_text_color(&warning_style, lv_color_hex(0xFF8000));
        
        static lv_style_t error_style;
        lv_style_init(&error_style);
        lv_style_set_text_color(&error_style, lv_color_hex(0xFF0000));
    }
    
    void createThreadStatus() {
        thread_status_label_ = lv_label_create(container_);
        lv_label_set_text(thread_status_label_, "Thread Status: OK");
        lv_obj_align(thread_status_label_, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    }
    
    void updateDisplay() {
        // Update latency chart
        updateLatencyChart();
        
        // Update metrics
        updateMetrics();
        
        // Update thread status
        updateThreadStatus();
    }
    
    void updateLatencyChart() {
        if (latency_history_.size() > 50) {
            latency_history_.erase(latency_history_.begin());
        }
        
        // Get latest latency measurement from MIDI processor
        float latest_latency = getCurrentLatency(); // Implement this
        latency_history_.push_back(latest_latency);
        
        // Update chart
        lv_chart_series_t* ser = lv_chart_get_series_next(latency_chart_, nullptr);
        if (ser) {
            lv_chart_set_next_value(latency_chart_, ser, (int32_t)latest_latency);
        }
        
        // Color coding for latency
        if (latest_latency > 2000) { // > 2ms
            lv_obj_add_style(latency_chart_, &error_style, 0);
        } else if (latest_latency > 1000) { // > 1ms
            lv_obj_add_style(latency_chart_, &warning_style, 0);
        }
    }
    
    void updateMetrics() {
        // Calculate current throughput
        uint32_t current_throughput = messages_per_second_.load();
        
        // Calculate drop rate
        uint32_t total = total_messages_.load();
        uint32_t dropped = dropped_messages_.load();
        float drop_rate = total > 0 ? (float)dropped / total * 100.0f : 0.0f;
        
        // Update labels
        char throughput_text[64];
        snprintf(throughput_text, sizeof(throughput_text), "Throughput: %u msg/s", current_throughput);
        lv_label_set_text(throughput_label_, throughput_text);
        
        char drop_text[64];
        snprintf(drop_text, sizeof(drop_text), "Drop Rate: %.2f%%", drop_rate);
        lv_label_set_text(drop_rate_label_, drop_text);
        
        // Color coding for performance
        if (drop_rate > 5.0f) {
            lv_obj_add_style(drop_rate_label_, &error_style, 0);
        } else if (drop_rate > 1.0f) {
            lv_obj_add_style(drop_rate_label_, &warning_style, 0);
        }
    }
    
    void updateThreadStatus() {
        // Check thread health
        bool midi_thread_ok = checkMidiThreadHealth(); // Implement this
        bool ui_thread_ok = true; // We're running in UI thread
        
        const char* status_text;
        if (midi_thread_ok && ui_thread_ok) {
            status_text = "Thread Status: ✅ OK";
            lv_obj_remove_style_all(thread_status_label_);
        } else {
            status_text = "Thread Status: ❌ ERROR";
            lv_obj_add_style(thread_status_label_, &error_style, 0);
        }
        
        lv_label_set_text(thread_status_label_, status_text);
    }
    
    // Interface methods for MIDI processor to update metrics
public:
    void recordLatency(float latency_us) {
        // Called from MIDI thread via lock-free mechanism
        latency_history_.push_back(latency_us);
    }
    
    void recordThroughput(uint32_t messages_per_sec) {
        messages_per_second_.store(messages_per_sec);
    }
    
    void recordDroppedMessage() {
        dropped_messages_.fetch_add(1);
        total_messages_.fetch_add(1);
    }
    
    void recordProcessedMessage() {
        total_messages_.fetch_add(1);
    }
    
private:
    float getCurrentLatency() {
        // Get latest latency from MIDI processor
        // This should be implemented to read from a lock-free data structure
        return latency_history_.empty() ? 0.0f : latency_history_.back();
    }
    
    bool checkMidiThreadHealth() {
        // Check if MIDI thread is responsive
        // Could check last heartbeat timestamp, etc.
        return true; // Placeholder
    }
};
