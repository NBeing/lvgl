// Update SynthApp to use the modular display driver
#include "SynthApp.h"
#include "../MidiDial.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>

#if defined(ESP32_BUILD)
    #include "../../hardware/ESP32Display.h"
    extern uint32_t millis_cb();
#endif

SynthApp::SynthApp() : initialized_(false) {
#if defined(ESP32_BUILD)
    display_driver_ = std::make_unique<ESP32Display>();
#endif
}

SynthApp::~SynthApp() {
    // Cleanup handled by smart pointers automatically
    // ESP32Display destructor will be called automatically when display_driver_ goes out of scope
    std::cout << "SynthApp destructor called" << std::endl;
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
    // Initialize LVGL
    lv_init();
    lv_tick_set_cb(millis_cb);
    
    // Initialize modular display driver
    if (!display_driver_->initialize()) {
        std::cerr << "Failed to initialize display!" << std::endl;
        return;
    }
    
    // Optional: Enable touch debug for testing
    // display_driver_->setTouchDebug(true);
    
#else
    initDesktop();
#endif
}
void SynthApp::initDesktop() {
#ifndef ESP32_BUILD
    std::cout << "Initializing desktop simulation..." << std::endl;
    
    // Initialize LVGL
    lv_init();
    
    // Use LVGL's built-in SDL integration
    lv_display_t* disp = lv_sdl_window_create(480, 320);
    if (!disp) {
        std::cerr << "Failed to create SDL window!" << std::endl;
        return;
    }
    
    // Create SDL mouse input
    lv_indev_t* mouse = lv_sdl_mouse_create();
    
    // Optional: Create SDL keyboard input
    lv_indev_t* kb = lv_sdl_keyboard_create();
    
    // Set up tick function
    lv_tick_set_cb([]() -> uint32_t {
        static auto start = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    });
    
    std::cout << "Desktop simulation initialized with LVGL SDL!" << std::endl;
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