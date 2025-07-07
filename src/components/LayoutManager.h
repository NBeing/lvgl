#pragma once

#include "lvgl.h"

class LayoutManager {
public:
    enum class ScreenSize {
        SMALL,   // ESP32 displays (e.g., 320x240, 480x320)
        MEDIUM,  // Tablet size (e.g., 800x480)
        LARGE    // Desktop (e.g., 1024x768+)
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
    static void initialize();
    static const LayoutConfig& getConfig();
    static ScreenSize getScreenSize();

    static void getGridPosition(int grid_x, int grid_y, int* out_x, int* out_y);
    static int scaleSize(int base_size);
    static void getContentArea(int* width, int* height);

private:
    static ScreenSize classifyScreen(int width, int height);
    static LayoutConfig createConfigForScreen(ScreenSize size);
};