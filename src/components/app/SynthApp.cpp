// Update SynthApp to use the modular display driver
#include "SynthApp.h"
#include "../MidiDial.h"
#include "../ParameterControl.h"
#include "../ParameterBinder.h"
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
        display_driver_.reset(new ESP32Display());
    #endif
    
    midi_handler_.reset(new MidiHandler());
    parameter_binder_.reset(new ParameterBinder());
}

SynthApp::~SynthApp() {
    std::cout << "SynthApp destructor called" << std::endl;
}

void SynthApp::setup() {
    std::cout << "=== LVGL Synth GUI Starting ===" << std::endl;
    
    // Load Hydrasynth parameter definitions
    if (!parameter_binder_->loadSynthDefinition("hydrasynth")) {
        std::cout << "Failed to load Hydrasynth parameter definitions!" << std::endl;
    } else {
        std::cout << "Loaded " << parameter_binder_->getParameterCount() 
                  << " parameters for " << parameter_binder_->getCurrentSynthName() << std::endl;
    }
    
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
        
        // Initialize display
        if (!display_driver_->initialize()) {
            std::cerr << "Failed to initialize display!" << std::endl;
            return;
        }
        
        // Initialize MIDI
        if (!midi_handler_->initialize()) {
            std::cerr << "Failed to initialize MIDI!" << std::endl;
            return;
        }
        
        std::cout << "MIDI Status: " << midi_handler_->getConnectionStatus() << std::endl;
    #else
        initDesktop();
        
        // Initialize MIDI
        if (!midi_handler_->initialize()) {
            std::cerr << "Failed to initialize desktop MIDI!" << std::endl;
            return;
        }
        
        std::cout << "Desktop MIDI Status: " << midi_handler_->getConnectionStatus() << std::endl;
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
    lv_label_set_text(title, "LVGL Synth GUI - Parameter System");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Create help message label
    lv_obj_t* help_label = lv_label_create(lv_screen_active());
    lv_label_set_text(help_label, "NEW: Parameter-aware dials (top) • OLD: Legacy dials (bottom)");
    lv_obj_set_style_text_color(help_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(help_label, &lv_font_montserrat_10, 0);
    lv_obj_align(help_label, LV_ALIGN_TOP_MID, 0, 35);
    
    // Create NEW parameter-aware dials
    createParameterDials();
    
    // Create OLD legacy dials for comparison
    createLegacyDials();
    
    // Create "Simulate MIDI CC" button
    lv_obj_t* sim_button = lv_btn_create(lv_screen_active());
    lv_obj_set_size(sim_button, 200, 40);
    lv_obj_align(sim_button, LV_ALIGN_BOTTOM_MID, 0, -90);
    
    // Style the button for 8-bit look
    lv_obj_set_style_bg_color(sim_button, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_color(sim_button, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_border_width(sim_button, 2, 0);
    lv_obj_set_style_radius(sim_button, 4, 0);
    
    // Button label
    lv_obj_t* btn_label = lv_label_create(sim_button);
    lv_label_set_text(btn_label, "Simulate Values");
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
    lv_obj_align(status_area, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(status_area, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_color(status_area, lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_width(status_area, 1, 0);
    lv_obj_set_style_radius(status_area, 4, 0);
    
    // Status label inside the area
    lv_obj_t* status_label = lv_label_create(status_area);
    lv_label_set_text(status_label, "Ready - Compare NEW vs OLD dial systems");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_10, 0);
    lv_obj_center(status_label);
    
    // Store status label for updates
    status_label_ = status_label;
}

void SynthApp::createParameterDials() {
    std::cout << "Creating NEW parameter-aware dials..." << std::endl;
    
    // Create parameter-aware dials in a grid
    const int start_x = 40;
    const int start_y = 70;
    const int spacing_x = 120;
    
    // Row 1: NEW parameter-aware dials
    cutoff_dial_ = std::make_shared<DialControl>(lv_screen_active(), start_x, start_y);
    cutoff_dial_->setColor(lv_color_hex(0x00FF00));  // Green
    
    resonance_dial_ = std::make_shared<DialControl>(lv_screen_active(), start_x + spacing_x, start_y);
    resonance_dial_->setColor(lv_color_hex(0xFF8000));  // Orange
    
    volume_dial_ = std::make_shared<DialControl>(lv_screen_active(), start_x + 2*spacing_x, start_y);
    volume_dial_->setColor(lv_color_hex(0x0080FF));  // Blue
    
    attack_dial_ = std::make_shared<DialControl>(lv_screen_active(), start_x, start_y + 100);
    attack_dial_->setColor(lv_color_hex(0xFF00FF));  // Magenta
    
    release_dial_ = std::make_shared<DialControl>(lv_screen_active(), start_x + spacing_x, start_y + 100);
    release_dial_->setColor(lv_color_hex(0x80FF00));  // Yellow-green
    
    lfo_rate_dial_ = std::make_shared<DialControl>(lv_screen_active(), start_x + 2*spacing_x, start_y + 100);
    lfo_rate_dial_->setColor(lv_color_hex(0xFF0080));  // Pink
    
    // Bind parameters to the dials
    std::vector<std::pair<std::shared_ptr<DialControl>, std::string>> bindings = {
        {cutoff_dial_, "Filter 1 Cutoff"},
        {resonance_dial_, "Filter 1 Resonance"},
        {volume_dial_, "Master Volume"},
        {attack_dial_, "ENV 1 Attack"},
        {release_dial_, "ENV 1 Release"},
        {lfo_rate_dial_, "LFO 1 Rate"}
    };
    
    for (auto& [dial, param_name] : bindings) {
        auto parameter = parameter_binder_->findParameterByName(param_name);
        if (parameter) {
            dial->bindParameter(parameter);
            
            // Set up value change callback with MIDI output
            dial->setValueChangedCallback([this](uint8_t /* value */, const Parameter* param) {
                // Send MIDI CC using existing MidiHandler
                if (midi_handler_->isConnected()) {
                    midi_handler_->sendControlChange(1, param->getCCNumber(), param->getCurrentValue());
                }
                
                // Update status display
                char status_text[100];
                snprintf(status_text, sizeof(status_text), 
                        "NEW: %s = %s (CC%d)", 
                        param->getShortName().c_str(),
                        param->getValueDisplayText().c_str(),
                        static_cast<int>(param->getCCNumber()));
                if (status_label_) {
                    lv_label_set_text(status_label_, status_text);
                }
                
                std::cout << "Parameter changed: " << param->getName() 
                          << " = " << param->getValueDisplayText() 
                          << " (CC " << static_cast<int>(param->getCCNumber()) << ")" << std::endl;
            });
            
            std::cout << "Bound dial to parameter: " << param_name 
                      << " (CC " << static_cast<int>(parameter->getCCNumber()) << ")" << std::endl;
        } else {
            std::cout << "Parameter not found: " << param_name << std::endl;
        }
    }
}

void SynthApp::createLegacyDials() {
    std::cout << "Creating OLD legacy dials for comparison..." << std::endl;
    
    const int start_x = 40;
    const int start_y = 220;
    const int spacing_x = 120;
    
    // Create OLD MidiDial widgets for comparison
    old_cutoff_dial_.reset(new MidiDial(lv_screen_active(), "OLD-CUT", start_x, start_y));
    old_cutoff_dial_->setColor(lv_color_hex(0x006600));  // Dark Green
    old_cutoff_dial_->setMidiCC(74);
    old_cutoff_dial_->setValue(64);
    
    old_resonance_dial_.reset(new MidiDial(lv_screen_active(), "OLD-RES", start_x + spacing_x, start_y));
    old_resonance_dial_->setColor(lv_color_hex(0x664400));  // Dark Orange
    old_resonance_dial_->setMidiCC(71);
    old_resonance_dial_->setValue(32);
    
    old_volume_dial_.reset(new MidiDial(lv_screen_active(), "OLD-VOL", start_x + 2*spacing_x, start_y));
    old_volume_dial_->setColor(lv_color_hex(0x004466));  // Dark Blue
    old_volume_dial_->setMidiCC(7);
    old_volume_dial_->setValue(96);
    
    // Set up callbacks for old dials
    old_cutoff_dial_->onValueChanged([this](int value) {
        handleMidiCC(old_cutoff_dial_->getMidiCC(), value);
        char status_text[100];
        snprintf(status_text, sizeof(status_text), "OLD: CUTOFF = %d (CC%d)", value, 74);
        if (status_label_) {
            lv_label_set_text(status_label_, status_text);
        }
    });
    
    old_resonance_dial_->onValueChanged([this](int value) {
        handleMidiCC(old_resonance_dial_->getMidiCC(), value);
        char status_text[100];
        snprintf(status_text, sizeof(status_text), "OLD: RESONANCE = %d (CC%d)", value, 71);
        if (status_label_) {
            lv_label_set_text(status_label_, status_text);
        }
    });
    
    old_volume_dial_->onValueChanged([this](int value) {
        handleMidiCC(old_volume_dial_->getMidiCC(), value);
        char status_text[100];
        snprintf(status_text, sizeof(status_text), "OLD: VOLUME = %d (CC%d)", value, 7);
        if (status_label_) {
            lv_label_set_text(status_label_, status_text);
        }
    });
}

void SynthApp::loop() {
    if (!initialized_) return;
    
    lv_timer_handler();
    midi_handler_->update();  // Process MIDI with Control_Surface
    handleEvents();
    
    platformDelay(5);
}

void SynthApp::handleEvents() {
    // Additional event handling if needed
}

void SynthApp::handleMidiCC(int cc_number, int value) {
    if (!midi_handler_->isConnected()) {
        std::cout << "MIDI not connected!" << std::endl;
        return;
    }
    
    // Send MIDI CC message on channel 1 using Control_Surface
    midi_handler_->sendControlChange(1, cc_number, value);
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
    
    std::cout << "Simulating MIDI CC values set to: " << value << std::endl;
    
    // Update NEW parameter-aware dials
    if (cutoff_dial_ && cutoff_dial_->getBoundParameter()) {
        cutoff_dial_->getBoundParameter()->setValue(value);
    }
    if (resonance_dial_ && resonance_dial_->getBoundParameter()) {
        resonance_dial_->getBoundParameter()->setValue(value);
    }
    if (volume_dial_ && volume_dial_->getBoundParameter()) {
        volume_dial_->getBoundParameter()->setValue(value);
    }
    
    // Update OLD legacy dials
    if (old_cutoff_dial_) old_cutoff_dial_->setValue(value);
    if (old_resonance_dial_) old_resonance_dial_->setValue(value);
    if (old_volume_dial_) old_volume_dial_->setValue(value);
    
    updateStatus("SIMULATION", value);
    sim_counter++;
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