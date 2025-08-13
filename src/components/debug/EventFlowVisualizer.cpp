#include "EventFlowVisualizer.h"

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)

#include "FontConfig.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace Debug {

constexpr lv_coord_t ESP32_SCREEN_WIDTH = 480;
constexpr lv_coord_t ESP32_SCREEN_HEIGHT = 320;
constexpr lv_coord_t NODE_WIDTH = 140;
constexpr lv_coord_t NODE_HEIGHT = 80;
constexpr lv_coord_t NODE_SPACING = 180;

// ============================================================================
// EventFlowNode Implementation
// ============================================================================

EventFlowNode::EventFlowNode(const std::string& name, lv_coord_t x, lv_coord_t y)
    : node_container_(nullptr)
    , node_title_(nullptr)
    , node_status_(nullptr) {
    
    info_.name = name;
    info_.x = x;
    info_.y = y;
    info_.width = NODE_WIDTH;
    info_.height = NODE_HEIGHT;
    info_.color = 0x2E2E3E;
    info_.active = false;
    info_.input_count = 0;
    info_.output_count = 0;
}

EventFlowNode::~EventFlowNode() {
    if (node_container_) {
        lv_obj_del(node_container_);
    }
}

void EventFlowNode::create(lv_obj_t* parent) {
    // Create main node container
    node_container_ = lv_obj_create(parent);
    lv_obj_set_size(node_container_, info_.width, info_.height);
    lv_obj_set_pos(node_container_, info_.x, info_.y);
    
    // Node styling
    lv_obj_set_style_bg_color(node_container_, lv_color_hex(info_.color), 0);
    lv_obj_set_style_border_color(node_container_, lv_color_hex(0x6C7086), 0);
    lv_obj_set_style_border_width(node_container_, 2, 0);
    lv_obj_set_style_radius(node_container_, 8, 0);
    lv_obj_set_style_pad_all(node_container_, 5, 0);
    
    // Node title
    node_title_ = lv_label_create(node_container_);
    lv_label_set_text(node_title_, info_.name.c_str());
    lv_obj_set_style_text_color(node_title_, lv_color_hex(0xCDD6F4), 0);
    lv_obj_set_style_text_font(node_title_, FontA.small, 0);
    lv_obj_align(node_title_, LV_ALIGN_TOP_MID, 0, 5);
    
    // Node status
    node_status_ = lv_label_create(node_container_);
    lv_label_set_text(node_status_, "Ready");
    lv_obj_set_style_text_color(node_status_, lv_color_hex(0x94E2D5), 0);
    lv_obj_set_style_text_font(node_status_, FontA.small, 0);
    lv_obj_align(node_status_, LV_ALIGN_BOTTOM_MID, 0, -5);
}

void EventFlowNode::setPosition(lv_coord_t x, lv_coord_t y) {
    info_.x = x;
    info_.y = y;
    if (node_container_) {
        lv_obj_set_pos(node_container_, x, y);
    }
}

void EventFlowNode::setActive(bool active) {
    info_.active = active;
    if (node_container_) {
        uint32_t color = active ? 0x1E3A5F : info_.color;
        lv_obj_set_style_bg_color(node_container_, lv_color_hex(color), 0);
        
        uint32_t border_color = active ? 0x00FF88 : 0x6C7086;
        lv_obj_set_style_border_color(node_container_, lv_color_hex(border_color), 0);
    }
}

void EventFlowNode::updateStatus(const std::string& status) {
    if (node_status_) {
        lv_label_set_text(node_status_, status.c_str());
    }
}

lv_point_t EventFlowNode::getInputPoint() const {
    lv_point_t point;
    point.x = info_.x;
    point.y = info_.y + info_.height / 2;
    return point;
}

lv_point_t EventFlowNode::getOutputPoint() const {
    lv_point_t point;
    point.x = info_.x + info_.width;
    point.y = info_.y + info_.height / 2;
    return point;
}

// ============================================================================
// EventFlowConnection Implementation
// ============================================================================

EventFlowConnection::EventFlowConnection(const std::string& source, const std::string& target)
    : connection_line_(nullptr)
    , event_log_container_(nullptr)
    , source_name_(source)
    , target_name_(target) {
    
    for (int i = 0; i < 5; i++) {
        event_labels_[i] = nullptr;
    }
}

EventFlowConnection::~EventFlowConnection() {
    if (connection_line_) {
        lv_obj_del(connection_line_);
    }
    if (event_log_container_) {
        lv_obj_del(event_log_container_);
    }
}

void EventFlowConnection::create(lv_obj_t* parent) {
    // Create connection line
    connection_line_ = lv_line_create(parent);
    lv_obj_set_style_line_width(connection_line_, 3, 0);
    lv_obj_set_style_line_color(connection_line_, lv_color_hex(0x6C7086), 0);
    
    // Create event log container (positioned near line midpoint)
    event_log_container_ = lv_obj_create(parent);
    lv_obj_set_size(event_log_container_, 200, 120);
    lv_obj_set_style_bg_color(event_log_container_, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_color(event_log_container_, lv_color_hex(0x16213E), 0);
    lv_obj_set_style_border_width(event_log_container_, 1, 0);
    lv_obj_set_style_radius(event_log_container_, 4, 0);
    lv_obj_set_style_pad_all(event_log_container_, 3, 0);
    lv_obj_set_style_bg_opa(event_log_container_, LV_OPA_90, 0);
    
    // Create event labels
    for (int i = 0; i < 5; i++) {
        event_labels_[i] = lv_label_create(event_log_container_);
        lv_label_set_text(event_labels_[i], "");
        lv_obj_set_style_text_color(event_labels_[i], lv_color_hex(0xCDD6F4), 0);
        lv_obj_set_style_text_font(event_labels_[i], FontA.small, 0);
        lv_obj_set_pos(event_labels_[i], 5, 5 + i * 20);
    }
}

void EventFlowConnection::updateConnection(lv_point_t start, lv_point_t end) {
    start_point_ = start;
    end_point_ = end;
    
    if (connection_line_) {
        static lv_point_precise_t line_points[2];
        line_points[0] = {start.x, start.y};
        line_points[1] = {end.x, end.y};
        lv_line_set_points(connection_line_, line_points, 2);
        
        // Position event log at midpoint
        lv_coord_t mid_x = (start.x + end.x) / 2 - 100;  // Center the 200px wide container
        lv_coord_t mid_y = (start.y + end.y) / 2 - 60;   // Center the 120px high container
        lv_obj_set_pos(event_log_container_, mid_x, mid_y);
    }
}

void EventFlowConnection::addEvent(const std::string& event_name, const std::string& data, uint32_t color) {
    EventEntry entry;
    entry.event_name = event_name;
    entry.event_data = data;
    entry.timestamp_us = getCurrentTimeMicros();
    entry.color = color;
    
    recent_events_.push_back(entry);
    if (recent_events_.size() > 5) {
        recent_events_.pop_front();
    }
    
    updateEventDisplay();
    animateEventFlow();
}

void EventFlowConnection::animateEventFlow() {
    if (!connection_line_) return;
    
    // Create animated pulse along the connection
    lv_obj_t* pulse_obj = lv_obj_create(lv_obj_get_parent(connection_line_));
    lv_obj_set_size(pulse_obj, 8, 8);
    lv_obj_set_style_bg_color(pulse_obj, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_radius(pulse_obj, 4, 0);
    lv_obj_set_style_border_width(pulse_obj, 0, 0);
    lv_obj_set_pos(pulse_obj, start_point_.x - 4, start_point_.y - 4);
    
    // Animate pulse from start to end
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, pulse_obj);
    lv_anim_set_time(&anim, 800);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
    
    // X animation
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t val) {
        lv_obj_set_x((lv_obj_t*)obj, val);
    });
    lv_anim_set_values(&anim, start_point_.x - 4, end_point_.x - 4);
    lv_anim_start(&anim);
    
    // Y animation
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t val) {
        lv_obj_set_y((lv_obj_t*)obj, val);
    });
    lv_anim_set_values(&anim, start_point_.y - 4, end_point_.y - 4);
    // Skip animation completion callback for now - just let object exist
    lv_anim_start(&anim);
    
    // Make connection line briefly brighter
    lv_obj_set_style_line_color(connection_line_, lv_color_hex(0x00FF88), 0);
    
    // Fade back to normal color
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, connection_line_);
    lv_anim_set_time(&anim, 800);
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t val) {
        // Interpolate color from bright green back to normal
        uint32_t color = val < 400 ? 0x00FF88 : 0x6C7086;
        lv_obj_set_style_line_color((lv_obj_t*)obj, lv_color_hex(color), 0);
    });
    lv_anim_set_values(&anim, 0, 800);
    lv_anim_start(&anim);
}

