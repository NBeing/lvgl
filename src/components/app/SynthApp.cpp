// Update SynthApp to use the modular display driver
#include "SynthApp.h"
#include "../MidiDial.h"
#include "../ParameterControl.h"
#include "../ParameterBinder.h"
#include "../CommandManager.h"
#include "../ButtonControl.h"
#include "../LayoutManager.h"
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
    command_manager_.reset(new CommandManager());
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
    
    // Inject command manager into all parameters
    auto all_params = parameter_binder_->getAllParameters();
    for (auto param : all_params) {
        param->setCommandManager(command_manager_.get());
    }
    
    // Set up history change callback to update UI
    command_manager_->setHistoryChangedCallback([this]() {
        updateUndoRedoButtons();
    });
    
    initHardware();
    // std::cout << "About to create ui!!" << std::endl;

    createUI();
    
    initialized_ = true;
    std::cout << "Synth GUI initialized successfully!" << std::endl;
}

void SynthApp::initHardware() {
    #if defined(ESP32_BUILD)
        // Initialize LVGL
        lv_init();
        lv_tick_set_cb(millis_cb);
        std::cout << "[ESP32] LVGL initialized. Registering input devices..." << std::endl;
        // Print all input devices for debug
        lv_indev_t* indev = lv_indev_get_next(NULL);
        int indev_count = 0;
        while (indev) {
            std::cout << "[ESP32] Found LVGL input device: " << indev << std::endl;
            indev = lv_indev_get_next(indev);
            indev_count++;
        }
        std::cout << "[ESP32] Total LVGL input devices: " << indev_count << std::endl;
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
    // Initialize the layout manager first
    LayoutManager::initialize();
    const auto& config = LayoutManager::getConfig();
    std::cout << "Using layout config: dial_size=" << config.dial_size 
              << ", spacing=" << config.dial_spacing_x << "x" << config.dial_spacing_y
              << ", margins=" << config.margin_x << "x" << config.margin_y << std::endl;

    // Set dark background for 8-bit retro feel
    lv_obj_t* screen = lv_screen_active();
    if (!screen) {
#if SYNTHAPP_DEBUG_UI_CHECKS
        std::cerr << "[UI-ERR] lv_screen_active() returned nullptr!" << std::endl;
#endif
        return;
    }
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0a0a0a), 0);
    // Add a transparent background object to catch and debug touch events
    lv_obj_t* debug_bg = lv_obj_create(screen);
    lv_obj_set_size(debug_bg, lv_obj_get_width(screen), lv_obj_get_height(screen));
    lv_obj_set_style_bg_opa(debug_bg, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(debug_bg, LV_OBJ_FLAG_CLICKABLE); // Don't block events
    lv_obj_move_background(debug_bg);
    lv_obj_add_event_cb(debug_bg, [](lv_event_t* e) {
        lv_point_t p;
        lv_indev_get_point(lv_indev_get_act(), &p);
        std::cout << "[DEBUG] Touch event at (" << p.x << ", " << p.y << ") code=" << lv_event_get_code(e) << std::endl;
    }, LV_EVENT_ALL, NULL);

    // Create title with responsive positioning and font
    lv_obj_t* title = lv_label_create(screen);
    if (!title) {
#if SYNTHAPP_DEBUG_UI_CHECKS
        std::cerr << "[UI-ERR] Failed to create title label!" << std::endl;
#endif
        return;
    }
    lv_label_set_text(title, "LVGL Synth GUI - Parameter System");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00FF00), 0);

    // Use appropriate font based on screen size
    if (LayoutManager::getScreenSize() == LayoutManager::ScreenSize::SMALL) {
        lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
    } else {
        lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    }
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, config.margin_y / 2);

    // Create help message label (only on larger screens)
    if (LayoutManager::getScreenSize() != LayoutManager::ScreenSize::SMALL) {
        lv_obj_t* help_label = lv_label_create(screen);
        if (!help_label) {
#if SYNTHAPP_DEBUG_UI_CHECKS
            std::cerr << "[UI-ERR] Failed to create help label!" << std::endl;
#endif
        } else {
            lv_label_set_text(help_label, "NEW: Parameter-aware dials (top) • OLD: Legacy dials (bottom)");
            lv_obj_set_style_text_color(help_label, lv_color_hex(0xCCCCCC), 0);
            lv_obj_set_style_text_font(help_label, &lv_font_montserrat_10, 0);
            lv_obj_align(help_label, LV_ALIGN_TOP_MID, 0, config.margin_y / 2 + 25);
        }
    }

    // Create NEW parameter-aware dials
    createParameterDials();

    // Create NEW button controls
    createButtonControls();

    // Create undo/redo controls
    createUndoRedoControls();
}

