// src/hal/desktop/hal_desktop.cpp - Desktop simulator HAL
#ifdef DESKTOP_BUILD

#include "../hal.h"
#include "lvgl.h"
#include <SDL2/SDL.h>
#include <chrono>
#include <thread>
#include <iostream>

static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;
static SDL_Texture* texture = nullptr;
static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[800 * 480 / 10];

// SDL display flush callback
void sdl_display_flush(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    if (!renderer || !texture) {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    SDL_Rect dst_rect;
    dst_rect.x = area->x1;
    dst_rect.y = area->y1;
    dst_rect.w = w;
    dst_rect.h = h;

    // Convert LVGL color format to SDL format
    SDL_UpdateTexture(texture, &dst_rect, color_p, w * sizeof(lv_color_t));
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    lv_disp_flush_ready(disp_drv);
}

void hal_init() {
    std::cout << "Initializing Desktop HAL (SDL2)" << std::endl;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }
    
    // Create window
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

void hal_setup_display() {
    std::cout << "Setting up LVGL display driver" << std::endl;
    
    // Initialize display buffer
    lv_disp_draw_buf_init(&draw_buf, buf1, nullptr, 800 * 480 / 10);
    
    // Initialize display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 800;
    disp_drv.ver_res = 480;
    disp_drv.flush_cb = sdl_display_flush;
    disp_drv.draw_buf = &draw_buf;
    
    lv_disp_drv_register(&disp_drv);
    
    std::cout << "LVGL display driver registered (800x480)" << std::endl;
}

void hal_delay(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Use LVGL's built-in tick instead of custom
unsigned long hal_get_tick() {
    return lv_tick_get();
}

#endif // DESKTOP_BUILD