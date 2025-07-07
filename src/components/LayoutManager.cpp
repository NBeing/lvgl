#include "LayoutManager.h"
#include <algorithm>
#include <cstdio>

LayoutManager::ScreenSize LayoutManager::screen_size_ = ScreenSize::MEDIUM;
LayoutManager::LayoutConfig LayoutManager::current_config_ = {};
bool LayoutManager::initialized_ = false;

void LayoutManager::initialize() {
    if (initialized_) return;

    int screen_width = lv_display_get_horizontal_resolution(lv_display_get_default());
    int screen_height = lv_display_get_vertical_resolution(lv_display_get_default());

    screen_size_ = classifyScreen(screen_width, screen_height);
    current_config_ = createConfigForScreen(screen_size_);
    initialized_ = true;

#if defined(SYNTHAPP_DEBUG_UI_CHECKS)
    printf("LayoutManager: Screen %dx%d classified as %s\n",
           screen_width, screen_height,
           (screen_size_ == ScreenSize::SMALL) ? "SMALL" :
           (screen_size_ == ScreenSize::MEDIUM) ? "MEDIUM" : "LARGE");
#endif
}

const LayoutManager::LayoutConfig& LayoutManager::getConfig() {
    if (!initialized_) initialize();
    return current_config_;
}

LayoutManager::ScreenSize LayoutManager::getScreenSize() {
    if (!initialized_) initialize();
    return screen_size_;
}

LayoutManager::ScreenSize LayoutManager::classifyScreen(int width, int height) {
    int min_dim = std::min(width, height);
    if (min_dim <= 320) {
        return ScreenSize::SMALL;
    } else if (min_dim <= 600) {
        return ScreenSize::MEDIUM;
    } else {
        return ScreenSize::LARGE;
    }
}

LayoutManager::LayoutConfig LayoutManager::createConfigForScreen(ScreenSize size) {
    LayoutConfig config = {};
    switch (size) {
        case ScreenSize::SMALL:
            config.dial_size = 45;
            config.dial_spacing_x = 90; // Dynamically fills width for 3 columns
            config.dial_spacing_y = 65;
            config.button_width = 60;
            config.button_height = 28;
            config.button_spacing = 38;
            config.margin_x = 10;
            config.margin_y = 10;
            config.grid_cols = 3;
            config.status_height = 18;
            config.title_font_size = 12;
            config.label_font_size = 10;
            config.value_font_size = 8;
            break;
        case ScreenSize::MEDIUM:
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
        case ScreenSize::LARGE:
            config.dial_size = 80;
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
    const auto& cfg = getConfig();
    *out_x = cfg.margin_x + grid_x * cfg.dial_spacing_x;
    *out_y = cfg.margin_y + grid_y * cfg.dial_spacing_y;
}

int LayoutManager::scaleSize(int base_size) {
    // Always apply UI_SCALE for desktop/ESP32 parity
    return static_cast<int>(base_size * UI_SCALE);
}

void LayoutManager::getContentArea(int* width, int* height) {
    int screen_width = lv_display_get_horizontal_resolution(lv_display_get_default());
    int screen_height = lv_display_get_vertical_resolution(lv_display_get_default());
    const auto& cfg = getConfig();
    *width = screen_width - (2 * cfg.margin_x);
    *height = screen_height - (2 * cfg.margin_y) - cfg.status_height;
}