// src/main.cpp - Main application entry point
#include "lvgl.h"
#include <iostream>
#include <algorithm>  // For std::max, std::min
#include <functional> // For std::function
#include <memory>     // For std::unique_ptr
#include <unordered_map>

#ifdef ESP32_BUILD
#include <Arduino.h>
#endif

#ifdef DESKTOP_BUILD
#include <SDL2/SDL.h>
#include <chrono>
#include <thread>
#endif

// Forward declarations
void create_demo_ui();
void handle_events();

// ==============================================
// MIDI DIAL WIDGET CLASS - 8-Bit Style
// ==============================================

class MidiDial {
public:
    MidiDial(lv_obj_t* parent, const char* label = "", int x = 0, int y = 0);
    ~MidiDial();
    
    void setValue(int value);
    int getValue() const { return current_value_; }
    void setRange(int min_val, int max_val);
    void setColor(lv_color_t color);
    void setPosition(int x, int y);
    void onValueChanged(std::function<void(int)> callback);
    lv_obj_t* getObject() { return container_; }
    void setMidiCC(int cc_number) { midi_cc_ = cc_number; }
    int getMidiCC() const { return midi_cc_; }

private:
    lv_obj_t* container_;
    lv_obj_t* value_label_;
    lv_obj_t* name_label_;
    lv_obj_t* arc_display_;
    
    int current_value_;
    int min_value_;
    int max_value_;
    int midi_cc_;
    lv_color_t arc_color_;
    std::function<void(int)> value_callback_;
    
    void updateDisplay();
    void setupStyling();
    static void click_event_cb(lv_event_t* e);
};

// Static map to link LVGL objects back to C++ instances
static std::unordered_map<lv_obj_t*, MidiDial*> dial_widget_map;

MidiDial::MidiDial(lv_obj_t* parent, const char* label, int x, int y) 
    : current_value_(0)
    , min_value_(0) 
    , max_value_(127)
    , midi_cc_(-1)
    , arc_color_(lv_color_hex(0xFF00FF00))
{
    // Create container object
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, 80, 80);
    lv_obj_set_pos(container_, x, y);
    
    // Store mapping for static callbacks
    dial_widget_map[container_] = this;
    
    // Create parameter name label
    name_label_ = lv_label_create(container_);
    lv_label_set_text(name_label_, label);
    lv_obj_set_style_text_color(name_label_, lv_color_hex(0xFFCCCCCC), 0);
    lv_obj_set_style_text_font(name_label_, &lv_font_montserrat_10, 0);
    lv_obj_align(name_label_, LV_ALIGN_TOP_MID, 0, 2);
    
    // Create arc display using LVGL's built-in arc widget
    arc_display_ = lv_arc_create(container_);
    lv_obj_set_size(arc_display_, 50, 50);
    lv_obj_align(arc_display_, LV_ALIGN_CENTER, 0, -5);
    
    // Configure arc for semicircle (8-bit style)
    lv_arc_set_bg_angles(arc_display_, 180, 0);  // Background semicircle
    lv_arc_set_angles(arc_display_, 180, 180);   // Start with no fill
    lv_arc_set_range(arc_display_, min_value_, max_value_);
    
    // Style the arc for 8-bit look
    lv_obj_set_style_arc_color(arc_display_, lv_color_hex(0xFF444444), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_display_, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_display_, arc_color_, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_display_, 6, LV_PART_INDICATOR);
    
    // Hide the knob for cleaner look
    lv_obj_set_style_bg_opa(arc_display_, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_opa(arc_display_, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc_display_, 0, LV_PART_KNOB);
    
    // Create value display label
    value_label_ = lv_label_create(container_);
    lv_label_set_text(value_label_, "000");
    lv_obj_set_style_text_color(value_label_, arc_color_, 0);
    lv_obj_set_style_text_font(value_label_, &lv_font_montserrat_12, 0);
    lv_obj_align(value_label_, LV_ALIGN_BOTTOM_MID, 0, -2);
    
    // Set up event handlers for click interaction
    lv_obj_add_event_cb(container_, click_event_cb, LV_EVENT_CLICKED, this);
    
    // Apply 8-bit retro styling
    setupStyling();
    
    // Initialize display
    updateDisplay();
    
    std::cout << "Created MIDI dial: " << label << std::endl;
}

MidiDial::~MidiDial() {
    dial_widget_map.erase(container_);
}

void MidiDial::setValue(int value) {
    if (value < min_value_) value = min_value_;
    if (value > max_value_) value = max_value_;
    
    if (current_value_ != value) {
        current_value_ = value;
        updateDisplay();
        
        if (value_callback_) {
            value_callback_(value);
        }
        
        std::cout << "MIDI Dial value changed to: " << value << std::endl;
    }
}

