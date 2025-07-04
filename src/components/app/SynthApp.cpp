#include "SynthApp.h"
#include "../MidiDial.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>

#if defined(ESP32_BUILD)
    #include "../../hardware/LGFX_ST7796S.h"
    extern LGFX_ST7796S tft;  // Defined in main.cpp
    extern uint32_t millis_cb();  // Tick function from main.cpp
#endif

SynthApp::SynthApp() : initialized_(false) {
}

SynthApp::~SynthApp() {
    // Cleanup handled by smart pointers
}

void SynthApp::setup() {
    std::cout << "=== LVGL Synth GUI Starting ===" << std::endl;
    
    initHardware();
    createUI();
    
    initialized_ = true;
    std::cout << "Synth GUI initialized successfully!" << std::endl;
}

void SynthApp::initHardware() {
#if defined(ESP32_BUILD)
    // ESP32 hardware initialization
    lv_init();
    
    // Set up custom tick function AFTER lv_init()
    lv_tick_set_cb(millis_cb);
    
    // Display setup
    tft.begin();
    tft.setRotation(1);
    
    std::cout << "Display info:" << std::endl;
    std::cout << "  Width: " << tft.width() << std::endl;
    std::cout << "  Height: " << tft.height() << std::endl;
    std::cout << "  Rotation: " << tft.getRotation() << std::endl;
    
    size_t buffer_pixels = tft.width() * 40;
    lv_color_t* buf1 = (lv_color_t*)heap_caps_malloc(
        buffer_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA);
    
    lv_display_t* display = lv_display_create(tft.width(), tft.height());
    lv_display_set_flush_cb(display, [](lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
        uint32_t w = area->x2 - area->x1 + 1;
        uint32_t h = area->y2 - area->y1 + 1;
        tft.startWrite();
        tft.setAddrWindow(area->x1, area->y1, w, h);
        tft.pushPixels((uint16_t *)px_map, w * h, true);
        tft.endWrite();
        lv_display_flush_ready(disp);
    });
    lv_display_set_buffers(display, buf1, NULL, 
        buffer_pixels * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    // Touch setup with calibrated coordinates AND debug output
    lv_indev_t* touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, [](lv_indev_t* indev, lv_indev_data_t* data) {
        uint16_t raw_x, raw_y;
        
        uint_fast8_t touch_count = tft.getTouch(&raw_x, &raw_y);
        
        if (touch_count > 0) {
            // No Y masking needed - values are already in correct range
            uint16_t fixed_y = raw_y;  // Don't mask - use direct values
            
            // NEW calibration based on your ACTUAL current hardware values:
            // Top-left: X=449, Y=41-50    → should map to (0, 0)
            // Top-right: X=37-41, Y=20-30 → should map to (479, 0)  
            // Bottom-left: X=452, Y=292   → should map to (0, 319)
            // Bottom-right: X=60-61, Y=290-292 → should map to (479, 319)
            
            const int TOUCH_MIN_X = 37;   // Right edge (smallest X)
            const int TOUCH_MAX_X = 452;  // Left edge (largest X)
            const int TOUCH_MIN_Y = 20;   // Top edge (smallest Y)
            const int TOUCH_MAX_Y = 292;  // Bottom edge (largest Y)
            
            // X mapping: 452 (left) → 0, 37 (right) → 479
            int mapped_x = ((TOUCH_MAX_X - raw_x) * 480) / (TOUCH_MAX_X - TOUCH_MIN_X);
            int mapped_y = ((fixed_y - TOUCH_MIN_Y) * 320) / (TOUCH_MAX_Y - TOUCH_MIN_Y);
            
            // Clamp to screen bounds
            mapped_x = std::max(0, std::min(mapped_x, 479));
            mapped_y = std::max(0, std::min(mapped_y, 319));
            
            data->point.x = mapped_x;
            data->point.y = mapped_y;
            data->state = LV_INDEV_STATE_PRESSED;
            
            // Debug output
            // std::cout << "Touch: raw(" << raw_x << "," << fixed_y 
            //           << ") → mapped(" << mapped_x << "," << mapped_y << ")" << std::endl;
            
        } else {
            data->state = LV_INDEV_STATE_RELEASED;
        }
    });
    
    std::cout << "ESP32 hardware initialization complete!" << std::endl;
    std::cout << "Touch panel configured and ready" << std::endl;
    
#else
    // Desktop initialization with official SDL driver
    initDesktop();
#endif
}

void SynthApp::initDesktop() {
#ifndef ESP32_BUILD
    lv_init();
    
    // Create SDL window using official LVGL driver
    lv_display_t* display = lv_sdl_window_create(800, 480);
    if (!display) {
        std::cerr << "Failed to create SDL window!" << std::endl;
        return;
    }
    
    // Create input devices
    lv_sdl_mouse_create();
    lv_sdl_keyboard_create();
    
    // Set up keyboard navigation
    lv_group_t* group = lv_group_create();
    lv_group_set_default(group);
#endif
}

