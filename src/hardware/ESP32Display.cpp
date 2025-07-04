// src/hardware/ESP32Display.cpp
#include "ESP32Display.h"
#include <iostream>
#include <algorithm>

// Static instance for callbacks
ESP32Display* ESP32Display::instance_ = nullptr;

ESP32Display::ESP32Display() 
    : display_(nullptr)
    , touch_indev_(nullptr) 
    , display_buffer_(nullptr)
    , touch_debug_(false) {
    instance_ = this;
}

ESP32Display::~ESP32Display() {
    if (display_buffer_) {
        heap_caps_free(display_buffer_);
    }
    instance_ = nullptr;
}

bool ESP32Display::initialize() {
    std::cout << "=== ESP32Display Initialization ===" << std::endl;
    
    // Initialize hardware
    tft_.begin();
    tft_.setRotation(1);
    
    std::cout << "Display info:" << std::endl;
    std::cout << "  Width: " << tft_.width() << std::endl;
    std::cout << "  Height: " << tft_.height() << std::endl;
    std::cout << "  Rotation: " << tft_.getRotation() << std::endl;
    
    setupDisplay();
    setupTouch();
    
    std::cout << "ESP32Display initialization complete!" << std::endl;
    return true;
}

void ESP32Display::setupDisplay() {
    // Create display buffer
    size_t buffer_pixels = tft_.width() * 40;
    display_buffer_ = (lv_color_t*)heap_caps_malloc(
        buffer_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA);
    
    if (!display_buffer_) {
        std::cerr << "Failed to allocate display buffer!" << std::endl;
        return;
    }
    
    // Create LVGL display
    display_ = lv_display_create(tft_.width(), tft_.height());
    lv_display_set_flush_cb(display_, flushCallback);
    lv_display_set_buffers(display_, display_buffer_, NULL, 
        buffer_pixels * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
}

void ESP32Display::setupTouch() {
    // Create touch input device
    touch_indev_ = lv_indev_create();
    lv_indev_set_type(touch_indev_, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev_, touchCallback);
    
    std::cout << "Touch panel configured with calibration:" << std::endl;
    std::cout << "  X range: " << touch_cal_.min_x << " - " << touch_cal_.max_x << std::endl;
    std::cout << "  Y range: " << touch_cal_.min_y << " - " << touch_cal_.max_y << std::endl;
}

void ESP32Display::flushCallback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    if (!instance_) return;
    
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    
    instance_->tft_.startWrite();
    instance_->tft_.setAddrWindow(area->x1, area->y1, w, h);
    instance_->tft_.pushPixels((uint16_t *)px_map, w * h, true);
    instance_->tft_.endWrite();
    
    lv_display_flush_ready(disp);
}

void ESP32Display::touchCallback(lv_indev_t* indev, lv_indev_data_t* data) {
    if (!instance_) return;
    
    uint16_t raw_x, raw_y;
    uint_fast8_t touch_count = instance_->tft_.getTouch(&raw_x, &raw_y);
    
    if (touch_count > 0) {
        // Use calibrated touch mapping
        const auto& cal = instance_->touch_cal_;
        
        // Map coordinates
        int mapped_x = ((cal.max_x - raw_x) * instance_->getWidth()) / (cal.max_x - cal.min_x);
        int mapped_y = ((raw_y - cal.min_y) * instance_->getHeight()) / (cal.max_y - cal.min_y);
        
        // Clamp to screen bounds
        mapped_x = std::max(0, std::min(mapped_x, instance_->getWidth() - 1));
        mapped_y = std::max(0, std::min(mapped_y, instance_->getHeight() - 1));
        
        data->point.x = mapped_x;
        data->point.y = mapped_y;
        data->state = LV_INDEV_STATE_PRESSED;
        
        // Optional debug output
        if (instance_->touch_debug_) {
            std::cout << "Touch: raw(" << raw_x << "," << raw_y 
                      << ") → mapped(" << mapped_x << "," << mapped_y << ")" << std::endl;
        }
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}