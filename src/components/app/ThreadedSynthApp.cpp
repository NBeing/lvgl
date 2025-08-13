#include "ThreadedSynthApp.h"

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)

void ThreadedSynthApp::createDesktopWithVisualizerLayout() {
    std::cout << "[ThreadedSynthApp] Creating desktop layout with event visualizer" << std::endl;
    
    // Create main container that spans the full desktop window
    lv_obj_t* main_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(main_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(main_container, 10, 0);
    lv_obj_set_style_bg_color(main_container, lv_color_hex(0x1a1a1a), 0);
    
    // Create horizontal layout with two panels
    lv_obj_set_layout(main_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(main_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(main_container, 20, 0);
    
    // Left panel: ESP32-sized app container (480x320)
    lv_obj_t* app_container = lv_obj_create(main_container);
    lv_obj_set_size(app_container, SynthConstants::ESP32_SCREEN_WIDTH, SynthConstants::ESP32_SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(app_container, lv_color_hex(SynthConstants::Color::BG), 0);
    lv_obj_set_style_border_color(app_container, lv_color_hex(0xFF333333), 0);
    lv_obj_set_style_border_width(app_container, 2, 0);
    lv_obj_set_style_radius(app_container, 8, 0);
    lv_obj_set_style_pad_all(app_container, 0, 0);
    
    // Right panel: Event visualizer (takes remaining space)
    lv_obj_t* visualizer_container = lv_obj_create(main_container);
    lv_obj_set_flex_grow(visualizer_container, 1);  // Take remaining space
    lv_obj_set_height(visualizer_container, LV_PCT(100));
    lv_obj_set_style_bg_color(visualizer_container, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_color(visualizer_container, lv_color_hex(0xFF444444), 0);
    lv_obj_set_style_border_width(visualizer_container, 1, 0);
    lv_obj_set_style_radius(visualizer_container, 8, 0);
    lv_obj_set_style_pad_all(visualizer_container, 10, 0);
    
    // Add title to event visualizer panel
    lv_obj_t* title = lv_label_create(visualizer_container);
    lv_label_set_text(title, "  Real-Time Event Flow Visualizer");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    
    std::cout << "[ThreadedSynthApp] Created dual-panel layout:" << std::endl;
    std::cout << "  - ESP32 App: " << SynthConstants::ESP32_SCREEN_WIDTH << "x" << SynthConstants::ESP32_SCREEN_HEIGHT << std::endl;
    std::cout << "  - Event Visualizer: Flexible width, full height" << std::endl;
    
    // Create window manager with the ESP32-sized container
    window_manager_ = std::make_unique<WindowManager>(app_container);
    
    // Store visualizer container for later use
    visualizer_container_ = visualizer_container;
    
    // Initialize event visualizer in its container
    initializeEventVisualizer();
}

void ThreadedSynthApp::initializeEventVisualizer() {
    std::cout << "[ThreadedSynthApp] Initializing event visualizer system" << std::endl;
    
    // Initialize the event visualizer manager
    auto* manager = Debug::EventVisualizerManager::getInstance();
    if (manager) {
        if (manager->initialize(visualizer_container_)) {
            std::cout << "[ThreadedSynthApp] ✅ Event visualizer initialized successfully" << std::endl;
        } else {
            std::cout << "[ThreadedSynthApp] ❌ Failed to initialize event visualizer" << std::endl;
        }
    } else {
        std::cout << "[ThreadedSynthApp] ❌ Failed to get event visualizer manager" << std::endl;
    }
}

#endif // DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER
