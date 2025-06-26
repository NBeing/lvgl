// src/hal/desktop/hal_desktop.cpp - Desktop simulator HAL
#ifdef DESKTOP_BUILD

#include "lvgl.h"
#include <SDL2/SDL.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <memory>
#include "hal.h"

static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;
static SDL_Texture* texture = nullptr;
static lv_display_t* display = nullptr;
static lv_color_t* buf1 = nullptr;
static int buffer_size = 0;

static lv_indev_t* indev = nullptr;

// Mouse input state
static int mouse_x = 0;
static int mouse_y = 0;
static bool mouse_pressed = false;

// SDL display flush callback for LVGL 9
void sdl_display_flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    if (!renderer || !texture) {
        lv_display_flush_ready(disp);
        return;
    }

    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    SDL_Rect dst_rect;
    dst_rect.x = area->x1;
    dst_rect.y = area->y1;
    dst_rect.w = w;
    dst_rect.h = h;

    // For 16-bit LVGL colors, each pixel is 2 bytes (ARGB)
    int pitch = w * 2;

    SDL_UpdateTexture(texture, &dst_rect, px_map, pitch);
    
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    lv_display_flush_ready(disp);
}

void hal_init() {
    std::cout << "Initializing Desktop HAL (SDL2)" << std::endl;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }

    window = SDL_CreateWindow("LVGL Synth GUI Simulator",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              800, 480, SDL_WINDOW_SHOWN);

    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 800, 480);
    if (!texture) {
        std::cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }

    std::cout << "SDL2 initialized successfully!" << std::endl;
}

// Mouse input callback for LVGL
void mouse_read(lv_indev_t* indev_drv, lv_indev_data_t* data) {
    (void)indev_drv;  // Suppress unused parameter warning
    
    // Get current window size to scale coordinates correctly
    int window_w, window_h;
    SDL_GetWindowSize(window, &window_w, &window_h);
    
    // Get current LVGL display size
    int display_w = lv_display_get_horizontal_resolution(display);
    int display_h = lv_display_get_vertical_resolution(display);
    
    // Scale mouse coordinates from window size to LVGL display size
    int scaled_x = (mouse_x * display_w) / window_w;
    int scaled_y = (mouse_y * display_h) / window_h;
    
    // Clamp coordinates to valid range
    scaled_x = std::max(0, std::min(display_w - 1, scaled_x));
    scaled_y = std::max(0, std::min(display_h - 1, scaled_y));
    
    data->point.x = scaled_x;
    data->point.y = scaled_y;
    data->state = mouse_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void handle_window_resize(int new_width, int new_height) {
    std::cout << "Handling window resize: " << new_width << "x" << new_height << std::endl;
    
    // Recreate the SDL texture with new size
    if (texture) {
        SDL_DestroyTexture(texture);
    }
    
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, new_width, new_height);
    if (!texture) {
        std::cerr << "Failed to recreate texture: " << SDL_GetError() << std::endl;
        return;
    }
    
    // Recreate LVGL buffer for new size (larger buffer = fewer strips)
    if (buf1) delete[] buf1;
    
    // Use larger buffer for smoother rendering (1/3 of screen instead of 1/10)
    buffer_size = (new_width * new_height) / 3;
    buf1 = new lv_color_t[buffer_size];
    
    // Update LVGL display size and buffer
    if (display) {
        lv_display_set_resolution(display, new_width, new_height);
        lv_display_set_buffers(display, buf1, nullptr, buffer_size * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
        
        // Force a complete screen refresh
        lv_obj_invalidate(lv_screen_active());
        lv_refr_now(display);
        
        std::cout << "LVGL display updated to: " << new_width << "x" << new_height << " with larger buffer" << std::endl;
    }
}

void handle_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            std::cout << "Quit event received, exiting..." << std::endl;
            exit(0);
        }
        else if (e.type == SDL_WINDOWEVENT) {
            switch (e.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    std::cout << "Window resized to: " << e.window.data1 << "x" << e.window.data2 << std::endl;
                    handle_window_resize(e.window.data1, e.window.data2);
                    break;
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    std::cout << "Window gained focus" << std::endl;
                    // Force refresh when gaining focus (useful in i3)
                    lv_obj_invalidate(lv_screen_active());
                    break;
                case SDL_WINDOWEVENT_FOCUS_LOST:
                    std::cout << "Window lost focus" << std::endl;
                    break;
                case SDL_WINDOWEVENT_EXPOSED:
                    // Window needs to be redrawn (common in tiling WMs)
                    lv_obj_invalidate(lv_screen_active());
                    break;
            }
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN) {
            mouse_x = e.button.x;
            mouse_y = e.button.y;
            mouse_pressed = true;
        }
        else if (e.type == SDL_MOUSEBUTTONUP) {
            mouse_x = e.button.x;
            mouse_y = e.button.y;
            mouse_pressed = false;
        }
        else if (e.type == SDL_MOUSEMOTION) {
            mouse_x = e.motion.x;
            mouse_y = e.motion.y;
        }
        else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    std::cout << "Escape pressed, exiting..." << std::endl;
                    exit(0);
                    break;
                case SDLK_F11:
                    {
                        static bool fullscreen = false;
                        fullscreen = !fullscreen;
                        SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                        std::cout << "Fullscreen " << (fullscreen ? "enabled" : "disabled") << std::endl;
                    }
                    break;
                case SDLK_F5:
                    // Manual refresh key (useful for debugging)
                    std::cout << "F5 pressed - forcing screen refresh" << std::endl;
                    lv_obj_invalidate(lv_screen_active());
                    lv_refr_now(display);
                    break;
            }
        }
    }
}

void hal_setup_display() {
    std::cout << "Setting up LVGL display driver" << std::endl;
    
    // Get initial window size for proper LVGL setup
    int window_w, window_h;
    SDL_GetWindowSize(window, &window_w, &window_h);
    
    // Allocate larger buffer for smoother rendering
    buffer_size = (window_w * window_h) / 3;  // 1/3 of screen = fewer strips
    buf1 = new lv_color_t[buffer_size];
    
    display = lv_display_create(window_w, window_h);  // Use actual window size
    if (!display) {
        std::cerr << "Failed to create LVGL display!" << std::endl;
        return;
    }
    
    // Use single buffer with larger size for better performance
    lv_display_set_buffers(display, buf1, nullptr, buffer_size * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, sdl_display_flush);
    
    // Set up mouse input device
    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, mouse_read);
    
    lv_obj_invalidate(lv_screen_active());
    
    std::cout << "LVGL display driver with optimized rendering (" << window_w << "x" << window_h << ", buffer: " << buffer_size << " pixels)" << std::endl;
}

void hal_delay(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

unsigned long hal_get_tick() {
    return lv_tick_get();
}

#endif // DESKTOP_BUILD