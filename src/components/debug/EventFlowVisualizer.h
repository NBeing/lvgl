#pragma once

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)

#include <lvgl.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <deque>
#include <chrono>

namespace Debug {

/**
 * @brief Individual node in the event flow graph
 */
class EventFlowNode {
public:
    struct NodeInfo {
        std::string name;
        lv_coord_t x, y;
        lv_coord_t width, height;
        uint32_t color;
        bool active;
        uint32_t input_count;
        uint32_t output_count;
    };

private:
    lv_obj_t* node_container_;
    lv_obj_t* node_title_;
    lv_obj_t* node_status_;
    NodeInfo info_;
    
public:
    EventFlowNode(const std::string& name, lv_coord_t x, lv_coord_t y);
    ~EventFlowNode();
    
    void create(lv_obj_t* parent);
    void setPosition(lv_coord_t x, lv_coord_t y);
    void setActive(bool active);
    void updateStatus(const std::string& status);
    
    const NodeInfo& getInfo() const { return info_; }
    lv_obj_t* getContainer() const { return node_container_; }
    
    // Connection points
    lv_point_t getInputPoint() const;
    lv_point_t getOutputPoint() const;
};

/**
 * @brief Connection line between nodes with event history
 */
class EventFlowConnection {
public:
    struct EventEntry {
        std::string event_name;
        std::string event_data;
        uint64_t timestamp_us;
        uint32_t color;
    };
    
    // Friend class to allow EventFlowVisualizer access to private members
    friend class EventFlowVisualizer;
    
private:
    lv_obj_t* connection_line_;
    lv_obj_t* event_log_container_;
    lv_obj_t* event_labels_[5];  // Show last 5 events
    
    std::string source_name_;
    std::string target_name_;
    std::deque<EventEntry> recent_events_;
    
    lv_point_t start_point_;
    lv_point_t end_point_;
    
    // Private methods
    void updateEventDisplay();
    
public:
    EventFlowConnection(const std::string& source, const std::string& target);
    ~EventFlowConnection();
    
    // Public getter methods for accessing private data
    const std::string& getSourceName() const { return source_name_; }
    const std::string& getTargetName() const { return target_name_; }
    const std::deque<EventEntry>& getRecentEvents() const { return recent_events_; }
    
    void create(lv_obj_t* parent);
    void updateConnection(lv_point_t start, lv_point_t end);
    void addEvent(const std::string& event_name, const std::string& data, uint32_t color = 0x00FF88);
    void animateEventFlow();
    
    uint64_t getCurrentTimeMicros() const;
};

/**
 * @brief Main event flow visualizer panel
 */
class EventFlowVisualizer {
private:
    lv_obj_t* main_container_;
    lv_obj_t* esp32_area_;          // ESP32-sized area for your existing app
    lv_obj_t* visualizer_area_;     // Remaining area for event flow graph
    lv_obj_t* graph_container_;
    lv_obj_t* controls_panel_;
    
    // Graph components
    std::unordered_map<std::string, std::unique_ptr<EventFlowNode>> nodes_;
    std::vector<std::unique_ptr<EventFlowConnection>> connections_;
    
    // Control components
    lv_obj_t* filter_dropdown_;
    lv_obj_t* clear_btn_;
    lv_obj_t* pause_btn_;
    lv_obj_t* stats_label_;
    
    bool paused_;
    uint64_t total_events_;
    
public:
    EventFlowVisualizer();
    ~EventFlowVisualizer();
    
    bool initialize(lv_obj_t* parent);
    void shutdown();
    
    // Node management
    void addNode(const std::string& name, lv_coord_t x = 0, lv_coord_t y = 0);
    void removeNode(const std::string& name);
    EventFlowNode* getNode(const std::string& name);
    
    // Connection management  
    void addConnection(const std::string& source, const std::string& target);
    void removeConnection(const std::string& source, const std::string& target);
    
    // Event handling
    void traceEvent(const std::string& source, const std::string& target, 
                   const std::string& event_name, const std::string& data = "");
    
    // Layout management
    void autoLayoutNodes();
    void clearAllEvents();
    void setPaused(bool paused);
    
    // Access to ESP32 area for main app
    lv_obj_t* getESP32Area() const { return esp32_area_; }
    
private:
    void createLayout(lv_obj_t* parent);
    void createControlsPanel();
    void updateStatistics();
    
    // Event handlers
    static void onClearClicked(lv_event_t* e);
    static void onPauseClicked(lv_event_t* e);
    static void onFilterChanged(lv_event_t* e);
};

} // namespace Debug

#endif // DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER
