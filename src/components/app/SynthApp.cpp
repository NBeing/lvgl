#include "SynthApp.h"
#include "components/layout/LayoutManager.h"
#include "components/parameter/ParameterBinder.h"
#include "components/parameter/CommandManager.h"
#include "components/ui/MainControlTab.h"
#include "components/ui/HelloTab.h"
#include "components/ui/WorldTab.h"
#include "components/ui/SettingsTab.h"
#include "components/ui/ClockTab.h"
#include "components/midi/MidiClockManager.h"
#include "components/midi/UnifiedMidiManager.h"
#include "FontConfig.h"
#include "Constants.h"
#include <iostream>
#if !defined(ESP32_BUILD)
#include <unistd.h>  // For usleep on desktop
#endif

#if defined(ESP32_BUILD)
#include "hardware/LGFX_ST7796S.h"
extern LGFX_ST7796S tft;

// External callback from main.cpp
extern uint32_t millis_cb();
#endif

SynthApp::SynthApp() 
    : initialized_(false)
    , window_manager_(nullptr)
{
    #if defined(ESP32_BUILD)
        display_driver_.reset(new ESP32Display());
    #else
        // Initialize desktop pointers to nullptr
        display_ = nullptr;
        mouse_ = nullptr;
        keyboard_ = nullptr;
        mousewheel_ = nullptr;
    #endif
    
    midi_handler_ = std::shared_ptr<MidiHandler>(new MidiHandler());
    std::cout << "[SynthApp] Constructor: Created MidiHandler at " << midi_handler_.get() << std::endl;
    
    // Initialize the MIDI handler immediately
    if (midi_handler_->initialize()) {
        std::cout << "[SynthApp] Constructor: MidiHandler initialized successfully" << std::endl;
    } else {
        std::cout << "[SynthApp] Constructor: MidiHandler initialization failed" << std::endl;
    }
    std::cout << "[SynthApp] Constructor: MidiHandler status: " << midi_handler_->getConnectionStatus() << std::endl;
    
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
    
    // Set up history change callback to update UI (handled by MainControlTab now)
    command_manager_->setHistoryChangedCallback([this]() {
        // MainControlTab handles UI updates
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
        
        // Initialize display driver
        if (display_driver_) {
            display_driver_->initialize();
            std::cout << "[ESP32] Display driver initialized" << std::endl;
        }
        
        // Initialize unified MIDI system
        std::cout << "[SynthApp] Before sharing - MidiHandler status: " << midi_handler_->getConnectionStatus() << std::endl;
        std::cout << "[SynthApp] Before sharing - MidiHandler isConnected: " << midi_handler_->isConnected() << std::endl;
        UnifiedMidiManager::setSharedMidiHandler(midi_handler_);
        std::cout << "[SynthApp] After sharing - MidiHandler status: " << midi_handler_->getConnectionStatus() << std::endl;
        std::cout << "[SynthApp] After sharing - MidiHandler isConnected: " << midi_handler_->isConnected() << std::endl;
        UnifiedMidiManager::getInstance().initialize();
        
        // Configure for ESP32 environment
        LayoutManager::initialize();
        
    #else
        // Desktop initialization
        initDesktop();
        
        // Initialize unified MIDI system for desktop with shared MidiHandler
        std::cout << "[SynthApp] Desktop - About to share MidiHandler at " << midi_handler_.get() << std::endl;
        std::cout << "[SynthApp] Desktop - Before sharing - MidiHandler status: " << midi_handler_->getConnectionStatus() << std::endl;
        std::cout << "[SynthApp] Desktop - Before sharing - MidiHandler isConnected: " << midi_handler_->isConnected() << std::endl;
        UnifiedMidiManager::setSharedMidiHandler(midi_handler_);
        std::cout << "[SynthApp] Desktop - After sharing - MidiHandler status: " << midi_handler_->getConnectionStatus() << std::endl;
        std::cout << "[SynthApp] Desktop - After sharing - MidiHandler isConnected: " << midi_handler_->isConnected() << std::endl;
        UnifiedMidiManager::getInstance().initialize();
    #endif
    
    std::cout << "Hardware initialization complete" << std::endl;
}

#if !defined(ESP32_BUILD)
void SynthApp::initDesktop() {
    std::cout << "[Desktop] Initializing LVGL for desktop..." << std::endl;
    
    // LVGL initialization
    lv_init();
    
    // Display setup using ESP32 screen dimensions to match hardware
    display_ = lv_sdl_window_create(SynthConstants::ESP32_SCREEN_WIDTH, SynthConstants::ESP32_SCREEN_HEIGHT);
    lv_sdl_window_set_title(display_, SynthConstants::Text::TITLE);
    
    std::cout << "[Desktop] SDL window created: " << SynthConstants::ESP32_SCREEN_WIDTH 
              << "x" << SynthConstants::ESP32_SCREEN_HEIGHT << std::endl;
    
    // Input setup
    mouse_ = lv_sdl_mouse_create();
    keyboard_ = lv_sdl_keyboard_create();
    mousewheel_ = lv_sdl_mousewheel_create();
    
    // Initialize layout manager for desktop environment
    LayoutManager::initialize();
    
    std::cout << "[Desktop] LVGL initialized successfully (480x320 to match ESP32)" << std::endl;
}
#endif // !defined(ESP32_BUILD)

void SynthApp::initWindowManager() {
    std::cout << "Initializing WindowManager..." << std::endl;
    
    // Create a constrained root container for ESP32 parity
    lv_obj_t* app_container = lv_obj_create(lv_screen_active());
    
    // Set size to match ESP32 display dimensions for consistency
    lv_obj_set_size(app_container, SynthConstants::ESP32_SCREEN_WIDTH, SynthConstants::ESP32_SCREEN_HEIGHT);
    lv_obj_center(app_container);  // Center on screen
    
    // Style the app container
    lv_obj_set_style_bg_color(app_container, lv_color_hex(SynthConstants::Color::BG), 0);
    lv_obj_set_style_border_color(app_container, lv_color_hex(0xFF333333), 0);
    lv_obj_set_style_border_width(app_container, 2, 0);
    lv_obj_set_style_radius(app_container, 8, 0);
    lv_obj_set_style_pad_all(app_container, 0, 0);
    
    std::cout << "[Desktop] Created constrained app container: " 
              << SynthConstants::ESP32_SCREEN_WIDTH << "x" << SynthConstants::ESP32_SCREEN_HEIGHT << std::endl;
    
    // Create window manager with the constrained container (not the full screen)
    window_manager_ = std::make_unique<WindowManager>(app_container);
    window_manager_->addObserver(this);  // Register as observer
    
    createTabs();
    
    std::cout << "WindowManager initialized with tabs" << std::endl;
}

void SynthApp::createTabs() {
    // Create Main Control Tab (with undo/redo and parameter controls)
    main_tab_ = std::make_unique<MainControlTab>(
        parameter_binder_.get(), 
        command_manager_.get(), 
        midi_handler_.get()
    );
    window_manager_->addTab(std::move(main_tab_));
    
    // Create Hello Tab
    hello_tab_ = std::make_unique<HelloTab>();
    window_manager_->addTab(std::move(hello_tab_));
    
    // Create World Tab  
    world_tab_ = std::make_unique<WorldTab>();
    window_manager_->addTab(std::move(world_tab_));
    
    // Create Settings Tab
    settings_tab_ = std::make_unique<SettingsTab>();
    window_manager_->addTab(std::move(settings_tab_));
    
    // Create Clock Tab
    clock_tab_ = std::make_unique<ClockTab>();
    window_manager_->addTab(std::move(clock_tab_));
    
    // Main tab will be active by default (first tab added)
    std::cout << "Created 5 tabs: Main, Hello, World, Settings, Clock" << std::endl;
}

void SynthApp::loop() {
    if (!initialized_) return;
    
    #if defined(ESP32_BUILD)
        lv_timer_handler();
        delay(5);
    #else
        lv_timer_handler();
        usleep(5000);  // 5ms delay
    #endif
    
    // Update window manager
    if (window_manager_) {
        window_manager_->update();
    }
    
    // Update MIDI clock manager
    MidiClockManager::getInstance().update();
    
    // Update unified MIDI manager
    UnifiedMidiManager::getInstance().update();
}

// WindowObserver implementation
void SynthApp::onTabChanged(const std::string& /* old_tab */, const std::string& new_tab) {
    std::cout << "Tab changed to: " << new_tab << std::endl;
}

// Utility methods that might be useful for debugging
void SynthApp::simulateMidiCC() {
    if (!midi_handler_ || !midi_handler_->isConnected()) {
        std::cout << "MIDI not connected - simulation skipped" << std::endl;
        return;
    }
    
    // Simulate some MIDI CC changes for testing
    static uint8_t test_value = 64;
    test_value = (test_value + 10) % 128;
    
    midi_handler_->sendControlChange(SynthConstants::Midi::CHANNEL, 74, test_value); // Filter cutoff
    std::cout << "Simulated MIDI CC74 = " << (int)test_value << std::endl;
}

void SynthApp::testHardwareMidi() {
    auto& unified_midi = UnifiedMidiManager::getInstance();
    
    if (!unified_midi.isConnected()) {
        std::cout << "Unified MIDI not connected" << std::endl;
        return;
    }
    
    std::cout << "=== Unified MIDI Test ===" << std::endl;
    
    // Show backend status
    auto backends = unified_midi.getAvailableBackends();
    for (const auto& backend : backends) {
        std::cout << "Backend: " << backend.name << " - Status: " 
                  << (backend.status == UnifiedMidiManager::ConnectionStatus::CONNECTED ? "Connected" : "Disconnected")
                  << " - Sent: " << backend.messages_sent << std::endl;
    }
    
    // Test note sequence
    unified_midi.sendNoteOn(1, 60, 100);   // Middle C, velocity 100
    unified_midi.sendNoteOff(1, 60, 0);    // Middle C off
    
    // Test control change
    unified_midi.sendControlChange(1, 7, 127);  // Volume full
    unified_midi.sendControlChange(1, 74, 64);  // Filter cutoff middle
    
    // Test program change
    unified_midi.sendProgramChange(1, 10);  // Change to program 10
    
    std::cout << "Unified MIDI test complete - Total messages sent: " << unified_midi.getTotalMessagesSent() << std::endl;
}

bool SynthApp::isInitialized() const {
    return initialized_;
}

std::shared_ptr<MidiHandler> SynthApp::getMidiHandler() const {
    return midi_handler_;
}
