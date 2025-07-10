
#include <lvgl.h>
// Global root container for LVGL UI
lv_obj_t* root_container = nullptr;
// Update SynthApp to use the modular display driver
#include "SynthApp.h"
#include "components/controls/ParameterControl.h"
#include "components/parameter/ParameterBinder.h"
#include "components/parameter/CommandManager.h"
#include "components/controls/ButtonControl.h"
#include "components/controls/DialControl.h"
#include "components/layout/LayoutManager.h"
#include "FontConfig.h"
#include "Constants.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>

#if defined(ESP32_BUILD)
    #include "../../hardware/ESP32Display.h"
    extern uint32_t millis_cb();
#endif

SynthApp::SynthApp() 
    : initialized_(false)
    , root_container_(nullptr)
{
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
    
    // Initialize parameter system
    parameter_binder_ = std::make_unique<ParameterBinder>();
    command_manager_ = std::make_unique<CommandManager>();
    
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
        // Will be handled by MainControlTab
    });
    
    initHardware();
    initWindowManager();
    
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
    // Set to ESP32's native resolution for parity
    lv_display_t* disp = lv_sdl_window_create(SynthConstants::ESP32_SCREEN_WIDTH, SynthConstants::ESP32_SCREEN_HEIGHT);
    if (!disp) {
        std::cerr << "Failed to create SDL window!" << std::endl;
        return;
    }
    
    // Create SDL mouse input
    lv_sdl_mouse_create();
    
    // Optional: Create SDL keyboard input
    lv_sdl_keyboard_create();
    
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

    lv_obj_set_scrollbar_mode(lv_layer_top(), LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollbar_mode(lv_layer_sys(), LV_SCROLLBAR_MODE_OFF);

    // Create a fixed-size app container centered in the screen for pixel-perfect ESP32 parity
    lv_obj_t* app_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(app_container, SynthConstants::ESP32_SCREEN_WIDTH, SynthConstants::ESP32_SCREEN_HEIGHT);
    lv_obj_center(app_container);
    lv_obj_set_style_bg_color(app_container, lv_color_hex(SynthConstants::Color::BG), 0);
    lv_obj_set_style_border_color(app_container, lv_color_hex(0xFFFFFF), 0); // Debug border
    lv_obj_set_style_border_width(app_container, 2, 0);
    lv_obj_set_flex_flow(app_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(app_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(app_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(app_container, LV_SCROLLBAR_MODE_OFF);

    // Use app_container as the new root_container for all UI elements
    extern lv_obj_t* root_container;
    root_container = app_container;
    lv_scr_load(lv_scr_act()); // Show the screen (app_container is child of screen)

    // --- FLEX CHILD ORDER: title, help, content, undo/redo, status ---
    // 1. Title label
    lv_obj_t* title = lv_label_create(root_container);
    lv_label_set_text(title, SynthConstants::Text::TITLE);
    lv_obj_set_style_text_color(title, lv_color_hex(SynthConstants::Color::TITLE), 0);
    lv_obj_set_style_text_font(title, FontA.small, 0);
    // No align! Let flex layout handle it

    // 2. Help label (if not small screen)
    if (LayoutManager::getScreenSize() != LayoutManager::ScreenSize::SMALL) {
        lv_obj_t* help_label = lv_label_create(root_container);
        lv_label_set_text(help_label, SynthConstants::Text::HELP);
        lv_obj_set_style_text_color(help_label, lv_color_hex(SynthConstants::Color::HELP), 0);
        lv_obj_set_style_text_font(help_label, FontA.med, 0);
    }

    // 3. Main content row: dials (flex row) + button column (flex col)
    lv_obj_t* content_row = lv_obj_create(root_container);
    lv_obj_set_size(content_row, lv_pct(100), lv_pct(65));
    lv_obj_set_flex_flow(content_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(content_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_border_color(content_row, lv_color_hex(0xFF00FF), 0); // Debug border
    lv_obj_set_style_border_width(content_row, 1, 0);
    lv_obj_set_style_pad_all(content_row, 4, 0);
    // Disable scrollbars for this container (unless scrolling is needed)
    lv_obj_clear_flag(content_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(content_row, LV_SCROLLBAR_MODE_OFF);

    // Dials flex row
    lv_obj_t* dials_row = lv_obj_create(content_row);
    lv_obj_set_flex_flow(dials_row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(dials_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_border_color(dials_row, lv_color_hex(0x00FFFF), 0); // Debug border
    lv_obj_set_style_border_width(dials_row, 1, 0);
    lv_obj_set_style_pad_all(dials_row, 2, 0);
    lv_obj_set_size(dials_row, lv_pct(70), lv_pct(100));
    // Disable scrollbars for this container (unless scrolling is needed)
    lv_obj_clear_flag(dials_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(dials_row, LV_SCROLLBAR_MODE_OFF);

    // Button column (flex col)
    lv_obj_t* button_col = lv_obj_create(content_row);
    lv_obj_set_flex_flow(button_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(button_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_border_color(button_col, lv_color_hex(0xFF8800), 0); // Debug border
    lv_obj_set_style_border_width(button_col, 1, 0);
    lv_obj_set_style_pad_all(button_col, 2, 0);
    lv_obj_set_size(button_col, lv_pct(30), lv_pct(100));
    // Disable scrollbars for this container (unless scrolling is needed)
    lv_obj_clear_flag(button_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(button_col, LV_SCROLLBAR_MODE_OFF);

    // Store for use in child creation
    this->dials_row_ = dials_row;
    this->button_col_ = button_col;

    // 4. Undo/redo and simulate controls (bottom of root container)
    createParameterDialsFlex();
    createButtonControlsFlex();
    createUndoRedoControlsFlex();

    // 5. Status area (bottom)
    createStatusAreaFlex();
    // ...all UI creation now handled by flex-based functions above...
}

void SynthApp::createParameterDialsFlex() {
    // Use dials_row_ as parent
    const auto& config = LayoutManager::getConfig();
    lv_obj_t* parent = this->dials_row_;
    // Create dials as children of dials_row_ (flex row)
    std::vector<std::pair<std::shared_ptr<DialControl>*, std::string>> dials = {
        {&cutoff_dial_, "Filter 1 Cutoff"},
        {&resonance_dial_, "Filter 1 Resonance"},
        {&volume_dial_, "Master Volume"},
        {&attack_dial_, "ENV 1 Attack"},
        {&release_dial_, "ENV 1 Release"},
        {&lfo_rate_dial_, "LFO 1 Rate"}
    };
    // Store dial objects and names for post-layout debug logging
    struct DialDebugInfo {
        lv_obj_t* obj;
        std::string name;
    };
    std::vector<DialDebugInfo> dial_debug_infos;
    for (auto& d : dials) {
        *(d.first) = std::make_shared<DialControl>(parent, 0, 0);
        auto param = parameter_binder_->findParameterByName(d.second);
        if (param) {
            (*(d.first))->bindParameter(param);
        }
        // Set dial size and enforce fixed size in flex layout
        (*(d.first))->setDialSize(config.dial_size);
        (*(d.first))->setSize(config.dial_size + 20, config.dial_size + 30);
        // Enforce fixed size for flex: get the root LVGL object and set min/max/flex grow
        if ((*(d.first))->getObject()) {
            lv_obj_t* dial_obj = (*(d.first))->getObject();
            int w = config.dial_size + 20;
            int h = config.dial_size + 30;
            lv_obj_set_style_min_width(dial_obj, w, 0);
            lv_obj_set_style_max_width(dial_obj, w, 0);
            lv_obj_set_style_min_height(dial_obj, h, 0);
            lv_obj_set_style_max_height(dial_obj, h, 0);
            lv_obj_set_flex_grow(dial_obj, 0);
            // Store for post-layout debug logging
            dial_debug_infos.push_back({dial_obj, d.second});
        }
    }
    // Defer debug logging until after LVGL layout is performed
    struct DebugInfo {
        std::vector<DialDebugInfo> dials;
        lv_obj_t* parent;
        int tries = 0;
    };
    DebugInfo* dbg = new DebugInfo{dial_debug_infos, parent, 0};
    lv_timer_t* log_timer = lv_timer_create_basic();
    lv_timer_set_period(log_timer, 50); // 50ms poll interval
    lv_timer_set_repeat_count(log_timer, -1); // repeat until we stop it
    lv_timer_set_user_data(log_timer, dbg);
    lv_timer_set_cb(log_timer, [](lv_timer_t* timer) {
        DebugInfo* dbg = static_cast<DebugInfo*>(lv_timer_get_user_data(timer));
        bool all_ready = true;
        for (const auto& info : dbg->dials) {
            int w = lv_obj_get_width(info.obj);
            int h = lv_obj_get_height(info.obj);
            if (w == 0 || h == 0) all_ready = false;
        }
        int flex_w = lv_obj_get_width(dbg->parent);
        int flex_h = lv_obj_get_height(dbg->parent);
        if (flex_w == 0 || flex_h == 0) all_ready = false;
        dbg->tries++;
        if (all_ready || dbg->tries > 10) { // 10*50ms = 500ms max
            for (const auto& info : dbg->dials) {
                int dial_actual_w = lv_obj_get_width(info.obj);
                int dial_actual_h = lv_obj_get_height(info.obj);
                lv_obj_t* parent_obj = lv_obj_get_parent(info.obj);
                int parent_actual_w = parent_obj ? lv_obj_get_width(parent_obj) : -1;
                int parent_actual_h = parent_obj ? lv_obj_get_height(parent_obj) : -1;
                std::cout << "[DEBUG] (POST-LAYOUT) Dial '" << info.name << "' size: " << dial_actual_w << "x" << dial_actual_h
                          << ", parent flex container size: " << parent_actual_w << "x" << parent_actual_h << std::endl;
            }
            std::cout << "[DEBUG] (POST-LAYOUT) Dials flex container total size: " << flex_w << "x" << flex_h << std::endl;
            delete dbg;
            lv_timer_del(timer);
        }
    });
}

void SynthApp::createButtonControlsFlex() {
    // Use button_col_ as parent
    const auto& config = LayoutManager::getConfig();
    lv_obj_t* parent = this->button_col_;
    filter_enable_btn_ = std::make_shared<ButtonControl>(parent, 0, 0, config.button_width, config.button_height);
    filter_enable_btn_->setMode(ButtonControl::ButtonMode::TOGGLE);
    filter_enable_btn_->setLabel(SynthConstants::Text::BTN_FILTER);
    filter_enable_btn_->setToggleColors(lv_color_hex(SynthConstants::Color::BTN_FILTER_OFF), lv_color_hex(SynthConstants::Color::BTN_FILTER_ON));
    filter_enable_btn_->setToggleValues(SynthConstants::Midi::BTN_TOGGLE_OFF, SynthConstants::Midi::BTN_TOGGLE_ON);
    filter_enable_btn_->setSize(config.button_width, config.button_height);

    lfo_sync_btn_ = std::make_shared<ButtonControl>(parent, 0, 0, config.button_width, config.button_height);
    lfo_sync_btn_->setMode(ButtonControl::ButtonMode::TOGGLE);
    lfo_sync_btn_->setLabel(SynthConstants::Text::BTN_LFO);
    lfo_sync_btn_->setToggleColors(lv_color_hex(SynthConstants::Color::BTN_LFO_OFF), lv_color_hex(SynthConstants::Color::BTN_LFO_ON));
    lfo_sync_btn_->setToggleValues(SynthConstants::Midi::BTN_TOGGLE_OFF, SynthConstants::Midi::BTN_TOGGLE_ON);
    lfo_sync_btn_->setSize(config.button_width, config.button_height);

    trigger_btn_ = std::make_shared<ButtonControl>(parent, 0, 0, config.button_width, config.button_height);
    trigger_btn_->setMode(ButtonControl::ButtonMode::TRIGGER);
    trigger_btn_->setLabel(SynthConstants::Text::BTN_TRIGGER);
    trigger_btn_->setColors(lv_color_hex(SynthConstants::Color::BTN_TRIGGER_OFF), lv_color_hex(SynthConstants::Color::BTN_TRIGGER_ON));
    trigger_btn_->setTriggerValue(SynthConstants::Midi::BTN_TRIGGER_ON, SynthConstants::Midi::BTN_TRIGGER_OFF);
    trigger_btn_->setSize(config.button_width, config.button_height);

    // Bind parameters if available
    auto filter_param = parameter_binder_->findParameterByName("Filter 1 Enable");
    if (filter_param) filter_enable_btn_->bindParameter(filter_param);
    auto lfo_param = parameter_binder_->findParameterByName("LFO 1 Sync");
    if (lfo_param) lfo_sync_btn_->bindParameter(lfo_param);
    auto trigger_param = parameter_binder_->findParameterByName("Trigger");
    if (trigger_param) trigger_btn_->bindParameter(trigger_param);

    // Set up callbacks for all buttons
    filter_enable_btn_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
        if (midi_handler_->isConnected() && param) {
            midi_handler_->sendControlChange(SynthConstants::Midi::CHANNEL, param->getCCNumber(), value);
        }
        std::cout << "Filter Enable: " << (value > 63 ? "ON" : "OFF") << std::endl;
    });
    lfo_sync_btn_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
        if (midi_handler_->isConnected() && param) {
            midi_handler_->sendControlChange(SynthConstants::Midi::CHANNEL, param->getCCNumber(), value);
        }
        std::cout << "LFO Sync: " << (value > 63 ? "ON" : "OFF") << std::endl;
    });
    trigger_btn_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
        if (midi_handler_->isConnected() && param) {
            midi_handler_->sendControlChange(SynthConstants::Midi::CHANNEL, param->getCCNumber(), value);
        }
        std::cout << "Trigger: " << static_cast<int>(value) << std::endl;
    });
}

void SynthApp::createUndoRedoControlsFlex() {
    // Place undo/redo and simulate buttons in a flex row at the bottom of root_container
    lv_obj_t* bottom_row = lv_obj_create(root_container);
    lv_obj_set_flex_flow(bottom_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_border_color(bottom_row, lv_color_hex(0x00FF00), 0); // Debug border
    lv_obj_set_style_border_width(bottom_row, 1, 0);
    lv_obj_set_style_pad_all(bottom_row, 2, 0);
    lv_obj_set_size(bottom_row, lv_pct(100), lv_pct(10));
    // Disable scrollbars for this container (unless scrolling is needed)
    lv_obj_clear_flag(bottom_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(bottom_row, LV_SCROLLBAR_MODE_OFF);

    const auto& config = LayoutManager::getConfig();
    // Undo button
    undo_btn_ = lv_btn_create(bottom_row);
    lv_obj_set_size(undo_btn_, config.button_width, config.button_height);
    lv_obj_set_style_bg_color(undo_btn_, lv_color_hex(SynthConstants::Color::BTN_UNDO_BG), 0);
    lv_obj_set_style_border_color(undo_btn_, lv_color_hex(SynthConstants::Color::BTN_UNDO_BORDER), 0);
    lv_obj_set_style_border_width(undo_btn_, SynthConstants::Layout::STATUS_BORDER_WIDTH, 0);
    lv_obj_set_style_radius(undo_btn_, SynthConstants::Layout::STATUS_RADIUS, 0);
    lv_obj_t* undo_label = lv_label_create(undo_btn_);
    lv_label_set_text(undo_label, SynthConstants::Text::BTN_UNDO);
    lv_obj_set_style_text_color(undo_label, lv_color_hex(SynthConstants::Color::HELP), 0);
    lv_obj_set_style_text_font(undo_label, FontA.small, 0);
    lv_obj_center(undo_label);
    undo_label_ = undo_label;
    lv_obj_add_event_cb(undo_btn_, [](lv_event_t* e) {
        SynthApp* app = static_cast<SynthApp*>(lv_event_get_user_data(e));
        app->handleUndo();
    }, LV_EVENT_CLICKED, this);

    // Redo button
    redo_btn_ = lv_btn_create(bottom_row);
    lv_obj_set_size(redo_btn_, config.button_width, config.button_height);
    lv_obj_set_style_bg_color(redo_btn_, lv_color_hex(SynthConstants::Color::BTN_REDO_BG), 0);
    lv_obj_set_style_border_color(redo_btn_, lv_color_hex(SynthConstants::Color::BTN_REDO_BORDER), 0);
    lv_obj_set_style_border_width(redo_btn_, SynthConstants::Layout::STATUS_BORDER_WIDTH, 0);
    lv_obj_set_style_radius(redo_btn_, SynthConstants::Layout::STATUS_RADIUS, 0);
    lv_obj_t* redo_label = lv_label_create(redo_btn_);
    lv_label_set_text(redo_label, SynthConstants::Text::BTN_REDO);
    lv_obj_set_style_text_color(redo_label, lv_color_hex(SynthConstants::Color::HELP), 0);
    lv_obj_set_style_text_font(redo_label, FontA.small, 0);
    lv_obj_center(redo_label);
    redo_label_ = redo_label;
    lv_obj_add_event_cb(redo_btn_, [](lv_event_t* e) {
        SynthApp* app = static_cast<SynthApp*>(lv_event_get_user_data(e));
        app->handleRedo();
    }, LV_EVENT_CLICKED, this);

    // Simulate button (always visible)
    lv_obj_t* sim_button = lv_btn_create(bottom_row);
    lv_obj_set_size(sim_button, LayoutManager::scaleSize(SynthConstants::Layout::SIM_BTN_WIDTH), config.button_height);
    lv_obj_set_style_bg_color(sim_button, lv_color_hex(SynthConstants::Color::SIM_BTN_BG), 0);
    lv_obj_set_style_border_color(sim_button, lv_color_hex(SynthConstants::Color::SIM_BTN_BORDER), 0);
    lv_obj_set_style_border_width(sim_button, 2, 0);
    lv_obj_set_style_radius(sim_button, SynthConstants::Layout::STATUS_RADIUS, 0);
    lv_obj_t* btn_label = lv_label_create(sim_button);
    lv_label_set_text(btn_label, SynthConstants::Text::BTN_SIMULATE);
    lv_obj_set_style_text_color(btn_label, lv_color_hex(SynthConstants::Color::SIM_BTN_TEXT), 0);
    lv_obj_set_style_text_font(btn_label, FontA.small, 0);
    lv_obj_center(btn_label);
    lv_obj_add_event_cb(sim_button, [](lv_event_t* e) {
        SynthApp* app = static_cast<SynthApp*>(lv_event_get_user_data(e));
        app->simulateMidiCC();
    }, LV_EVENT_CLICKED, this);

    updateUndoRedoButtons();
}

void SynthApp::createStatusAreaFlex() {
    // Status area at the very bottom of root_container
    lv_obj_t* status_area = lv_obj_create(root_container);
    lv_obj_set_size(status_area, lv_pct(100), lv_pct(10));
    lv_obj_set_style_bg_color(status_area, lv_color_hex(SynthConstants::Color::STATUS_BG), 0);
    lv_obj_set_style_border_color(status_area, lv_color_hex(SynthConstants::Color::STATUS_BORDER), 0);
    lv_obj_set_style_border_width(status_area, SynthConstants::Layout::STATUS_BORDER_WIDTH, 0);
    lv_obj_set_style_radius(status_area, SynthConstants::Layout::STATUS_RADIUS, 0);
    lv_obj_set_style_pad_all(status_area, 4, 0);
    lv_obj_clear_flag(status_area, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_background(status_area);
    // Disable scrollbars for this container (unless scrolling is needed)
    lv_obj_clear_flag(status_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(status_area, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(status_area, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_area, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* status_label = lv_label_create(status_area);
    if (LayoutManager::getScreenSize() == LayoutManager::ScreenSize::SMALL) {
        lv_label_set_text(status_label, SynthConstants::Text::STATUS_READY);
    } else {
        lv_label_set_text(status_label, SynthConstants::Text::STATUS_READY_LG);
    }
    lv_obj_set_style_text_color(status_label, lv_color_hex(SynthConstants::Color::STATUS), 0);
    lv_obj_set_style_text_font(status_label, FontA.small, 0);
    lv_obj_center(status_label);
    status_label_ = status_label;
}


void SynthApp::createParameterDials() {
    std::cout << "Creating NEW parameter-aware dials..." << std::endl;
    
    const auto& config = LayoutManager::getConfig();
    extern lv_obj_t* root_container;
    lv_obj_t* screen = root_container;
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!screen) {
        std::cerr << "[UI-ERR] lv_screen_active() returned nullptr in createParameterDials!" << std::endl;
        return;
    }
#endif
    // Calculate starting position for the dial grid
    int grid_start_y = SynthConstants::Layout::TITLE_Y_SMALL; // Title area
    if (LayoutManager::getScreenSize() != LayoutManager::ScreenSize::SMALL) {
        grid_start_y = SynthConstants::Layout::TITLE_Y_LARGE; // Account for help text on larger screens
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
    cutoff_dial_->setColor(lv_color_hex(SynthConstants::Color::DIAL_GREEN));  // Green
    cutoff_dial_->setDialSize(config.dial_size);
    cutoff_dial_->setSize(config.dial_size + 20, config.dial_size + 30);
    cutoff_dial_->setPosition(dial_x, dial_y);

    LayoutManager::getGridPosition(1, 0, &dial_x, &dial_y);
    dial_y += grid_start_y;
    resonance_dial_ = std::make_shared<DialControl>(screen, dial_x, dial_y);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!resonance_dial_) std::cerr << "[UI-ERR] Failed to create resonance_dial_!" << std::endl;
#endif
    resonance_dial_->setColor(lv_color_hex(SynthConstants::Color::DIAL_ORANGE));  // Orange
    resonance_dial_->setDialSize(config.dial_size);
    resonance_dial_->setSize(config.dial_size + 20, config.dial_size + 30);
    resonance_dial_->setPosition(dial_x, dial_y);

    LayoutManager::getGridPosition(2, 0, &dial_x, &dial_y);
    dial_y += grid_start_y;
    volume_dial_ = std::make_shared<DialControl>(screen, dial_x, dial_y);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!volume_dial_) std::cerr << "[UI-ERR] Failed to create volume_dial_!" << std::endl;
#endif
    volume_dial_->setColor(lv_color_hex(SynthConstants::Color::DIAL_BLUE));  // Blue
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
    attack_dial_->setColor(lv_color_hex(SynthConstants::Color::DIAL_MAGENTA));  // Magenta
    attack_dial_->setDialSize(config.dial_size);
    attack_dial_->setSize(config.dial_size + 20, config.dial_size + 30);
    attack_dial_->setPosition(dial_x, dial_y);

    LayoutManager::getGridPosition(1, 1, &dial_x, &dial_y);
    dial_y += grid_start_y;
    release_dial_ = std::make_shared<DialControl>(screen, dial_x, dial_y);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!release_dial_) std::cerr << "[UI-ERR] Failed to create release_dial_!" << std::endl;
#endif
    release_dial_->setColor(lv_color_hex(SynthConstants::Color::DIAL_YELLOW_GREEN));  // Yellow-green
    release_dial_->setDialSize(config.dial_size);
    release_dial_->setSize(config.dial_size + 20, config.dial_size + 30);
    release_dial_->setPosition(dial_x, dial_y);

    LayoutManager::getGridPosition(2, 1, &dial_x, &dial_y);
    dial_y += grid_start_y;
    lfo_rate_dial_ = std::make_shared<DialControl>(screen, dial_x, dial_y);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!lfo_rate_dial_) std::cerr << "[UI-ERR] Failed to create lfo_rate_dial_!" << std::endl;
#endif
    lfo_rate_dial_->setColor(lv_color_hex(SynthConstants::Color::DIAL_PINK));  // Pink
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
            midi_handler_->sendControlChange(SynthConstants::Midi::CHANNEL, param->getCCNumber(), param->getCurrentValue());
        }
        char status_text[SynthConstants::Buffer::STATUS];
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
    extern lv_obj_t* root_container;
    lv_obj_t* screen = root_container;
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
        btn_x = config.margin_x + ((content_width - config.button_width) / 2);
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
    filter_enable_btn_->setLabel(SynthConstants::Text::BTN_FILTER);
    filter_enable_btn_->setToggleColors(lv_color_hex(SynthConstants::Color::BTN_FILTER_OFF), lv_color_hex(SynthConstants::Color::BTN_FILTER_ON));
    filter_enable_btn_->setToggleValues(SynthConstants::Midi::BTN_TOGGLE_OFF, SynthConstants::Midi::BTN_TOGGLE_ON);
    filter_enable_btn_->setSize(config.button_width, config.button_height);
    filter_enable_btn_->setPosition(btn_x, btn_y);

    // Create LFO sync toggle button
    lfo_sync_btn_ = std::make_shared<ButtonControl>(screen, btn_x, btn_y + config.button_spacing, config.button_width, config.button_height);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!lfo_sync_btn_) std::cerr << "[UI-ERR] Failed to create lfo_sync_btn_!" << std::endl;
#endif
    lfo_sync_btn_->setMode(ButtonControl::ButtonMode::TOGGLE);
    lfo_sync_btn_->setLabel(SynthConstants::Text::BTN_LFO);
    lfo_sync_btn_->setToggleColors(lv_color_hex(SynthConstants::Color::BTN_LFO_OFF), lv_color_hex(SynthConstants::Color::BTN_LFO_ON));
    lfo_sync_btn_->setToggleValues(SynthConstants::Midi::BTN_TOGGLE_OFF, SynthConstants::Midi::BTN_TOGGLE_ON);
    lfo_sync_btn_->setSize(config.button_width, config.button_height);
    lfo_sync_btn_->setPosition(btn_x, btn_y + config.button_spacing);

    // Create trigger button
    trigger_btn_ = std::make_shared<ButtonControl>(screen, btn_x, btn_y + 2*config.button_spacing, config.button_width, config.button_height);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!trigger_btn_) std::cerr << "[UI-ERR] Failed to create trigger_btn_!" << std::endl;
#endif
    trigger_btn_->setMode(ButtonControl::ButtonMode::TRIGGER);
    trigger_btn_->setLabel(SynthConstants::Text::BTN_TRIGGER);
    trigger_btn_->setColors(lv_color_hex(SynthConstants::Color::BTN_TRIGGER_OFF), lv_color_hex(SynthConstants::Color::BTN_TRIGGER_ON));
    trigger_btn_->setTriggerValue(SynthConstants::Midi::BTN_TRIGGER_ON, SynthConstants::Midi::BTN_TRIGGER_OFF);
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
            midi_handler_->sendControlChange(SynthConstants::Midi::CHANNEL, param->getCCNumber(), value);
        }
        std::cout << "Filter Enable: " << (value > 63 ? "ON" : "OFF") << std::endl;
    });

    lfo_sync_btn_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
        if (midi_handler_->isConnected() && param) {
            midi_handler_->sendControlChange(SynthConstants::Midi::CHANNEL, param->getCCNumber(), value);
        }
        std::cout << "LFO Sync: " << (value > 63 ? "ON" : "OFF") << std::endl;
    });

    trigger_btn_->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
        if (midi_handler_->isConnected() && param) {
            midi_handler_->sendControlChange(SynthConstants::Midi::CHANNEL, param->getCCNumber(), value);
        }
        std::cout << "Trigger: " << static_cast<int>(value) << std::endl;
    });
}