void EventFlowConnection::updateEventDisplay() {
    for (int i = 0; i < 5; i++) {
        if (i < static_cast<int>(recent_events_.size())) {
            const auto& event = recent_events_[recent_events_.size() - 1 - i];
            auto now = getCurrentTimeMicros();
            auto age_ms = (now - event.timestamp_us) / 1000;
            
            std::string text = event.event_name;
            if (!event.event_data.empty()) {
                text += "(" + event.event_data + ")";
            }
            text += " [" + std::to_string(age_ms) + "ms]";
            
            lv_label_set_text(event_labels_[i], text.c_str());
            lv_obj_set_style_text_color(event_labels_[i], lv_color_hex(event.color), 0);
        } else {
            lv_label_set_text(event_labels_[i], "");
        }
    }
}

uint64_t EventFlowConnection::getCurrentTimeMicros() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

// ============================================================================
// EventFlowVisualizer Implementation
// ============================================================================

EventFlowVisualizer::EventFlowVisualizer()
    : main_container_(nullptr)
    , esp32_area_(nullptr)
    , visualizer_area_(nullptr)
    , graph_container_(nullptr)
    , controls_panel_(nullptr)
    , filter_dropdown_(nullptr)
    , clear_btn_(nullptr)
    , pause_btn_(nullptr)
    , stats_label_(nullptr)
    , paused_(false)
    , total_events_(0) {
}

EventFlowVisualizer::~EventFlowVisualizer() {
    shutdown();
}

bool EventFlowVisualizer::initialize(lv_obj_t* parent) {
    createLayout(parent);
    createControlsPanel();
    
    std::cout << "[EventFlowVisualizer] Initialized - Desktop debug mode active" << std::endl;
    return true;
}

void EventFlowVisualizer::shutdown() {
    nodes_.clear();
    connections_.clear();
    
    if (main_container_) {
        lv_obj_del(main_container_);
        main_container_ = nullptr;
    }
}

