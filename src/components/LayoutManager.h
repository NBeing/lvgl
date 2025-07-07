#pragma once

#include "lvgl.h"

/**
 * @brief Responsive layout management for different screen sizes
 */
class LayoutManager {
public:
    enum class ScreenSize {
        SMALL,   // ESP32 displays (320x240, 480x320)
        MEDIUM,  // Tablet size (800x480)
        LARGE    // Desktop (1024x768+)
    };
    
    struct LayoutConfig {
        // Dial configuration
        int dial_size;
        int dial_spacing_x;
        int dial_spacing_y;
        
        // Button configuration
        int button_width;
        int button_height;
        int button_spacing;
        
        // General layout
        int margin_x;
        int margin_y;
        int grid_cols;
        int status_height;
        
        // Font sizes
        int title_font_size;
        int label_font_size;
        int value_font_size;
    };
    
private:
    static ScreenSize screen_size_;
    static LayoutConfig current_config_;
    static bool initialized_;
    
public:
    /**
     * Initialize layout manager with current screen dimensions
     */
    static void initialize();
    
    /**
     * Get current layout configuration
     */
    static const LayoutConfig& getConfig() { return current_config_; }
    
    /**
     * Get screen size classification
     */
    static ScreenSize getScreenSize() { return screen_size_; }
    
    /**
     * Calculate grid position for element
     */
    static void getGridPosition(int grid_x, int grid_y, int* out_x, int* out_y);
    
    /**
     * Get scaled size based on screen
     */
    static int scaleSize(int base_size);
    
    /**
     * Get available content area (excluding margins)
     */
    static void getContentArea(int* width, int* height);
    
private:
    static ScreenSize classifyScreen(int width, int height);
    static LayoutConfig createConfigForScreen(ScreenSize size);
};
