#include "LayoutManager.h"
#include <algorithm>
#include <iostream>

// Static member initialization
LayoutManager::ScreenSize LayoutManager::screen_size_ = ScreenSize::MEDIUM;
LayoutManager::LayoutConfig LayoutManager::current_config_ = {};
bool LayoutManager::initialized_ = false;

void LayoutManager::initialize() {
    if (initialized_) return;
    
    // Get actual screen dimensions
    int screen_width = lv_display_get_horizontal_resolution(lv_display_get_default());
    int screen_height = lv_display_get_vertical_resolution(lv_display_get_default());
    
    screen_size_ = classifyScreen(screen_width, screen_height);
    current_config_ = createConfigForScreen(screen_size_);
    // Dynamically adjust dial_spacing_x and margin_x for small screens
    if (screen_size_ == ScreenSize::SMALL) {
        int grid_cols = current_config_.grid_cols;
        int dial_size = current_config_.dial_size + 20; // match SynthApp dial container size
        int total_dial_width = grid_cols * dial_size;
        int available_width = screen_width - 2 * current_config_.margin_x;
        int spacing = 0;
        if (grid_cols > 1) {
            spacing = (available_width - total_dial_width) / (grid_cols - 1);
            if (spacing < 10) spacing = 10;
        }
        int used_width = grid_cols * dial_size + (grid_cols - 1) * spacing;
        int margin_x = (screen_width - used_width) / 2;
        if (margin_x < 0) margin_x = 0;
        current_config_.dial_spacing_x = dial_size + spacing;
        current_config_.margin_x = margin_x;
        std::cout << "[LayoutManager] SMALL: dial_size=" << dial_size << ", spacing=" << spacing << ", margin_x=" << margin_x << std::endl;
    }
    initialized_ = true;

    std::cout << "LayoutManager: Screen " << screen_width << "x" << screen_height 
              << " classified as " << ((screen_size_ == ScreenSize::SMALL) ? "SMALL" :
                                      (screen_size_ == ScreenSize::MEDIUM) ? "MEDIUM" : "LARGE")
              << std::endl;
}

LayoutManager::ScreenSize LayoutManager::classifyScreen(int width, int height) {
    // Classify based on smaller dimension to handle both orientations
    int min_dim = std::min(width, height);
    
    if (min_dim <= 320) {
        return ScreenSize::SMALL;   // ESP32 displays
    } else if (min_dim <= 600) {
        return ScreenSize::MEDIUM;  // Tablet-like
    } else {
        return ScreenSize::LARGE;   // Desktop
    }
}

LayoutManager::LayoutConfig LayoutManager::createConfigForScreen(ScreenSize size) {
    LayoutConfig config = {};
    
    switch (size) {
        case ScreenSize::SMALL:  // ESP32 displays (320x240, 480x320)
            config.dial_size = 45;           // Smaller dials
            config.dial_spacing_x = 90;      // Wider spacing to fill screen
            config.dial_spacing_y = 70;      // More vertical space
            config.button_width = 60;        // Slightly wider buttons
            config.button_height = 28;
            config.button_spacing = 32;
            config.margin_x = 18;            // More margin for centering
            config.margin_y = 10;
            config.grid_cols = 3;            // 3 columns to match dials
            config.status_height = 20;       // Compact status
            config.title_font_size = 12;
            config.label_font_size = 10;
            config.value_font_size = 8;
            break;
            
        case ScreenSize::MEDIUM:  // Tablet size
            config.dial_size = 60;
            config.dial_spacing_x = 80;
            config.dial_spacing_y = 90;
            config.button_width = 70;
            config.button_height = 30;
            config.button_spacing = 40;
            config.margin_x = 20;
            config.margin_y = 20;
            config.grid_cols = 3;
            config.status_height = 30;
            config.title_font_size = 16;
            config.label_font_size = 12;
            config.value_font_size = 10;
            break;
            
        case ScreenSize::LARGE:   // Desktop
            config.dial_size = 80;           // Current sizes
            config.dial_spacing_x = 120;
            config.dial_spacing_y = 120;
            config.button_width = 80;
            config.button_height = 35;
            config.button_spacing = 50;
            config.margin_x = 50;
            config.margin_y = 50;
            config.grid_cols = 3;
            config.status_height = 40;
            config.title_font_size = 20;
            config.label_font_size = 14;
            config.value_font_size = 12;
            break;
    }
    
    return config;
}

void LayoutManager::getGridPosition(int grid_x, int grid_y, int* out_x, int* out_y) {
    *out_x = current_config_.margin_x + grid_x * current_config_.dial_spacing_x;
    *out_y = current_config_.margin_y + grid_y * current_config_.dial_spacing_y;
}

int LayoutManager::scaleSize(int base_size) {
    switch (screen_size_) {
        case ScreenSize::SMALL:  return static_cast<int>(base_size * 0.6);  // 60% of base
        case ScreenSize::MEDIUM: return static_cast<int>(base_size * 0.8);  // 80% of base
        case ScreenSize::LARGE:  return base_size;        // 100% of base
    }
    return base_size;
}

void LayoutManager::getContentArea(int* width, int* height) {
    int screen_width = lv_display_get_horizontal_resolution(lv_display_get_default());
    int screen_height = lv_display_get_vertical_resolution(lv_display_get_default());
    
    *width = screen_width - (2 * current_config_.margin_x);
    *height = screen_height - (2 * current_config_.margin_y) - current_config_.status_height;
}