void SynthApp::createUndoRedoControls() {
    std::cout << "Creating undo/redo controls..." << std::endl;
    
    const auto& config = LayoutManager::getConfig();
    extern lv_obj_t* root_container;
    lv_obj_t* screen = root_container;
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
        undo_x = config.margin_x + SynthConstants::Layout::UNDO_X_LG;
        undo_y = content_height - config.button_height - SynthConstants::Layout::UNDO_Y_LG;
    }

    // Create undo button
    undo_btn_ = lv_btn_create(screen);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!undo_btn_) std::cerr << "[UI-ERR] Failed to create undo_btn_!" << std::endl;
#endif
    lv_obj_set_size(undo_btn_, config.button_width, config.button_height);
    lv_obj_set_pos(undo_btn_, undo_x, undo_y);

    // Style the undo button
    lv_obj_set_style_bg_color(undo_btn_, lv_color_hex(SynthConstants::Color::BTN_UNDO_BG), 0);
    lv_obj_set_style_border_color(undo_btn_, lv_color_hex(SynthConstants::Color::BTN_UNDO_BORDER), 0);
    lv_obj_set_style_border_width(undo_btn_, SynthConstants::Layout::STATUS_BORDER_WIDTH, 0);
    lv_obj_set_style_radius(undo_btn_, SynthConstants::Layout::STATUS_RADIUS, 0);

    // Undo button label
    lv_obj_t* undo_label = lv_label_create(undo_btn_);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!undo_label) std::cerr << "[UI-ERR] Failed to create undo_label!" << std::endl;
#endif
    lv_label_set_text(undo_label, SynthConstants::Text::BTN_UNDO);
    lv_obj_set_style_text_color(undo_label, lv_color_hex(SynthConstants::Color::HELP), 0);
    lv_obj_set_style_text_font(undo_label, FontA.small, 0);
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
    lv_obj_set_style_bg_color(redo_btn_, lv_color_hex(SynthConstants::Color::BTN_REDO_BG), 0);
    lv_obj_set_style_border_color(redo_btn_, lv_color_hex(SynthConstants::Color::BTN_REDO_BORDER), 0);
    lv_obj_set_style_border_width(redo_btn_, SynthConstants::Layout::STATUS_BORDER_WIDTH, 0);
    lv_obj_set_style_radius(redo_btn_, SynthConstants::Layout::STATUS_RADIUS, 0);

    // Redo button label
    lv_obj_t* redo_label = lv_label_create(redo_btn_);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!redo_label) std::cerr << "[UI-ERR] Failed to create redo_label!" << std::endl;
#endif
    lv_label_set_text(redo_label, SynthConstants::Text::BTN_REDO);
    lv_obj_set_style_text_color(redo_label, lv_color_hex(SynthConstants::Color::HELP), 0);
    lv_obj_set_style_text_font(redo_label, FontA.small, 0);
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
        lv_obj_set_size(sim_button, LayoutManager::scaleSize(SynthConstants::Layout::SIM_BTN_WIDTH), config.button_height);
        lv_obj_align(sim_button, LV_ALIGN_BOTTOM_MID, 0, -config.status_height - config.button_height - SynthConstants::Layout::SIM_BTN_Y_OFFSET);

        // Style the button for 8-bit look
        lv_obj_set_style_bg_color(sim_button, lv_color_hex(SynthConstants::Color::SIM_BTN_BG), 0);
        lv_obj_set_style_border_color(sim_button, lv_color_hex(SynthConstants::Color::SIM_BTN_BORDER), 0);
        lv_obj_set_style_border_width(sim_button, 2, 0);
        lv_obj_set_style_radius(sim_button, SynthConstants::Layout::STATUS_RADIUS, 0);

        // Button label
        lv_obj_t* btn_label = lv_label_create(sim_button);
#if SYNTHAPP_DEBUG_UI_CHECKS
        if (!btn_label) std::cerr << "[UI-ERR] Failed to create sim_button label!" << std::endl;
#endif
        lv_label_set_text(btn_label, SynthConstants::Text::BTN_SIMULATE);
        lv_obj_set_style_text_color(btn_label, lv_color_hex(SynthConstants::Color::SIM_BTN_TEXT), 0);
        lv_obj_set_style_text_font(btn_label, FontA.small, 0);
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
    extern lv_obj_t* root_container;
    lv_obj_t* screen = root_container;
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
    lv_obj_set_style_bg_color(status_area, lv_color_hex(SynthConstants::Color::STATUS_BG), 0);
    lv_obj_set_style_border_color(status_area, lv_color_hex(SynthConstants::Color::STATUS_BORDER), 0);
    lv_obj_set_style_border_width(status_area, SynthConstants::Layout::STATUS_BORDER_WIDTH, 0);
    lv_obj_set_style_radius(status_area, SynthConstants::Layout::STATUS_RADIUS, 0);

    // Status label inside the area
    lv_obj_t* status_label = lv_label_create(status_area);
#if SYNTHAPP_DEBUG_UI_CHECKS
    if (!status_label) {
        std::cerr << "[UI-ERR] Failed to create status_label!" << std::endl;
        return;
    }
#endif
    if (LayoutManager::getScreenSize() == LayoutManager::ScreenSize::SMALL) {
        lv_label_set_text(status_label, SynthConstants::Text::STATUS_READY);
    } else {
        lv_label_set_text(status_label, SynthConstants::Text::STATUS_READY_LG);
    }
    lv_obj_set_style_text_color(status_label, lv_color_hex(SynthConstants::Color::STATUS), 0);
    lv_obj_set_style_text_font(status_label, FontA.small, 0);
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
                             can_undo ? lv_color_hex(SynthConstants::Color::BTN_UNDO_ACTIVE) : lv_color_hex(SynthConstants::Color::BTN_UNDO_BG), 
                             LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(undo_label_, 
                               can_undo ? lv_color_hex(SynthConstants::Color::BTN_UNDO_TEXT) : lv_color_hex(SynthConstants::Color::BTN_UNDO_TEXT_DISABLED), 
                               LV_STATE_DEFAULT);

    // Update redo button state
    bool can_redo = command_manager_->canRedo();
    lv_obj_set_style_bg_color(redo_btn_, 
                             can_redo ? lv_color_hex(SynthConstants::Color::BTN_REDO_ACTIVE) : lv_color_hex(SynthConstants::Color::BTN_REDO_BG), 
                             LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(redo_label_, 
                               can_redo ? lv_color_hex(SynthConstants::Color::BTN_REDO_TEXT) : lv_color_hex(SynthConstants::Color::BTN_REDO_TEXT_DISABLED), 
                               LV_STATE_DEFAULT);

    // Update status with undo/redo info
    if (status_label_) {
        char status_text[SynthConstants::Buffer::STATUS_UNDO_REDO];
        snprintf(status_text, sizeof(status_text), 
                SynthConstants::Text::STATUS_UNDO_REDO_FORMAT, 
                command_manager_->getUndoDescription().c_str(),
                command_manager_->getRedoDescription().c_str());
        lv_label_set_text(status_label_, status_text);
    }
}

void SynthApp::loop() {
    if (!initialized_) return;
    
    lv_timer_handler();
    midi_handler_->update();  // Process MIDI with Control_Surface
    
    // Update window manager
    if (window_manager_) {
        window_manager_->update();
    }
    
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
    midi_handler_->sendControlChange(SynthConstants::Midi::CHANNEL, cc_number, value);
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
    int value = SynthConstants::Midi::SIM_VALUES[sim_counter % 5];

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

    updateStatus(SynthConstants::Text::STATUS_SIM, value);
    sim_counter++;
}

void SynthApp::updateStatus(const char* control, int value) {
    if (status_label_) {
        char status_text[SynthConstants::Buffer::STATUS];
        snprintf(status_text, sizeof(status_text), 
                SynthConstants::Text::STATUS_FORMAT, control, value, 
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

// ============================================================================
// NEW: Window Management Implementation
// ============================================================================

void SynthApp::initWindowManager() {
    std::cout << "Initializing WindowManager..." << std::endl;
    
    // Create a constrained root container for ESP32 parity
    if (!root_container_) {
        root_container_ = lv_obj_create(lv_scr_act());
        
        // Set size to match ESP32 display dimensions for consistency
        // Common ESP32 display sizes: 320x240, 480x320, etc.
        // Let's use a reasonable size that works on both platforms
        int target_width = 480;
        int target_height = 320;
        
        // Get actual screen size
        int screen_width = lv_display_get_horizontal_resolution(lv_display_get_default());
        int screen_height = lv_display_get_vertical_resolution(lv_display_get_default());
        
        // Use smaller of target size or actual screen size
        int container_width = std::min(target_width, screen_width);
        int container_height = std::min(target_height, screen_height);
        
        lv_obj_set_size(root_container_, container_width, container_height);
        lv_obj_center(root_container_);  // Center on screen
        
        // Style the root container
        lv_obj_set_style_bg_color(root_container_, lv_color_hex(SynthConstants::Color::BG), 0);
        lv_obj_set_style_border_color(root_container_, lv_color_hex(0xFF333333), 0);
        lv_obj_set_style_border_width(root_container_, 2, 0);
        lv_obj_set_style_radius(root_container_, 8, 0);
        lv_obj_set_style_pad_all(root_container_, 0, 0);
        
        std::cout << "Created root container: " << container_width << "x" << container_height << std::endl;
    }
    
    // Create window manager with the constrained container
    window_manager_ = std::make_unique<WindowManager>(root_container_);
    window_manager_->addObserver(this);  // Register as observer
    
    createTabs();
    
    std::cout << "WindowManager initialized with 3 tabs" << std::endl;
}

void SynthApp::createTabs() {
    // Create Main Control Tab (with your existing controls)
    main_tab_ = std::make_unique<MainControlTab>(parameter_binder_.get(), command_manager_.get(), midi_handler_.get());
    window_manager_->addTab(std::move(main_tab_));
    
    // Create Hello Tab
    hello_tab_ = std::make_unique<HelloTab>();
    window_manager_->addTab(std::move(hello_tab_));
    
    // Create World Tab  
    world_tab_ = std::make_unique<WorldTab>();
    window_manager_->addTab(std::move(world_tab_));
    
    // Main tab will be active by default (first tab added)
    std::cout << "Created 3 tabs: Main, Hello, World" << std::endl;
}

// WindowObserver interface implementation
void SynthApp::onTabChanged(const std::string& old_tab, const std::string& new_tab) {
    std::cout << "Tab changed from '" << old_tab << "' to '" << new_tab << "'" << std::endl;
}

void SynthApp::onPopupOpened(const std::string& popup_name) {
    std::cout << "Popup opened: " << popup_name << std::endl;
}

void SynthApp::onPopupClosed(const std::string& popup_name) {
    std::cout << "Popup closed: " << popup_name << std::endl;
}