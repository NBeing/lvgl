#pragma once

#include "components/threading/ThreadingAbstraction.h"
#include "components/midi/MidiClockManager.h"
#include "components/midi/MidiEvents.h"
#include "components/ui/WindowManager.h"
#include "components/ui/SimpleClockTab.h"
// Add all the SynthApp components
#include "components/parameter/ParameterBinder.h"
#include "components/parameter/CommandManager.h"
#include "components/ui/MainControlTab.h"
#include "components/ui/HelloTab.h"
#include "components/ui/WorldTab.h"
#include "components/ui/SettingsTab.h"
#include "components/ui/ClockTab.h"
#include "components/ui/MidiMonitorTab.h"
#include "components/midi/UnifiedMidiManager.h"
#include "hardware/MidiHandler.h"
#include "Constants.h"
#include <memory>
#include <atomic>
#include <iostream>

#if defined(ESP32_BUILD)
#include "hardware/ESP32Display.h"
// External callback from main.cpp
extern uint32_t millis_cb();
#endif

/**
 * @brief Threaded application manager - replaces single-threaded SynthApp gradually
 */
class ThreadedSynthApp {
private:
    std::atomic<bool> initialized_{false};
    
    // Threading
    std::unique_ptr<Threading::ThreadHandle> ui_thread_;
    
    // Core systems (same as SynthApp but with threading)
    // Use MidiClockManager instead of SimpleMidiClockProcessor (connects to UI)
    std::unique_ptr<WindowManager> window_manager_;
    
    // Parameter system (from SynthApp)
    std::unique_ptr<ParameterBinder> parameter_binder_;
    std::unique_ptr<CommandManager> command_manager_;
    
    // MIDI system (from SynthApp)
    std::shared_ptr<MidiHandler> midi_handler_;
    
    // UI Tabs (from SynthApp)
    std::unique_ptr<MainControlTab> main_tab_;
    std::unique_ptr<HelloTab> hello_tab_;
    std::unique_ptr<WorldTab> world_tab_;
    std::unique_ptr<SettingsTab> settings_tab_;
    std::unique_ptr<ClockTab> clock_tab_;
    std::unique_ptr<MidiMonitorTab> midi_monitor_tab_;
    MidiMonitorTab* midi_monitor_tab_ptr_ = nullptr;  // Raw pointer for updates
    
    // LVGL objects (for platform compatibility)
    lv_obj_t* main_screen_ = nullptr;
    
#if defined(ESP32_BUILD)
    // ESP32-specific display driver
    std::unique_ptr<ESP32Display> display_driver_;
#else
    // Desktop-specific LVGL objects
    lv_display_t* display_ = nullptr;
    lv_indev_t* mouse_ = nullptr;
    lv_indev_t* keyboard_ = nullptr;
    lv_indev_t* mousewheel_ = nullptr;
#endif
    
public:
    ThreadedSynthApp() {
#if defined(ESP32_BUILD)
        display_driver_.reset(new ESP32Display());
#endif
    }
    ~ThreadedSynthApp() { shutdown(); }
    
    /**
     * @brief Initialize the threaded application
     */
    bool initialize() {
        if (initialized_.load()) return true;
        
        std::cout << "=== Threaded Synth App Starting ===" << std::endl;
        
        // Initialize LVGL first (platform-specific)
        if (!initializeLVGL()) {
            std::cout << "Failed to initialize LVGL!" << std::endl;
            return false;
        }
        
        // Initialize UI system first (includes MIDI setup)
        if (!initializeUI()) {
            std::cout << "Failed to initialize UI!" << std::endl;
            return false;
        }
        
        // MidiClockManager is a singleton, so we just initialize it
        std::cout << "[ThreadedSynthApp] MIDI clock will use MidiClockManager (same as ClockTab)" << std::endl;
        
        // Skip UI thread for now - use main thread like SynthApp
        // The threading advantage comes from MIDI background processing
        
        initialized_.store(true);
        std::cout << "=== Threaded Synth App Initialized ===" << std::endl;
        return true;
    }
    
    /**
     * @brief Main loop - similar to SynthApp but with background MIDI processing
     */
    void loop() {
        if (!initialized_.load()) return;
        
        // Handle LVGL updates (same as SynthApp)
        lv_timer_handler();
        
        // Update MidiClockManager (handles threaded clock generation)
        MidiClockManager::getInstance().update();
        
        // Update UnifiedMidiManager (processes MIDI input from all backends)
        UnifiedMidiManager::getInstance().update();
        
        // Update window manager
        if (window_manager_) {
            window_manager_->update();
        }
        
        // Update MIDI monitor safely in main loop context (like SynthApp)
        if (midi_monitor_tab_ptr_) {
            midi_monitor_tab_ptr_->getMonitor().update();
        }
        
        // Sleep for smooth 60Hz updates (like SynthApp)
        Threading::TaskManager::sleep(16);  // ~60Hz updates
        
        // Optional: Print status less frequently
        static int loop_count = 0;
        if (++loop_count % 3000 == 0) {  // Every ~50 seconds
            auto& clock_mgr = MidiClockManager::getInstance();
            std::cout << "[ThreadedSynthApp] Running - Clock: " 
                      << (clock_mgr.isRunning() ? "ON" : "OFF")
                      << ", BPM: " << clock_mgr.getBPM() << std::endl;
        }
    }
    
    /**
     * @brief Clean shutdown
     */
    void shutdown() {
        if (!initialized_.load()) return;
        
        std::cout << "=== Shutting down Threaded Synth App ===" << std::endl;
        
        initialized_.store(false);
        
        // Stop UI thread
        if (ui_thread_) {
            ui_thread_->stop();
            ui_thread_.reset();
        }
        
        // Stop MIDI processor
        MidiClockManager::getInstance().stop();
        
        // Cleanup UI
        window_manager_.reset();
    }
    
    /**
     * @brief Access to MIDI clock for testing
     */
    MidiClockManager& getMidiClock() const {
        return MidiClockManager::getInstance();
    }
    
private:
    bool initializeLVGL() {
        #ifdef ESP32_BUILD
            // ESP32 LVGL initialization (same as SynthApp)
            lv_init();
            lv_tick_set_cb(millis_cb);
            std::cout << "[ESP32] LVGL initialized. Registering input devices..." << std::endl;
            
            // Initialize display driver
            if (display_driver_) {
                display_driver_->initialize();
                std::cout << "[ESP32] Display driver initialized" << std::endl;
            }
            
        #else
            // Desktop LVGL initialization
            lv_init();
            
            // Create SDL display with exact ESP32 dimensions
            auto display = lv_sdl_window_create(
                SynthConstants::ESP32_SCREEN_WIDTH, 
                SynthConstants::ESP32_SCREEN_HEIGHT
            );
            lv_sdl_window_set_title(display, "Threaded MIDI Synth");
            
            // Input setup
            auto mouse = lv_sdl_mouse_create();
            auto mousewheel = lv_sdl_mousewheel_create();
            auto keyboard = lv_sdl_keyboard_create();
            
            std::cout << "[Desktop] LVGL and SDL initialized (" 
                      << SynthConstants::ESP32_SCREEN_WIDTH << "x" 
                      << SynthConstants::ESP32_SCREEN_HEIGHT << ")" << std::endl;
        #endif
        
        // Create main screen AFTER display is set up
        main_screen_ = lv_obj_create(nullptr);
        lv_obj_set_size(main_screen_, 
                        SynthConstants::ESP32_SCREEN_WIDTH, 
                        SynthConstants::ESP32_SCREEN_HEIGHT);
        lv_obj_set_style_bg_color(main_screen_, 
                                  lv_color_hex(SynthConstants::Color::BG), 0);
        lv_scr_load(main_screen_);
        
        std::cout << "[LVGL] Main screen created: " 
                  << SynthConstants::ESP32_SCREEN_WIDTH << "x" 
                  << SynthConstants::ESP32_SCREEN_HEIGHT << std::endl;
        
        return true;
    }
    
