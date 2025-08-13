#include "ExampleEventVisualizerApp.h"
#include "EventVisualizerManager.h"

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)

#include "components/debug/RTEventTracer.h"
#include <iostream>

namespace Debug {

ExampleEventVisualizerApp::ExampleEventVisualizerApp()
    : main_screen_(nullptr)
    , visualizer_initialized_(false) {
}

ExampleEventVisualizerApp::~ExampleEventVisualizerApp() {
    shutdown();
}

bool ExampleEventVisualizerApp::initialize(lv_obj_t* parent) {
    if (visualizer_initialized_) {
        return true;
    }
    
    // Create main screen
    main_screen_ = lv_obj_create(parent);
    lv_obj_set_size(main_screen_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(main_screen_, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(main_screen_, 0, 0);
    lv_obj_set_style_pad_all(main_screen_, 0, 0);
    
    // Initialize event visualizer
    auto* manager = Debug::EventVisualizerManager::getInstance();
    if (!manager || !manager->initialize(main_screen_)) {
        std::cerr << "[ExampleApp] Failed to initialize event visualizer" << std::endl;
        return false;
    }
    
    visualizer_initialized_ = true;
    
    // Create some demo events to show the system working
    createExampleEvents();
    
    std::cout << "[ExampleApp] Initialized with event visualizer" << std::endl;
    std::cout << "[ExampleApp] ESP32 area available at: " << getESP32Area() << std::endl;
    
    return true;
}

lv_obj_t* ExampleEventVisualizerApp::getESP32Area() {
    // Return the main screen since our new manager doesn't create separate ESP32 areas
    return main_screen_;
}

void ExampleEventVisualizerApp::shutdown() {
    if (visualizer_initialized_) {
        auto* manager = Debug::EventVisualizerManager::getInstance();
        if (manager) {
            // Note: We don't shutdown the manager as it might be used elsewhere
        }
        visualizer_initialized_ = false;
    }
    
    if (main_screen_) {
        lv_obj_del(main_screen_);
        main_screen_ = nullptr;
    }
    
    std::cout << "[ExampleApp] Shutdown completed" << std::endl;
}

bool ExampleEventVisualizerApp::isVisualizerAvailable() const {
    auto* manager = Debug::EventVisualizerManager::getInstance();
    return visualizer_initialized_ && manager && manager->isInitialized();
}

void ExampleEventVisualizerApp::createExampleEvents() {
    // Create a timer that generates demo events to show the visualizer working
    lv_timer_t* demo_timer = lv_timer_create(demoTimerCallback, 2000, this);
    lv_timer_set_repeat_count(demo_timer, 10); // Run 10 times then stop
}

void ExampleEventVisualizerApp::demoTimerCallback(lv_timer_t* timer) {
    // auto* app = static_cast<ExampleEventVisualizerApp*>(lv_timer_get_user_data(timer));
    (void)timer; // Suppress unused parameter warning
    
    static int demo_counter = 0;
    demo_counter++;
    
    // Generate various types of demo events - fix template issues
    std::string tick_data = std::to_string(demo_counter * 24);
    TRACE_CLOCK_EVENT("MidiClockManager", "ClockTab", "onClockTick", tick_data.c_str());
    TRACE_UI_EVENT("ClockTab", "TransportControl", "updateTransportState", "playing");
    TRACE_PARAMETER_EVENT("ParameterManager", "MidiBridge", "parameterChange", "filter_cutoff");
    TRACE_MIDI_EVENT("MidiBridge", "ExternalSynth", "sendCC", "74:95");
    TRACE_SETTINGS_EVENT("SettingsManager", "ClockTab", "settingChanged", "midi.ppqn");
    
    // Simulate user interactions
    if (demo_counter % 3 == 0) {
        TRACE_UI_EVENT("User", "ClockTab", "tempoButtonClicked", "tempo_up");
        TRACE_UI_EVENT("ClockTab", "MidiClockManager", "setBPM", std::to_string(120 + demo_counter));
    }
    
    if (demo_counter % 5 == 0) {
        TRACE_UI_EVENT("User", "ClockTab", "midiTestClicked", "test_sequence");
        TRACE_MIDI_EVENT("HardwareMidiManager", "ExternalSynth", "sendNoteOn", "60:100");
        TRACE_MIDI_EVENT("HardwareMidiManager", "ExternalSynth", "sendNoteOff", "60:0");
    }
    
    std::cout << "[ExampleApp] Generated demo events batch " << demo_counter << std::endl;
}

} // namespace Debug

#endif // DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER
