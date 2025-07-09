    enum class ScreenSize {
        SMALL = 0,
        MEDIUM = 1,
        LARGE = 2
    };
#include "../../include/Constants.h"


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
        int dial_size = SynthConstants::Layout::DEFAULT_DIAL_SIZE;
        int dial_spacing_x = SynthConstants::Layout::DEFAULT_DIAL_SPACING_X;
        int dial_spacing_y = SynthConstants::Layout::DEFAULT_DIAL_SPACING_Y;

        // Button configuration
        int button_width = SynthConstants::Layout::DEFAULT_BUTTON_WIDTH;
        int button_height = SynthConstants::Layout::DEFAULT_BUTTON_HEIGHT;
        int button_spacing = SynthConstants::Layout::DEFAULT_BUTTON_SPACING;

        // General layout
        int margin_x = SynthConstants::Layout::DEFAULT_MARGIN_X;
        int margin_y = SynthConstants::Layout::DEFAULT_MARGIN_Y;
        int grid_cols = SynthConstants::Layout::DEFAULT_GRID_COLS;
        int status_height = SynthConstants::Layout::DEFAULT_STATUS_HEIGHT;

        // Font sizes (can be set per screen size)
        int title_font_size = 24;
        int label_font_size = 16;
        int value_font_size = 16;
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