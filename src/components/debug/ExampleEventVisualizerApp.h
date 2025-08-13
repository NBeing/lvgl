#pragma once

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)

#include "components/debug/EventVisualizerIntegration.h"
#include <lvgl.h>

namespace Debug {

/**
 * @brief Example integration showing how to use the event visualizer 
 * in your main application
 */
class ExampleEventVisualizerApp {
private:
    lv_obj_t* main_screen_;
    bool visualizer_initialized_;
    
public:
    ExampleEventVisualizerApp();
    ~ExampleEventVisualizerApp();
    
    /**
     * @brief Initialize the application with event visualizer
     * @param parent Parent object for the main screen
     * @return true if successful
     */
    bool initialize(lv_obj_t* parent);
    
    /**
     * @brief Get the ESP32 area where your normal app should be created
     * @return ESP32-sized container for your app, or nullptr if not initialized
     */
    lv_obj_t* getESP32Area();
    
    /**
     * @brief Shutdown the application
     */
    void shutdown();
    
    /**
     * @brief Check if visualizer is available
     */
    bool isVisualizerAvailable() const;
    
private:
    void createExampleEvents();
    static void demoTimerCallback(lv_timer_t* timer);
};

} // namespace Debug

#endif // DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER
