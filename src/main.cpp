// src/main.cpp - Main application entry point
#include <lvgl.h>
#include <iostream>
#include <chrono>
#include "components/MidiDial.h"

#if defined(ESP32_BUILD)
#include "hardware/LGFX_ST7796S.h"

LGFX_ST7796S tft;

// --- LVGL 9.3.0 display buffer and flush callback ---
static lv_color_t *buf1 = nullptr;
size_t buffer_pixels = 480 * 40;
static lv_display_t *display = nullptr;

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();
    lv_display_flush_ready(disp);
}

void hal_setup_display() {
    tft.begin();
    tft.setRotation(1); // Landscape

    buffer_pixels = tft.width() * 40; // 40 lines, adjust as needed
    if (buf1) heap_caps_free(buf1);
    buf1 = (lv_color_t *)heap_caps_malloc(buffer_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA);

    display = lv_display_create(tft.width(), tft.height());
    lv_display_set_flush_cb(display, my_disp_flush);
    lv_display_set_buffers(display, buf1, NULL, buffer_pixels * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
}

void hal_init() {
    // No-op for ESP32, but required for cross-platform compatibility
}

void hal_delay(int ms) {
    delay(ms);
}
#else
// Desktop simulator HAL
    #include <SDL2/SDL.h>
    #include <functional>
    #include <unordered_map>
    #include <memory>
    #include <thread>
    #include "hal.h"
#endif

// Forward declarations
void create_demo_ui();
void handle_events();

#if defined(ESP32_BUILD)
#define RAW_MIN_X  0
#define RAW_MAX_X  479
#define RAW_MIN_Y  0
#define RAW_MAX_Y  319

#define DISP_WIDTH  480
#define DISP_HEIGHT 320

static lv_indev_t* touch_indev = nullptr;

// Store last known good touch point
static uint16_t last_touch_x = 0;
static uint16_t last_touch_y = 0;

// Map raw touch to display coordinates
// static uint16_t map_touch_x(uint16_t raw_x) {
//     if (raw_x < RAW_MIN_X) raw_x = RAW_MIN_X;
//     if (raw_x > RAW_MAX_X) raw_x = RAW_MAX_X;
// return (uint16_t)(((raw_x - RAW_MIN_X) * (DISP_WIDTH - 1)) / (RAW_MAX_X - RAW_MIN_X));}
// static uint16_t map_touch_y(uint16_t raw_y) {
//     if (raw_y < RAW_MIN_Y) raw_y = RAW_MIN_Y;
//     if (raw_y > RAW_MAX_Y) raw_y = RAW_MAX_Y;
// return (uint16_t)(((raw_y - RAW_MIN_Y) * (DISP_HEIGHT - 1)) / (RAW_MAX_Y - RAW_MIN_Y));}

// LVGL touch read callback
void touch_read_cb(lv_indev_t* indev_drv, lv_indev_data_t* data) {
    std::cout << "=== touch read callback ===" << std::endl;
    uint16_t raw_x, raw_y;
    if (tft.getTouch(&raw_x, &raw_y)) {
        std::cout << printf("Raw touch: x=%u y=%u\n", raw_x, raw_y) << std::endl;
        last_touch_x = raw_x;// map_touch_x(raw_x);
        last_touch_y = raw_y;//map_touch_y(raw_y);
        data->point.x = last_touch_x;
        data->point.y = last_touch_y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->point.x = last_touch_x;
        data->point.y = last_touch_y;
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void hal_setup_touch() {
    std::cout << "=== setting up to touchie touch ===" << std::endl;
    touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, touch_read_cb);
    std::cout << "=== done setting touchie touch ===" << std::endl;
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
        std::cout << "=== going to touchie touch ===" << std::endl;

        #if defined(ESP32_BUILD)
            std::cout << "=== touchie touch ===" << std::endl;

            hal_setup_touch();
        #endif
        create_demo_ui();
        
        std::cout << "Synth GUI initialized successfully!" << std::endl;
    }
    
    void loop() {
        // Update LVGL tick
        // std::cout << "=== touchie touch ===" << std::endl;

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
    lv_label_set_text(title, "LVGL Synth GUI - 8-Bit Style");
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
        std::cout << "Filter Cutoff changed to: " << value << " (CC74)" << std::endl;
    });
    
    resonance_dial->onValueChanged([](int value) {
        std::cout << "Resonance changed to: " << value << " (CC71)" << std::endl;
    });
    
    volume_dial->onValueChanged([](int value) {
        std::cout << "Volume changed to: " << value << " (CC7)" << std::endl;
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
        
        std::cout << "Simulating MIDI CC - setting all dials to: " << test_value << std::endl;
        
        cutoff_dial->setValue(test_value);
        resonance_dial->setValue(test_value);
        volume_dial->setValue(test_value);
    }, LV_EVENT_CLICKED, nullptr);
    
    // Add screen background
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xFF101010), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);
    
    std::cout << "MIDI Synth GUI created with 3 dials!" << std::endl;
}

// MIDI CC handler function
void handleMidiCC(int cc_number, int value) {
    std::cout << "MIDI CC received: CC" << cc_number << " = " << value << std::endl;
    
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
    Serial.begin(115200);
    hal_delay(2000);
    Serial.println("Serial works!");

    app.setup();
}

void loop() {
    app.loop();
}
#endif