void EventFlowVisualizer::createLayout(lv_obj_t* parent) {
    // Create main container that fills the parent
    main_container_ = lv_obj_create(parent);
    lv_obj_set_size(main_container_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(main_container_, lv_color_hex(0x0F0F1A), 0);
    lv_obj_set_style_border_width(main_container_, 0, 0);
    lv_obj_set_style_pad_all(main_container_, 0, 0);
    
    // Create ESP32-sized area (left side) for your existing app
    esp32_area_ = lv_obj_create(main_container_);
    lv_obj_set_size(esp32_area_, ESP32_SCREEN_WIDTH, ESP32_SCREEN_HEIGHT);
    lv_obj_align(esp32_area_, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(esp32_area_, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_color(esp32_area_, lv_color_hex(0x6C7086), 0);
    lv_obj_set_style_border_width(esp32_area_, 2, 0);
    lv_obj_set_style_radius(esp32_area_, 8, 0);
    
    // Create title for ESP32 area
    lv_obj_t* esp32_title = lv_label_create(esp32_area_);
    lv_label_set_text(esp32_title, "ESP32 Display Area");
    lv_obj_set_style_text_color(esp32_title, lv_color_hex(0xCDD6F4), 0);
    lv_obj_set_style_text_font(esp32_title, FontA.small, 0);
    lv_obj_align(esp32_title, LV_ALIGN_TOP_MID, 0, 5);
    
    // Create visualizer area (right side) for the event flow graph
    visualizer_area_ = lv_obj_create(main_container_);
    lv_obj_set_pos(visualizer_area_, ESP32_SCREEN_WIDTH + 30, 10);
    lv_obj_set_size(visualizer_area_, LV_PCT(100) - ESP32_SCREEN_WIDTH - 40, LV_PCT(100) - 20);
    lv_obj_set_style_bg_color(visualizer_area_, lv_color_hex(0x16213E), 0);
    lv_obj_set_style_border_color(visualizer_area_, lv_color_hex(0x6C7086), 0);
    lv_obj_set_style_border_width(visualizer_area_, 2, 0);
    lv_obj_set_style_radius(visualizer_area_, 8, 0);
    lv_obj_set_style_pad_all(visualizer_area_, 10, 0);
    
    // Create title for visualizer area
    lv_obj_t* viz_title = lv_label_create(visualizer_area_);
    lv_label_set_text(viz_title, "Event Flow Visualizer");
    lv_obj_set_style_text_color(viz_title, lv_color_hex(0xCDD6F4), 0);
    lv_obj_set_style_text_font(viz_title, FontA.med, 0);
    lv_obj_align(viz_title, LV_ALIGN_TOP_MID, 0, 5);
    
    // Create graph container (main area for nodes and connections)
    graph_container_ = lv_obj_create(visualizer_area_);
    lv_obj_set_pos(graph_container_, 0, 40);
    lv_obj_set_size(graph_container_, LV_PCT(100), LV_PCT(100) - 100);
    lv_obj_set_style_bg_opa(graph_container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(graph_container_, 0, 0);
    lv_obj_set_style_pad_all(graph_container_, 5, 0);
}

void EventFlowVisualizer::createControlsPanel() {
    // Create controls panel at bottom of visualizer area
    controls_panel_ = lv_obj_create(visualizer_area_);
    lv_obj_align(controls_panel_, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_size(controls_panel_, LV_PCT(100), 50);
    lv_obj_set_style_bg_color(controls_panel_, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_color(controls_panel_, lv_color_hex(0x6C7086), 0);
    lv_obj_set_style_border_width(controls_panel_, 1, 0);
    lv_obj_set_style_radius(controls_panel_, 4, 0);
    lv_obj_set_style_pad_all(controls_panel_, 5, 0);
    
    // Set up layout
    lv_obj_set_layout(controls_panel_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(controls_panel_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(controls_panel_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Clear button
    clear_btn_ = lv_btn_create(controls_panel_);
    lv_obj_set_size(clear_btn_, 80, 35);
    lv_obj_set_style_bg_color(clear_btn_, lv_color_hex(0xFF6B35), 0);
    lv_obj_add_event_cb(clear_btn_, onClearClicked, LV_EVENT_CLICKED, this);
    
    lv_obj_t* clear_label = lv_label_create(clear_btn_);
    lv_label_set_text(clear_label, "Clear");
    lv_obj_center(clear_label);
    
    // Pause button
    pause_btn_ = lv_btn_create(controls_panel_);
    lv_obj_set_size(pause_btn_, 80, 35);
    lv_obj_set_style_bg_color(pause_btn_, lv_color_hex(0x0066CC), 0);
    lv_obj_add_event_cb(pause_btn_, onPauseClicked, LV_EVENT_CLICKED, this);
    
    lv_obj_t* pause_label = lv_label_create(pause_btn_);
    lv_label_set_text(pause_label, "Pause");
    lv_obj_center(pause_label);
    
    // Filter dropdown
    filter_dropdown_ = lv_dropdown_create(controls_panel_);
    lv_dropdown_set_options(filter_dropdown_, "All Events\nClock Events\nMIDI Events\nParameter Events\nUI Events");
    lv_obj_set_size(filter_dropdown_, 120, LV_SIZE_CONTENT);
    lv_obj_add_event_cb(filter_dropdown_, onFilterChanged, LV_EVENT_VALUE_CHANGED, this);
    
    // Statistics label
    stats_label_ = lv_label_create(controls_panel_);
    lv_label_set_text(stats_label_, "Events: 0");
    lv_obj_set_style_text_color(stats_label_, lv_color_hex(0x94E2D5), 0);
    lv_obj_set_style_text_font(stats_label_, FontA.small, 0);
}

void EventFlowVisualizer::addNode(const std::string& name, lv_coord_t x, lv_coord_t y) {
    if (nodes_.find(name) != nodes_.end()) {
        return; // Node already exists
    }
    
    auto node = std::make_unique<EventFlowNode>(name, x, y);
    node->create(graph_container_);
    nodes_[name] = std::move(node);
    
    std::cout << "[EventFlowVisualizer] Added node: " << name << std::endl;
}

void EventFlowVisualizer::removeNode(const std::string& name) {
    auto it = nodes_.find(name);
    if (it != nodes_.end()) {
        nodes_.erase(it);
        std::cout << "[EventFlowVisualizer] Removed node: " << name << std::endl;
    }
}

EventFlowNode* EventFlowVisualizer::getNode(const std::string& name) {
    auto it = nodes_.find(name);
    return (it != nodes_.end()) ? it->second.get() : nullptr;
}

void EventFlowVisualizer::addConnection(const std::string& source, const std::string& target) {
    // Check if connection already exists
    for (const auto& conn : connections_) {
        if (conn->getSourceName() == source && conn->getTargetName() == target) {
            return;
        }
    }
    
    auto connection = std::make_unique<EventFlowConnection>(source, target);
    connection->create(graph_container_);
    
    // Update connection line if both nodes exist
    auto source_node = getNode(source);
    auto target_node = getNode(target);
    if (source_node && target_node) {
        connection->updateConnection(source_node->getOutputPoint(), target_node->getInputPoint());
    }
    
    connections_.push_back(std::move(connection));
    std::cout << "[EventFlowVisualizer] Added connection: " << source << " --> " << target << std::endl;
}

void EventFlowVisualizer::traceEvent(const std::string& source, const std::string& target, 
                                   const std::string& event_name, const std::string& data) {
    if (paused_) return;
    
    // Ensure nodes exist
    if (!getNode(source)) {
        addNode(source);
        autoLayoutNodes();
    }
    if (!getNode(target)) {
        addNode(target);
        autoLayoutNodes();
    }
    
    // Ensure connection exists
    addConnection(source, target);
    
    // Find and update the connection
    for (auto& conn : connections_) {
        if (conn->getSourceName() == source && conn->getTargetName() == target) {
            conn->addEvent(event_name, data);
            break;
        }
    }
    
    // Update node activity
    auto source_node = getNode(source);
    auto target_node = getNode(target);
    if (source_node) source_node->setActive(true);
    if (target_node) target_node->setActive(true);
    
    // Reset activity after short delay
    static lv_timer_t* activity_timer = nullptr;
    if (activity_timer) {
        lv_timer_del(activity_timer);
    }
    activity_timer = lv_timer_create([](lv_timer_t* timer) {
        auto* visualizer = static_cast<EventFlowVisualizer*>(lv_timer_get_user_data(timer));
        for (auto& [name, node] : visualizer->nodes_) {
            node->setActive(false);
        }
        lv_timer_del(timer);
    }, 500, this);
    
    total_events_++;
    updateStatistics();
}

void EventFlowVisualizer::autoLayoutNodes() {
    int nodes_per_row = 3;
    int current_row = 0;
    int current_col = 0;
    
    for (auto& [name, node] : nodes_) {
        lv_coord_t x = current_col * NODE_SPACING + 20;
        lv_coord_t y = current_row * (NODE_HEIGHT + 40) + 20;
        
        node->setPosition(x, y);
        
        current_col++;
        if (current_col >= nodes_per_row) {
            current_col = 0;
            current_row++;
        }
    }
    
    // Update all connection lines
    for (auto& conn : connections_) {
        auto source_node = getNode(conn->getSourceName());
        auto target_node = getNode(conn->getTargetName());
        if (source_node && target_node) {
            conn->updateConnection(source_node->getOutputPoint(), target_node->getInputPoint());
        }
    }
}

void EventFlowVisualizer::clearAllEvents() {
    for (auto& conn : connections_) {
        // Access private members through friend class relationship
        conn->recent_events_.clear();
        conn->updateEventDisplay();
    }
    total_events_ = 0;
    updateStatistics();
}

void EventFlowVisualizer::setPaused(bool paused) {
    paused_ = paused;
    if (pause_btn_) {
        lv_obj_t* label = lv_obj_get_child(pause_btn_, 0);
        lv_label_set_text(label, paused ? "Resume" : "Pause");
    }
}

void EventFlowVisualizer::updateStatistics() {
    if (stats_label_) {
        lv_label_set_text_fmt(stats_label_, "Events: %lu | Nodes: %zu | Connections: %zu", 
                             (unsigned long)total_events_, nodes_.size(), connections_.size());
    }
}

// Static event handlers
void EventFlowVisualizer::onClearClicked(lv_event_t* e) {
    auto* visualizer = static_cast<EventFlowVisualizer*>(lv_event_get_user_data(e));
    visualizer->clearAllEvents();
}

void EventFlowVisualizer::onPauseClicked(lv_event_t* e) {
    auto* visualizer = static_cast<EventFlowVisualizer*>(lv_event_get_user_data(e));
    visualizer->setPaused(!visualizer->paused_);
}

void EventFlowVisualizer::onFilterChanged(lv_event_t* e) {
    // TODO: Implement event filtering
    std::cout << "[EventFlowVisualizer] Filter changed" << std::endl;
}

} // namespace Debug

#endif // DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER
