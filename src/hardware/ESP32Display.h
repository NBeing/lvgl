// src/hardware/ESP32Display.h - Modular ESP32 display and touch driver
#pragma once

#include <lvgl.h>
#include "./LGFX_ST7796S.h"

class ESP32Display {
public:
    ESP32Display();
    ~ESP32Display();
    
    // Main initialization - call this once
    bool initialize();
    
    // Get the display instance (for LVGL)
    lv_display_t* getDisplay() { return display_; }
    
    // Get the touch input device (for LVGL)
    lv_indev_t* getTouchInput() { return touch_indev_; }
    
    // Display info
    int getWidth() const { return tft_.width(); }
    int getHeight() const { return tft_.height(); }
    
    // Enable/disable touch debug output
    void setTouchDebug(bool enabled) { touch_debug_ = enabled; }
    
    // Calibration constants (can be overridden for different hardware)
    struct TouchCalibration {
        int min_x = 37;   // Right edge (smallest X)
        int max_x = 452;  // Left edge (largest X)
        int min_y = 20;   // Top edge (smallest Y)
        int max_y = 292;  // Bottom edge (largest Y)
    };
    
    void setTouchCalibration(const TouchCalibration& cal) { touch_cal_ = cal; }

private:
    LGFX_ST7796S tft_;
    lv_display_t* display_;
    lv_indev_t* touch_indev_;
    lv_color_t* display_buffer_;
    TouchCalibration touch_cal_;
    bool touch_debug_;
    
    void setupDisplay();
    void setupTouch();
    static void flushCallback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
    static void touchCallback(lv_indev_t* indev, lv_indev_data_t* data);
    
    // Static instance for callbacks
    static ESP32Display* instance_;
};