void MidiDial::setRange(int min_val, int max_val) {
    min_value_ = min_val;
    max_value_ = max_val;
    setValue(current_value_);
}

void MidiDial::setColor(lv_color_t color) {
    arc_color_ = color;
    lv_obj_set_style_text_color(value_label_, color, 0);
    lv_obj_set_style_arc_color(arc_display_, color, LV_PART_INDICATOR);
}

void MidiDial::setPosition(int x, int y) {
    lv_obj_set_pos(container_, x, y);
}

void MidiDial::onValueChanged(std::function<void(int)> callback) {
    value_callback_ = callback;
}

void MidiDial::updateDisplay() {
    char text[8];
    snprintf(text, sizeof(text), "%03d", current_value_);
    lv_label_set_text(value_label_, text);
    
    // Update arc value using LVGL's built-in arc widget
    lv_arc_set_value(arc_display_, current_value_);
    
    // Calculate angle for the arc (semicircle from 180° to 0°)
    int value_range = max_value_ - min_value_;
    if (value_range > 0) {
        int angle_range = 180;  // Semicircle
        int current_angle = 180 - ((current_value_ * angle_range) / value_range);
        lv_arc_set_angles(arc_display_, 180, current_angle);
    }
}

void MidiDial::setupStyling() {
    // 8-bit retro container styling
    lv_obj_set_style_bg_color(container_, lv_color_hex(0xFF1a1a1a), 0);
    lv_obj_set_style_bg_opa(container_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(container_, lv_color_hex(0xFF666666), 0);
    lv_obj_set_style_border_width(container_, 2, 0);
    lv_obj_set_style_border_opa(container_, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(container_, 4, 0);
    lv_obj_set_style_pad_all(container_, 4, 0);
}

void MidiDial::click_event_cb(lv_event_t* e) {
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    
    auto it = dial_widget_map.find(obj);
    if (it != dial_widget_map.end()) {
        MidiDial* dial = it->second;
        
        // Increment value on click (for testing)
        int new_value = dial->getValue() + 10;
        if (new_value > dial->max_value_) {
            new_value = dial->min_value_;
        }
        dial->setValue(new_value);
        
        std::cout << "🎛️ MIDI Dial clicked! New value: " << new_value << std::endl;
    }
}

// ==============================================
// PLATFORM-SPECIFIC HAL IMPLEMENTATION
// ==============================================

#ifdef DESKTOP_BUILD
static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;
static SDL_Texture* texture = nullptr;
static lv_display_t* display = nullptr;
static lv_indev_t* indev = nullptr;
static lv_color_t* buf1 = nullptr;
static int buffer_size = 0;
// Mouse input state
static int mouse_x = 0;
static int mouse_y = 0;
static bool mouse_pressed = false;

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

// SDL display flush callback for LVGL 9.x
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

    // For 32-bit LVGL colors, each pixel is 4 bytes (ARGB)
    int pitch = w * 4;

    SDL_UpdateTexture(texture, &dst_rect, px_map, pitch);
    
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    lv_display_flush_ready(disp);
}

void handle_window_resize(int new_width, int new_height) {
    std::cout << "Handling window resize: " << new_width << "x" << new_height << std::endl;
    
    // Recreate the SDL texture with new size
    if (texture) {
        SDL_DestroyTexture(texture);
    }
    
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, new_width, new_height);
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

void hal_init() {
    std::cout << "Initializing Desktop HAL (SDL2) for Regolith/i3" << std::endl;
    
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11");
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    SDL_SetHint(SDL_HINT_VIDEO_X11_WINDOW_VISUALID, "");
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }
    
    window = SDL_CreateWindow("LVGL Synth GUI Simulator",
                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             800, 480, 
                             SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }
    
    SDL_SetWindowMinimumSize(window, 400, 240);
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cout << "Hardware acceleration failed, trying software renderer..." << std::endl;
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }
    
    // Create initial texture with window size
    int window_w, window_h;
    SDL_GetWindowSize(window, &window_w, &window_h);
    
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, window_w, window_h);
    if (!texture) {
        std::cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }
    
    std::cout << "SDL2 initialized successfully with i3/Regolith optimizations! Initial size: " << window_w << "x" << window_h << std::endl;
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
#endif

// ==============================================
// MAIN APPLICATION CLASS
// ==============================================

class SynthApp {
public:
    void setup() {
        std::cout << "=== LVGL Synth GUI Starting ===" << std::endl;
        
        hal_init();
        lv_init();
        hal_setup_display();
        create_demo_ui();
        
        std::cout << "Synth GUI initialized successfully!" << std::endl;
    }
    
    void loop() {
        // Update LVGL tick
        static auto last_tick = std::chrono::steady_clock::now();
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_tick);
        
        if (elapsed.count() > 0) {
            lv_tick_inc(elapsed.count());
            last_tick = current_time;
        }
        
        lv_timer_handler();
        
#ifdef DESKTOP_BUILD
        handle_events();
#endif
        
        hal_delay(5);
    }
};

// Global MIDI dials for testing
std::unique_ptr<MidiDial> cutoff_dial;
std::unique_ptr<MidiDial> resonance_dial;
std::unique_ptr<MidiDial> volume_dial;

void create_demo_ui() {
    // Add title
    lv_obj_t* title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "🎛️ LVGL Synth GUI - 8-Bit Style");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF00FF00), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Create MIDI dial widgets
    cutoff_dial = std::make_unique<MidiDial>(lv_screen_active(), "CUTOFF", 50, 80);
    cutoff_dial->setColor(lv_color_hex(0xFF00FF00));  // Green
    cutoff_dial->setMidiCC(74);
    cutoff_dial->setValue(64);
    
    resonance_dial = std::make_unique<MidiDial>(lv_screen_active(), "RESONANCE", 200, 80);
    resonance_dial->setColor(lv_color_hex(0xFFFF3030));  // Red
    resonance_dial->setMidiCC(71);
    resonance_dial->setValue(32);
    
    volume_dial = std::make_unique<MidiDial>(lv_screen_active(), "VOLUME", 350, 80);
    volume_dial->setColor(lv_color_hex(0xFF3080FF));  // Blue
    volume_dial->setMidiCC(7);
    volume_dial->setValue(100);
    
    // Set up value change callbacks
    cutoff_dial->onValueChanged([](int value) {
        std::cout << "🎚️ Filter Cutoff changed to: " << value << " (CC74)" << std::endl;
    });
    
    resonance_dial->onValueChanged([](int value) {
        std::cout << "🌊 Resonance changed to: " << value << " (CC71)" << std::endl;
    });
    
    volume_dial->onValueChanged([](int value) {
        std::cout << "🔊 Volume changed to: " << value << " (CC7)" << std::endl;
    });
    
    // Add instructions
    lv_obj_t* instructions = lv_label_create(lv_screen_active());
    lv_label_set_text(instructions, "Click dials to change values • F11=Fullscreen • ESC=Exit");
    lv_obj_set_style_text_color(instructions, lv_color_hex(0xFFCCCCCC), 0);
    lv_obj_set_style_text_font(instructions, &lv_font_montserrat_10, 0);
    lv_obj_align(instructions, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    // Add a test MIDI CC simulation button
    lv_obj_t* test_btn = lv_button_create(lv_screen_active());
    lv_obj_set_size(test_btn, 150, 40);
    lv_obj_align(test_btn, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_bg_color(test_btn, lv_color_hex(0xFF8000FF), 0);  // Purple
    
    lv_obj_t* btn_label = lv_label_create(test_btn);
    lv_label_set_text(btn_label, "Simulate MIDI CC");
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xFFFFFFFF), 0);
    lv_obj_center(btn_label);
    
    // Add event for MIDI simulation
    lv_obj_add_event_cb(test_btn, [](lv_event_t* e) {
        (void)e;  // Suppress unused parameter warning
        static int test_value = 0;
        test_value = (test_value + 25) % 128;
        
        std::cout << "🎵 Simulating MIDI CC - setting all dials to: " << test_value << std::endl;
        
        cutoff_dial->setValue(test_value);
        resonance_dial->setValue(test_value);
        volume_dial->setValue(test_value);
    }, LV_EVENT_CLICKED, nullptr);
    
    // Add screen background
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xFF101010), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);
    
    std::cout << "🎛️ MIDI Synth GUI created with 3 dials!" << std::endl;
}

// MIDI CC handler function
void handleMidiCC(int cc_number, int value) {
    std::cout << "📡 MIDI CC received: CC" << cc_number << " = " << value << std::endl;
    
    if (cutoff_dial && cutoff_dial->getMidiCC() == cc_number) {
        cutoff_dial->setValue(value);
    } else if (resonance_dial && resonance_dial->getMidiCC() == cc_number) {
        resonance_dial->setValue(value);
    } else if (volume_dial && volume_dial->getMidiCC() == cc_number) {
        volume_dial->setValue(value);
    }
}

// Global app instance
SynthApp app;

#ifdef DESKTOP_BUILD
int main() {
    app.setup();
    
    std::cout << "Starting main loop (press Ctrl+C or close window to exit)" << std::endl;
    
    while (true) {
        app.loop();
    }
    
    return 0;
}
#endif

#ifdef ESP32_BUILD
void setup() {
    app.setup();
}

void loop() {
    app.loop();
}
#endif