    bool initializeUI() {
        // Initialize parameter system (from SynthApp)
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
        
        // Initialize MIDI handler (from SynthApp)
        midi_handler_ = std::shared_ptr<MidiHandler>(new MidiHandler());
        if (midi_handler_->initialize()) {
            std::cout << "[ThreadedSynthApp] MidiHandler initialized successfully" << std::endl;
        } else {
            std::cout << "[ThreadedSynthApp] MidiHandler initialization failed" << std::endl;
        }
        
        // Set up UnifiedMidiManager
        UnifiedMidiManager::setSharedMidiHandler(midi_handler_);
        UnifiedMidiManager::getInstance().initialize();
        
        // Create window manager with constrained container (like SynthApp)
        createWindowManager();
        
        // Create all tabs like SynthApp
        createAllTabs();
        
        return true;
    }
    
    void createWindowManager() {
        // Create a constrained root container for ESP32 parity (same as SynthApp)
        lv_obj_t* app_container = lv_obj_create(lv_scr_act());
        
        // Set size to match ESP32 display dimensions for consistency
        lv_obj_set_size(app_container, SynthConstants::ESP32_SCREEN_WIDTH, SynthConstants::ESP32_SCREEN_HEIGHT);
        lv_obj_center(app_container);  // Center on screen
        
        // Style the app container (same as SynthApp)
        lv_obj_set_style_bg_color(app_container, lv_color_hex(SynthConstants::Color::BG), 0);
        lv_obj_set_style_border_color(app_container, lv_color_hex(0xFF333333), 0);
        lv_obj_set_style_border_width(app_container, 2, 0);
        lv_obj_set_style_radius(app_container, 8, 0);
        lv_obj_set_style_pad_all(app_container, 0, 0);
        
        std::cout << "[ThreadedSynthApp] Created constrained app container: " 
                  << SynthConstants::ESP32_SCREEN_WIDTH << "x" << SynthConstants::ESP32_SCREEN_HEIGHT << std::endl;
        
        // Create window manager with the constrained container (not the full screen)
        window_manager_ = std::make_unique<WindowManager>(app_container);
    }
    
    void createAllTabs() {
        // Create all 6 tabs like SynthApp for full functionality
        std::cout << "Creating all tabs like SynthApp..." << std::endl;
        
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
        
        // Create Clock Tab (full version, not SimpleClockTab)
        clock_tab_ = std::make_unique<ClockTab>();
        window_manager_->addTab(std::move(clock_tab_));
        
        // Create MIDI Monitor Tab
        auto midi_monitor_unique = std::make_unique<MidiMonitorTab>();
        midi_monitor_tab_ptr_ = midi_monitor_unique.get();  // Get raw pointer before move
        midi_monitor_tab_ = std::move(midi_monitor_unique);
        window_manager_->addTab(std::move(midi_monitor_tab_));
        
        // Connect the MIDI monitor to the UnifiedMidiManager for logging
        UnifiedMidiManager::getInstance().setMidiMonitor(&midi_monitor_tab_ptr_->getMonitor());
        
        std::cout << "Created 6 tabs: Main, Hello, World, Settings, Clock, MIDI Monitor" << std::endl;
    }
    
    bool startUIThread() {
        ui_thread_ = Threading::TaskManager::createTask(
            "UI_Thread",
            [this]() { uiThreadLoop(); },
            Threading::TaskManager::Priority::NORMAL,
            Threading::TaskManager::Core::CORE_1,
            8192 // Larger stack for LVGL
        );
        
        return ui_thread_ != nullptr;
    }
    
    void uiThreadLoop() {
        std::cout << "[UI Thread] Started" << std::endl;
        
        while (initialized_.load()) {
            // Process LVGL
            lv_timer_handler();
            
            // Update MidiClockManager
            MidiClockManager::getInstance().update();
            
            // Update UnifiedMidiManager (processes MIDI input from all backends)
            UnifiedMidiManager::getInstance().update();
            
            // Update window manager
            if (window_manager_) {
                window_manager_->update();
            }
            
            // 60Hz UI updates (16.67ms)
            Threading::TaskManager::sleep(16);
        }
        
        std::cout << "[UI Thread] Stopped" << std::endl;
    }
};