void SynthApp::createUI() {
    // Set dark background for 8-bit retro feel
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x0a0a0a), 0);
    
    // Create title with better styling
    lv_obj_t* title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "LVGL Synth GUI - 8-Bit Style");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Create help message label
    lv_obj_t* help_label = lv_label_create(lv_screen_active());
    lv_label_set_text(help_label, "Touch dials to adjust values • CC messages sent via MIDI");
    lv_obj_set_style_text_color(help_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(help_label, &lv_font_montserrat_10, 0);
    lv_obj_align(help_label, LV_ALIGN_TOP_MID, 0, 35);
    
    // Create MIDI dial widgets with proper spacing
    cutoff_dial_ = std::make_unique<MidiDial>(lv_screen_active(), "CUTOFF", 80, 80);
    cutoff_dial_->setColor(lv_color_hex(0x00FF00));  // Green
    cutoff_dial_->setMidiCC(74);
    cutoff_dial_->setValue(64);  // Set initial value
    
    resonance_dial_ = std::make_unique<MidiDial>(lv_screen_active(), "RESONANCE", 200, 80);
    resonance_dial_->setColor(lv_color_hex(0xFF0000));  // Red
    resonance_dial_->setMidiCC(71);
    resonance_dial_->setValue(32);  // Set initial value
    
    volume_dial_ = std::make_unique<MidiDial>(lv_screen_active(), "VOLUME", 320, 80);
    volume_dial_->setColor(lv_color_hex(0x0000FF));  // Blue
    volume_dial_->setMidiCC(7);
    volume_dial_->setValue(96);  // Set initial value
    
    // Create "Simulate MIDI CC" button
    lv_obj_t* sim_button = lv_btn_create(lv_screen_active());
    lv_obj_set_size(sim_button, 200, 40);
    lv_obj_align(sim_button, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    // Style the button for 8-bit look
    lv_obj_set_style_bg_color(sim_button, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_color(sim_button, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_border_width(sim_button, 2, 0);
    lv_obj_set_style_radius(sim_button, 4, 0);
    
    // Button label
    lv_obj_t* btn_label = lv_label_create(sim_button);
    lv_label_set_text(btn_label, "Simulate MIDI CC");
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_12, 0);
    lv_obj_center(btn_label);
    
    // Add button click event
    lv_obj_add_event_cb(sim_button, [](lv_event_t* e) {
        SynthApp* app = static_cast<SynthApp*>(lv_event_get_user_data(e));
        app->simulateMidiCC();
    }, LV_EVENT_CLICKED, this);
    
    // Create status/info area
    lv_obj_t* status_area = lv_obj_create(lv_screen_active());
    lv_obj_set_size(status_area, 380, 60);
    lv_obj_align(status_area, LV_ALIGN_BOTTOM_MID, 0, -80);
    lv_obj_set_style_bg_color(status_area, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_color(status_area, lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_width(status_area, 1, 0);
    lv_obj_set_style_radius(status_area, 4, 0);
    
    // Status label inside the area
    lv_obj_t* status_label = lv_label_create(status_area);
    lv_label_set_text(status_label, "Ready - Touch dials or button to test MIDI output");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_10, 0);
    lv_obj_center(status_label);
    
    // Store status label for updates
    status_label_ = status_label;
    
    // Set up callbacks
    cutoff_dial_->onValueChanged([this](int value) {
        handleMidiCC(cutoff_dial_->getMidiCC(), value);
        updateStatus("CUTOFF", value);
    });
    
    resonance_dial_->onValueChanged([this](int value) {
        handleMidiCC(resonance_dial_->getMidiCC(), value);
        updateStatus("RESONANCE", value);
    });
    
    volume_dial_->onValueChanged([this](int value) {
        handleMidiCC(volume_dial_->getMidiCC(), value);
        updateStatus("VOLUME", value);
    });
}

void SynthApp::loop() {
    if (!initialized_) return;
    
    lv_timer_handler();
    handleEvents();
    
    platformDelay(5);
}

void SynthApp::handleEvents() {
    // Additional event handling if needed
}

void SynthApp::handleMidiCC(int cc_number, int value) {
    std::cout << "MIDI CC " << cc_number << ": " << value << std::endl;
    // Add your MIDI output logic here
}

void SynthApp::platformDelay(int ms) {
#if defined(ESP32_BUILD)
    delay(ms);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}

unsigned long SynthApp::getPlatformTick() {
    return lv_tick_get();
}

void SynthApp::simulateMidiCC() {
    static int sim_counter = 0;
    
    // Cycle through different values for demonstration
    int values[] = {0, 32, 64, 96, 127};
    int value = values[sim_counter % 5];
    
    // Update all dials with the simulated value
    cutoff_dial_->setValue(value);
    resonance_dial_->setValue(value);
    volume_dial_->setValue(value);
    
    updateStatus("SIMULATION", value);
    sim_counter++;
    
    std::cout << "Simulated MIDI CC values set to: " << value << std::endl;
}

void SynthApp::updateStatus(const char* control, int value) {
    if (status_label_) {
        char status_text[100];
        snprintf(status_text, sizeof(status_text), 
                "Last: %s = %d (CC%d)", control, value, 
                strcmp(control, "CUTOFF") == 0 ? 74 :
                strcmp(control, "RESONANCE") == 0 ? 71 : 7);
        lv_label_set_text(status_label_, status_text);
    }
}