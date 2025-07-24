#pragma once

#include "components/threading/ThreadingAbstraction.h"
#include "components/midi/SimpleMidiClockProcessor.h"
#include "components/ui/WindowManager.h"
#include "components/ui/SimpleClockTab.h"
#include "Constants.h"
#include <memory>
#include <atomic>
#include <iostream>

/**
 * @brief Threaded application manager - replaces single-threaded SynthApp gradually
 */
class ThreadedSynthApp {
private:
    std::atomic<bool> initialized_{false};
    
    // Threading
    std::unique_ptr<Threading::ThreadHandle> ui_thread_;
    
    // Core systems
    std::unique_ptr<MIDI::SimpleMidiClockProcessor> midi_clock_;
    std::unique_ptr<WindowManager> window_manager_;
    
    // LVGL objects (for platform compatibility)
    lv_obj_t* main_screen_ = nullptr;
    
public:
    ThreadedSynthApp() = default;
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
        
        // Create MIDI clock processor
        midi_clock_ = std::make_unique<MIDI::SimpleMidiClockProcessor>();
        if (!midi_clock_->start()) {
            std::cout << "Failed to start MIDI clock processor!" << std::endl;
            return false;
        }
        
        // Initialize UI system
        if (!initializeUI()) {
            std::cout << "Failed to initialize UI!" << std::endl;
            return false;
        }
        
        // Start UI thread
        if (!startUIThread()) {
            std::cout << "Failed to start UI thread!" << std::endl;
            return false;
        }
        
        initialized_.store(true);
        std::cout << "=== Threaded Synth App Initialized ===" << std::endl;
        return true;
    }
    
    /**
     * @brief Main loop - much simpler now!
     */
    void loop() {
        if (!initialized_.load()) return;
        
        // In threaded version, main loop just sleeps
        // All real work happens in dedicated threads
        Threading::TaskManager::sleep(1000);
        
        // Optional: Print status
        static int loop_count = 0;
        if (++loop_count % 10 == 0) {
            std::cout << "[Main] App running - Clock: " 
                      << (midi_clock_->isClockRunning() ? "ON" : "OFF")
                      << ", BPM: " << midi_clock_->getBPM() << std::endl;
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
        if (midi_clock_) {
            midi_clock_->stop();
            midi_clock_.reset();
        }
        
        // Cleanup UI
        window_manager_.reset();
    }
    
    /**
     * @brief Access to MIDI clock for testing
     */
    MIDI::SimpleMidiClockProcessor* getMidiClock() const {
        return midi_clock_.get();
    }
    
private:
    bool initializeLVGL() {
        #ifdef ESP32_BUILD
            // ESP32 LVGL initialization
            lv_init();
            // Display and input device setup would go here
            std::cout << "[ESP32] LVGL initialized" << std::endl;
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
        // Create window manager
        window_manager_ = std::make_unique<WindowManager>(main_screen_);
        
        // For now, only create the clock tab
        createMinimalTabs();
        
        return true;
    }
    
    void createMinimalTabs() {
        // Start with just a simple clock display tab
        // This reduces complexity while we test the threading
        std::cout << "Creating minimal tab set (Clock only)" << std::endl;
        
        // Create simple clock tab
        auto clock_tab = std::make_unique<SimpleClockTab>(midi_clock_.get());
        window_manager_->addTab(std::move(clock_tab));
        
        std::cout << "Clock tab created and added to window manager" << std::endl;
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
            
            // Process MIDI events for UI updates
            if (midi_clock_) {
                midi_clock_->processEvents();
            }
            
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