void SynthApp::createParameterDials() {
    std::cout << "Creating NEW parameter-aware dials..." << std::endl;
    
    const auto& config = LayoutManager::getConfig();
    lv_obj_t* screen = lv_screen_active();
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!screen) {
        std::cerr << "[UI-ERR] lv_screen_active() returned nullptr in createParameterDials!" << std::endl;
        return;
    }
#endif
    // Calculate starting position for the dial grid
    int grid_start_y = 60; // Title area
    if (LayoutManager::getScreenSize() != LayoutManager::ScreenSize::SMALL) {
        grid_start_y = 70; // Account for help text on larger screens
    }

    // Create parameter-aware dials using responsive grid layout
    int dial_x, dial_y;

    // Row 1: Primary controls (grid positions 0,0 through 2,0)
    LayoutManager::getGridPosition(0, 0, &dial_x, &dial_y);
    dial_y += grid_start_y; // Offset for title area
    cutoff_dial_ = std::make_shared<DialControl>(screen, dial_x, dial_y);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!cutoff_dial_) std::cerr << "[UI-ERR] Failed to create cutoff_dial_!" << std::endl;
#endif
    cutoff_dial_->setColor(lv_color_hex(0x00FF00));  // Green
    cutoff_dial_->setDialSize(config.dial_size);
    cutoff_dial_->setSize(config.dial_size + 20, config.dial_size + 30);
    cutoff_dial_->setPosition(dial_x, dial_y);

    LayoutManager::getGridPosition(1, 0, &dial_x, &dial_y);
    dial_y += grid_start_y;
    resonance_dial_ = std::make_shared<DialControl>(screen, dial_x, dial_y);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!resonance_dial_) std::cerr << "[UI-ERR] Failed to create resonance_dial_!" << std::endl;
#endif
    resonance_dial_->setColor(lv_color_hex(0xFF8000));  // Orange
    resonance_dial_->setDialSize(config.dial_size);
    resonance_dial_->setSize(config.dial_size + 20, config.dial_size + 30);
    resonance_dial_->setPosition(dial_x, dial_y);

    LayoutManager::getGridPosition(2, 0, &dial_x, &dial_y);
    dial_y += grid_start_y;
    volume_dial_ = std::make_shared<DialControl>(screen, dial_x, dial_y);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!volume_dial_) std::cerr << "[UI-ERR] Failed to create volume_dial_!" << std::endl;
#endif
    volume_dial_->setColor(lv_color_hex(0x0080FF));  // Blue
    volume_dial_->setDialSize(config.dial_size);
    volume_dial_->setSize(config.dial_size + 20, config.dial_size + 30);
    volume_dial_->setPosition(dial_x, dial_y);

    // Row 2: Secondary controls (grid positions 0,1 through 2,1)
    LayoutManager::getGridPosition(0, 1, &dial_x, &dial_y);
    dial_y += grid_start_y;
    attack_dial_ = std::make_shared<DialControl>(screen, dial_x, dial_y);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!attack_dial_) std::cerr << "[UI-ERR] Failed to create attack_dial_!" << std::endl;
#endif
    attack_dial_->setColor(lv_color_hex(0xFF00FF));  // Magenta
    attack_dial_->setDialSize(config.dial_size);
    attack_dial_->setSize(config.dial_size + 20, config.dial_size + 30);
    attack_dial_->setPosition(dial_x, dial_y);

    LayoutManager::getGridPosition(1, 1, &dial_x, &dial_y);
    dial_y += grid_start_y;
    release_dial_ = std::make_shared<DialControl>(screen, dial_x, dial_y);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!release_dial_) std::cerr << "[UI-ERR] Failed to create release_dial_!" << std::endl;
#endif
    release_dial_->setColor(lv_color_hex(0x80FF00));  // Yellow-green
    release_dial_->setDialSize(config.dial_size);
    release_dial_->setSize(config.dial_size + 20, config.dial_size + 30);
    release_dial_->setPosition(dial_x, dial_y);

    LayoutManager::getGridPosition(2, 1, &dial_x, &dial_y);
    dial_y += grid_start_y;
    lfo_rate_dial_ = std::make_shared<DialControl>(screen, dial_x, dial_y);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!lfo_rate_dial_) std::cerr << "[UI-ERR] Failed to create lfo_rate_dial_!" << std::endl;
#endif
    lfo_rate_dial_->setColor(lv_color_hex(0xFF0080));  // Pink
    lfo_rate_dial_->setDialSize(config.dial_size);
    lfo_rate_dial_->setSize(config.dial_size + 20, config.dial_size + 30);
    lfo_rate_dial_->setPosition(dial_x, dial_y);

    // Bind parameters to the dials
    std::vector<std::pair<std::shared_ptr<DialControl>, std::string>> bindings = {
        {cutoff_dial_, "Filter 1 Cutoff"},
        {resonance_dial_, "Filter 1 Resonance"},
        {volume_dial_, "Master Volume"},
        {attack_dial_, "ENV 1 Attack"},
        {release_dial_, "ENV 1 Release"},
        {lfo_rate_dial_, "LFO 1 Rate"}
    };

    for (auto& binding : bindings) {
        auto dial = binding.first;
        auto param_name = binding.second;
        auto parameter = parameter_binder_->findParameterByName(param_name);
        if (parameter) {
            dial->bindParameter(parameter);
            // Set up value change callback with MIDI output
            dial->setValueChangedCallback([this](uint8_t /* value */, const Parameter* param) {
                if (midi_handler_->isConnected()) {
                    midi_handler_->sendControlChange(1, param->getCCNumber(), param->getCurrentValue());
                }
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
#if SYNTHAPP_DEBUG_UI_CHECKS
            std::cerr << "[UI-ERR] Parameter not found for dial: '" << param_name << "'!" << std::endl;
#else
            std::cout << "Parameter not found: " << param_name << std::endl;
#endif
        }
    }
}

// Legacy dial implementation removed: now fully parameter-aware

void SynthApp::createButtonControls() {
    std::cout << "Creating button controls..." << std::endl;
    
    const auto& config = LayoutManager::getConfig();
    lv_obj_t* screen = lv_screen_active();
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!screen) {
        std::cerr << "[UI-ERR] lv_screen_active() returned nullptr in createButtonControls!" << std::endl;
        return;
    }
#endif
    // Calculate button column position (to the right of the dial grid)
    int btn_x, btn_y;
    int content_width, content_height;
    LayoutManager::getContentArea(&content_width, &content_height);

    // Position buttons for small screens: below all dial rows, centered
    if (LayoutManager::getScreenSize() == LayoutManager::ScreenSize::SMALL) {
        int num_dial_rows = 2; // We have 2 rows of dials
        int grid_start_y = 60;
        int dial_container_height = config.dial_size + 30; // match dial container size
        int dial_area_height = grid_start_y + num_dial_rows * config.dial_spacing_y + dial_container_height;
        btn_y = dial_area_height + 12; // 12px extra spacing below dials
        // Center buttons horizontally
        int content_width, content_height;
        LayoutManager::getContentArea(&content_width, &content_height);
        btn_x = config.margin_x + (content_width - config.button_width) / 2;
    } else {
        // On larger screens, put buttons to the right of the dials
        btn_x = config.margin_x + config.grid_cols * config.dial_spacing_x + 20;
        btn_y = 80; // Start below title
    }
    // Ensure each button is stacked vertically with spacing

    // Create filter enable toggle button
    filter_enable_btn_ = std::make_shared<ButtonControl>(screen, btn_x, btn_y, config.button_width, config.button_height);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!filter_enable_btn_) std::cerr << "[UI-ERR] Failed to create filter_enable_btn_!" << std::endl;
#endif
    filter_enable_btn_->setMode(ButtonControl::ButtonMode::TOGGLE);
    filter_enable_btn_->setText("FILTER");
    filter_enable_btn_->setToggleColors(lv_color_hex(0x444444), lv_color_hex(0x00AA00));
    filter_enable_btn_->setToggleValues(0, 127);
    filter_enable_btn_->setSize(config.button_width, config.button_height);
    filter_enable_btn_->setPosition(btn_x, btn_y);

    // Create LFO sync toggle button
    lfo_sync_btn_ = std::make_shared<ButtonControl>(screen, btn_x, btn_y + config.button_spacing, config.button_width, config.button_height);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!lfo_sync_btn_) std::cerr << "[UI-ERR] Failed to create lfo_sync_btn_!" << std::endl;
#endif
    lfo_sync_btn_->setMode(ButtonControl::ButtonMode::TOGGLE);
    lfo_sync_btn_->setText("LFO SYNC");
    lfo_sync_btn_->setToggleColors(lv_color_hex(0x444444), lv_color_hex(0xFF8800));
    lfo_sync_btn_->setToggleValues(0, 127);
    lfo_sync_btn_->setSize(config.button_width, config.button_height);
    lfo_sync_btn_->setPosition(btn_x, btn_y + config.button_spacing);

    // Create trigger button
    trigger_btn_ = std::make_shared<ButtonControl>(screen, btn_x, btn_y + 2*config.button_spacing, config.button_width, config.button_height);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!trigger_btn_) std::cerr << "[UI-ERR] Failed to create trigger_btn_!" << std::endl;
#endif
    trigger_btn_->setMode(ButtonControl::ButtonMode::TRIGGER);
    trigger_btn_->setText("TRIGGER");
    trigger_btn_->setColors(lv_color_hex(0x666666), lv_color_hex(0xFF0000));
    trigger_btn_->setTriggerValue(127, 0);
    trigger_btn_->setSize(config.button_width, config.button_height);
    trigger_btn_->setPosition(btn_x, btn_y + 2*config.button_spacing);

    // Try to bind to parameters (these may not exist in the demo set)
    auto filter_param = parameter_binder_->findParameterByName("Filter 1 Enable");
    if (filter_param) {
        filter_enable_btn_->bindParameter(filter_param);
        std::cout << "Bound filter enable button to parameter" << std::endl;
    } else {
#if SYNTHAPP_DEBUG_UI_CHECKS
        std::cerr << "[UI-ERR] Parameter not found for filter_enable_btn_: 'Filter 1 Enable'!" << std::endl;
#endif
    }

    auto lfo_param = parameter_binder_->findParameterByName("LFO 1 Sync");
    if (lfo_param) {
        lfo_sync_btn_->bindParameter(lfo_param);
        std::cout << "Bound LFO sync button to parameter" << std::endl;
    } else {
#if SYNTHAPP_DEBUG_UI_CHECKS
        std::cerr << "[UI-ERR] Parameter not found for lfo_sync_btn_: 'LFO 1 Sync'!" << std::endl;
#endif
    }

    auto trigger_param = parameter_binder_->findParameterByName("Trigger");
    if (trigger_param) {
        trigger_btn_->bindParameter(trigger_param);
        std::cout << "Bound trigger button to parameter" << std::endl;
    } else {
#if SYNTHAPP_DEBUG_UI_CHECKS
        std::cerr << "[UI-ERR] Parameter not found for trigger_btn_: 'Trigger'!" << std::endl;
#endif
    }

    // Set up callbacks for all buttons
    filter_enable_btn_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
        if (midi_handler_->isConnected() && param) {
            midi_handler_->sendControlChange(1, param->getCCNumber(), value);
        }
        std::cout << "Filter Enable: " << (value > 63 ? "ON" : "OFF") << std::endl;
    });

    lfo_sync_btn_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
        if (midi_handler_->isConnected() && param) {
            midi_handler_->sendControlChange(1, param->getCCNumber(), value);
        }
        std::cout << "LFO Sync: " << (value > 63 ? "ON" : "OFF") << std::endl;
    });

    trigger_btn_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
        if (midi_handler_->isConnected() && param) {
            midi_handler_->sendControlChange(1, param->getCCNumber(), value);
        }
        std::cout << "Trigger: " << static_cast<int>(value) << std::endl;
    });
}

void SynthApp::createUndoRedoControls() {
    std::cout << "Creating undo/redo controls..." << std::endl;
    
    const auto& config = LayoutManager::getConfig();
    lv_obj_t* screen = lv_screen_active();
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!screen) {
        std::cerr << "[UI-ERR] lv_screen_active() returned nullptr in createUndoRedoControls!" << std::endl;
        return;
    }
#endif
    // Position undo/redo buttons responsively
    int undo_x, undo_y;
    int content_width, content_height;
    LayoutManager::getContentArea(&content_width, &content_height);

    // Position based on screen size
    if (LayoutManager::getScreenSize() == LayoutManager::ScreenSize::SMALL) {
        // Small screen: bottom left corner
        undo_x = config.margin_x;
        undo_y = content_height - config.button_height - config.margin_y;
    } else {
        // Larger screens: bottom area
        undo_x = config.margin_x + 100;
        undo_y = content_height - config.button_height - 20;
    }

    // Create undo button
    undo_btn_ = lv_btn_create(screen);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!undo_btn_) std::cerr << "[UI-ERR] Failed to create undo_btn_!" << std::endl;
#endif
    lv_obj_set_size(undo_btn_, config.button_width, config.button_height);
    lv_obj_set_pos(undo_btn_, undo_x, undo_y);

    // Style the undo button
    lv_obj_set_style_bg_color(undo_btn_, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_color(undo_btn_, lv_color_hex(0x888888), 0);
    lv_obj_set_style_border_width(undo_btn_, 1, 0);
    lv_obj_set_style_radius(undo_btn_, 4, 0);

    // Undo button label
    lv_obj_t* undo_label = lv_label_create(undo_btn_);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!undo_label) std::cerr << "[UI-ERR] Failed to create undo_label!" << std::endl;
#endif
    lv_label_set_text(undo_label, "UNDO");
    lv_obj_set_style_text_color(undo_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_center(undo_label);
    undo_label_ = undo_label;

    // Create redo button
    redo_btn_ = lv_btn_create(screen);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!redo_btn_) std::cerr << "[UI-ERR] Failed to create redo_btn_!" << std::endl;
#endif
    lv_obj_set_size(redo_btn_, config.button_width, config.button_height);
    lv_obj_set_pos(redo_btn_, undo_x + config.button_width + 10, undo_y);

    // Style the redo button
    lv_obj_set_style_bg_color(redo_btn_, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_color(redo_btn_, lv_color_hex(0x888888), 0);
    lv_obj_set_style_border_width(redo_btn_, 1, 0);
    lv_obj_set_style_radius(redo_btn_, 4, 0);

    // Redo button label
    lv_obj_t* redo_label = lv_label_create(redo_btn_);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!redo_label) std::cerr << "[UI-ERR] Failed to create redo_label!" << std::endl;
#endif
    lv_label_set_text(redo_label, "REDO");
    lv_obj_set_style_text_color(redo_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_center(redo_label);
    redo_label_ = redo_label;

    // Add event callbacks
    lv_obj_add_event_cb(undo_btn_, [](lv_event_t* e) {
        SynthApp* app = static_cast<SynthApp*>(lv_event_get_user_data(e));
        app->handleUndo();
    }, LV_EVENT_CLICKED, this);

    lv_obj_add_event_cb(redo_btn_, [](lv_event_t* e) {
        SynthApp* app = static_cast<SynthApp*>(lv_event_get_user_data(e));
        app->handleRedo();
    }, LV_EVENT_CLICKED, this);

    // Create simulate button (only on larger screens)
    if (LayoutManager::getScreenSize() != LayoutManager::ScreenSize::SMALL) {
        lv_obj_t* sim_button = lv_btn_create(screen);
#if SYNTHAPP_DEBUG_UI_CHECKS
        if (!sim_button) std::cerr << "[UI-ERR] Failed to create sim_button!" << std::endl;
#endif
        lv_obj_set_size(sim_button, LayoutManager::scaleSize(200), config.button_height);
        lv_obj_align(sim_button, LV_ALIGN_BOTTOM_MID, 0, -config.status_height - config.button_height - 10);

        // Style the button for 8-bit look
        lv_obj_set_style_bg_color(sim_button, lv_color_hex(0x333333), 0);
        lv_obj_set_style_border_color(sim_button, lv_color_hex(0x00FF00), 0);
        lv_obj_set_style_border_width(sim_button, 2, 0);
        lv_obj_set_style_radius(sim_button, 4, 0);

        // Button label
        lv_obj_t* btn_label = lv_label_create(sim_button);
#if SYNTHAPP_DEBUG_UI_CHECKS
        if (!btn_label) std::cerr << "[UI-ERR] Failed to create sim_button label!" << std::endl;
#endif
        lv_label_set_text(btn_label, "Simulate Values");
        lv_obj_set_style_text_color(btn_label, lv_color_hex(0x00FF00), 0);
        lv_obj_center(btn_label);

        // Add button click event
        lv_obj_add_event_cb(sim_button, [](lv_event_t* e) {
            SynthApp* app = static_cast<SynthApp*>(lv_event_get_user_data(e));
            app->simulateMidiCC();
        }, LV_EVENT_CLICKED, this);
    }

    // Initialize button states
    updateUndoRedoButtons();
}

void SynthApp::createStatusArea() {
    const auto& config = LayoutManager::getConfig();
    lv_obj_t* screen = lv_screen_active();
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!screen) {
        std::cerr << "[UI-ERR] lv_screen_active() returned nullptr in createStatusArea!" << std::endl;
        return;
    }
#endif
    // Calculate status area size and position
    int content_width, content_height;
    LayoutManager::getContentArea(&content_width, &content_height);

    int status_width = content_width;
    int status_height = config.status_height;

    // Create status/info area at the bottom
    lv_obj_t* status_area = lv_obj_create(screen);
    // Make sure status area is not clickable and is in the background
    lv_obj_clear_flag(status_area, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_background(status_area);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!status_area) {
        std::cerr << "[UI-ERR] Failed to create status_area!" << std::endl;
        return;
    }
#endif
    lv_obj_set_size(status_area, status_width, status_height);
    lv_obj_align(status_area, LV_ALIGN_BOTTOM_MID, 0, -config.margin_y);
    lv_obj_set_style_bg_color(status_area, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_color(status_area, lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_width(status_area, 1, 0);
    lv_obj_set_style_radius(status_area, 4, 0);

    // Status label inside the area
    lv_obj_t* status_label = lv_label_create(status_area);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!status_label) {
        std::cerr << "[UI-ERR] Failed to create status_label!" << std::endl;
        return;
    }
#endif
    if (LayoutManager::getScreenSize() == LayoutManager::ScreenSize::SMALL) {
        lv_label_set_text(status_label, "Parameter System Ready");
    } else {
        lv_label_set_text(status_label, "Ready - Compare NEW vs OLD dial systems");
    }
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFF00), 0);

    // Use appropriate font size
    if (LayoutManager::getScreenSize() == LayoutManager::ScreenSize::SMALL) {
        lv_obj_set_style_text_font(status_label, &lv_font_montserrat_10, 0);
    } else {
        lv_obj_set_style_text_font(status_label, &lv_font_montserrat_10, 0);
    }
    lv_obj_center(status_label);

    // Store status label for updates
    status_label_ = status_label;
}

void SynthApp::updateUndoRedoButtons() {
    if (!command_manager_) return;
    if (!undo_btn_ || !redo_btn_ || !undo_label_ || !redo_label_) {
#if SYNTHAPP_DEBUG_UI_CHECKS
        std::cerr << "[UI-ERR] Undo/redo button or label is null in updateUndoRedoButtons!" << std::endl;
#endif
        return;
    }

    // Update undo button state
    bool can_undo = command_manager_->canUndo();
    lv_obj_set_style_bg_color(undo_btn_, 
                             can_undo ? lv_color_hex(0x004400) : lv_color_hex(0x333333), 
                             LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(undo_label_, 
                               can_undo ? lv_color_hex(0x00FF00) : lv_color_hex(0x666666), 
                               LV_STATE_DEFAULT);

    // Update redo button state
    bool can_redo = command_manager_->canRedo();
    lv_obj_set_style_bg_color(redo_btn_, 
                             can_redo ? lv_color_hex(0x444400) : lv_color_hex(0x333333), 
                             LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(redo_label_, 
                               can_redo ? lv_color_hex(0xFFFF00) : lv_color_hex(0x666666), 
                               LV_STATE_DEFAULT);

    // Update status with undo/redo info
    if (status_label_) {
        char status_text[150];
        snprintf(status_text, sizeof(status_text), 
                "Undo: %s | Redo: %s", 
                command_manager_->getUndoDescription().c_str(),
                command_manager_->getRedoDescription().c_str());
        lv_label_set_text(status_label_, status_text);
    }
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
    
    // Legacy dials removed
    
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

void SynthApp::handleUndo() {
    if (command_manager_) {
        command_manager_->undo();
        updateUndoRedoButtons();
    }
}

void SynthApp::handleRedo() {
    if (command_manager_) {
        command_manager_->redo();
        updateUndoRedoButtons();
    }
}