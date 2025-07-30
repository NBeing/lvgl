#pragma once

#include "components/threading/ThreadingAbstraction.h"
#include "components/midi/MidiClockManager.h"
#include "components/midi/ThreadedMidiClockManager.h"
#include "components/midi/MetronomeObserver.h"
#include "components/midi/StepSequencer.h"
#include "components/midi/SequencerMidiOutput.h"
#include "components/midi/TimestampedMidiOutput.h"
#include "components/midi/MidiTimingThread.h"
#include "components/midi/MidiEvents.h"
#include "components/ui/WindowManager.h"
#include "components/ui/SimpleClockTab.h"
#include "components/controls/SafeMidiControlIntegration.h"
#include "components/controls/SafeControlClockObserver.h"
#include "components/controls/ControlClockObserver.h"
#include "components/memory/SimpleMemoryMonitor.h"

#ifdef MEMORY_STRESS_TEST
#include "components/memory/MemoryStressTest.h"
#endif
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
#include "components/parameter/ParameterManager.h"
#include "components/parameter/MidiParameterBridge.h"
#include "components/parameter/ParameterRegistry.h"
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
    
    // Threaded MIDI observers (demonstration)
    std::unique_ptr<MIDI::MetronomeObserver> metronome_observer_;
    std::unique_ptr<MIDI::StepSequencer> step_sequencer_;
    std::unique_ptr<MIDI::SequencerMidiOutput> sequencer_midi_output_;
    std::shared_ptr<MIDI::TimestampedMidiOutput> timestamped_midi_output_;
    std::unique_ptr<SafeControlClockObserver> safe_control_clock_observer_;
    std::unique_ptr<ControlClockObserver> control_clock_observer_; // Keep old one for compatibility
    
    // High-precision timing components
    std::shared_ptr<MIDI::MidiTimingThread> midi_timing_thread_;
    
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
        
        // Start the threaded MIDI clock manager
        std::cout << "[ThreadedSynthApp] Starting threaded MIDI clock manager..." << std::endl;
        if (!MIDI::ThreadedMidiClockManager::getInstance().start()) {
            std::cout << "Failed to start threaded MIDI clock manager!" << std::endl;
            return false;
        }
        
        // Initialize enhanced MIDI control integration (SAFE VERSION)
        std::cout << "[ThreadedSynthApp] Initializing safe MIDI control integration..." << std::endl;
        SafeMidiControlIntegration::getInstance().initialize();
        
        // Initialize memory monitoring systems
        std::cout << "[ThreadedSynthApp] 🛡️ Initializing memory monitoring..." << std::endl;
        setupMemoryMonitoring();
        
        // Create and register safe control clock observer for tempo-synced controls
        safe_control_clock_observer_ = std::make_unique<SafeControlClockObserver>();
        MIDI::ThreadedMidiClockManager::getInstance().addClockObserver(safe_control_clock_observer_.get());
        std::cout << "[ThreadedSynthApp] Safe control clock observer registered for tempo-synced controls" << std::endl;
        
        // Create and register metronome observer (demonstration)
        metronome_observer_ = std::make_unique<MIDI::MetronomeObserver>();
        MIDI::ThreadedMidiClockManager::getInstance().addClockObserver(metronome_observer_.get());
        std::cout << "[ThreadedSynthApp] Metronome observer registered for clock events" << std::endl;
        
        // Create and register step sequencer
        step_sequencer_ = std::make_unique<MIDI::StepSequencer>();
        MIDI::ThreadedMidiClockManager::getInstance().addClockObserver(step_sequencer_.get());
        std::cout << "[ThreadedSynthApp] Step sequencer registered for clock events" << std::endl;
        
        // Create high-precision timing components
        midi_timing_thread_ = std::make_shared<MIDI::MidiTimingThread>();
        
        // Create timestamped MIDI output for precise timing
        timestamped_midi_output_ = std::make_shared<MIDI::TimestampedMidiOutput>(midi_handler_);
        
        // Connect timestamped output to timing thread
        midi_timing_thread_->setTimestampedOutput(timestamped_midi_output_);
        
        // Start the high-precision timing thread
        if (midi_timing_thread_->start()) {
            std::cout << "[ThreadedSynthApp] ⚡ High-precision MIDI timing thread started (10kHz)" << std::endl;
        } else {
            std::cerr << "[ThreadedSynthApp] ❌ Failed to start MIDI timing thread!" << std::endl;
        }
        
        // Connect sequencer to timestamped output
        step_sequencer_->addSequencerObserver(timestamped_midi_output_.get());
        std::cout << "[ThreadedSynthApp] Sequencer connected to high-precision timing system" << std::endl;
        
        // Program a simple demo pattern
        setupDemoPattern();
        
        // The regular MidiClockManager is still used for non-RT features
        std::cout << "[ThreadedSynthApp] Using ThreadedMidiClockManager for RT clock generation" << std::endl;
        
        // Skip UI thread for now - use main thread like SynthApp
        // The threading advantage comes from MIDI background processing
        
        initialized_.store(true);
        
        std::cout << "=== Threaded Synth App Initialized ===" << std::endl;
        
        // Final memory checkpoint
        MEMORY_CHECKPOINT("ThreadedSynthApp fully initialized");
        
        return true;
    }
    
    /**
     * @brief Setup a demo sequencer pattern
     */
    void setupDemoPattern() {
        if (!step_sequencer_) return;
        
        std::cout << "[ThreadedSynthApp] Setting up demo sequencer pattern..." << std::endl;
        
        // Track 0: Kick drum pattern (MIDI note 36 = C2)
        step_sequencer_->setStep(0, 0, MIDI::StepSequencer::Step(36, 120, true));  // Downbeat accent
        step_sequencer_->setStep(0, 4, MIDI::StepSequencer::Step(36, 100));        // Beat 2
        step_sequencer_->setStep(0, 8, MIDI::StepSequencer::Step(36, 110));        // Beat 3
        step_sequencer_->setStep(0, 12, MIDI::StepSequencer::Step(36, 100));       // Beat 4
        
        // Track 1: Snare drum pattern (MIDI note 38 = D2)
        step_sequencer_->setStep(1, 4, MIDI::StepSequencer::Step(38, 115));        // Beat 2
        step_sequencer_->setStep(1, 12, MIDI::StepSequencer::Step(38, 115));       // Beat 4
        
        // Track 2: Hi-hat pattern (MIDI note 42 = F#2)
        for (int i = 0; i < 16; i += 2) {
            step_sequencer_->setStep(2, i, MIDI::StepSequencer::Step(42, 80));     // 8th notes
        }
        
        // Track 3: Bass line (MIDI notes C3, E3, G3)
        step_sequencer_->setStep(3, 0, MIDI::StepSequencer::Step(48, 100));        // C3
        step_sequencer_->setStep(3, 6, MIDI::StepSequencer::Step(52, 90));         // E3
        step_sequencer_->setStep(3, 8, MIDI::StepSequencer::Step(48, 100));        // C3
        step_sequencer_->setStep(3, 14, MIDI::StepSequencer::Step(55, 90));        // G3
        
        // Set up track channels and properties
        step_sequencer_->getTrack(0).channel = 10; // Drum channel
        step_sequencer_->getTrack(1).channel = 10; // Drum channel
        step_sequencer_->getTrack(2).channel = 10; // Drum channel
        step_sequencer_->getTrack(3).channel = 1;  // Bass channel
        
        std::cout << "[ThreadedSynthApp] Demo pattern loaded - 4 tracks ready!" << std::endl;
    }
    
    /**
     * @brief Print detailed performance statistics
     */
    void printDetailedPerformanceStats(std::chrono::microseconds midi_duration,
                                      std::chrono::microseconds ui_duration,
                                      std::chrono::microseconds total_duration) {
        std::cout << "\n=== ThreadedSynthApp Detailed Performance Report ===" << std::endl;
        std::cout << "Loop Timing:" << std::endl;
        std::cout << "  MIDI Scheduling: " << midi_duration.count() << "μs" << std::endl;
        std::cout << "  UI Processing: " << ui_duration.count() << "μs" << std::endl;
        std::cout << "  Total Loop: " << total_duration.count() << "μs" << std::endl;
        
        if (midi_timing_thread_) {
            const auto& timing_stats = midi_timing_thread_->getStats();
            std::cout << "High-Precision Timing Thread:" << std::endl;
            std::cout << "  Average Sleep Accuracy: " << timing_stats.average_sleep_accuracy_us << "μs" << std::endl;
            std::cout << "  Max Sleep Error: " << timing_stats.max_sleep_error_us << "μs" << std::endl;
            std::cout << "  Processed Cycles: " << timing_stats.processed_cycles << std::endl;
            std::cout << "  Missed Deadlines: " << timing_stats.missed_deadlines << std::endl;
        }
        
        if (timestamped_midi_output_) {
            std::cout << "Timestamped MIDI Output:" << std::endl;
            std::cout << "  Scheduled Events: " << timestamped_midi_output_->getScheduledEventCount() << std::endl;
            std::cout << "  Average Latency: " << timestamped_midi_output_->getAverageLatency() << "μs" << std::endl;
            std::cout << "  Max Latency: " << timestamped_midi_output_->getMaxLatency() << "μs" << std::endl;
        }
        
        auto& clock_mgr = MIDI::ThreadedMidiClockManager::getInstance();
        std::cout << "MIDI Clock:" << std::endl;
        std::cout << "  Clock Running: " << (clock_mgr.isClockRunning() ? "YES" : "NO") << std::endl;
        std::cout << "  Current BPM: " << clock_mgr.getBPM() << std::endl;
        std::cout << "  Current Tick: " << clock_mgr.getCurrentTick() << std::endl;
        std::cout << "======================================================\n" << std::endl;
    }    /**
     * @brief Main loop - similar to SynthApp but with background MIDI processing
     */
    void loop() {
        if (!initialized_.load()) return;
        
        // ⚡ SOLUTION: Use dedicated timing thread approach
        // The MidiTimingThread runs independently at 10kHz
        // UI can take 50ms without affecting MIDI timing!
        
        auto loop_start = std::chrono::steady_clock::now();
        
        // PHASE 1: Schedule MIDI events (fast, non-blocking)
        auto midi_start = std::chrono::steady_clock::now();
        
        // Process threaded clock events (RT thread -> UI thread)
        // This includes our ControlClockObserver which updates MidiControlIntegration
        MIDI::ThreadedMidiClockManager::getInstance().processEvents();
        
        // Process sequencer events (sequencer -> timestamped scheduler)
        if (step_sequencer_) {
            step_sequencer_->processSequencerEvents();
            // Events are now scheduled with precise timestamps
            // The MidiTimingThread will send them at exactly the right time
        }
        
        auto midi_end = std::chrono::steady_clock::now();
        auto midi_duration = std::chrono::duration_cast<std::chrono::microseconds>(midi_end - midi_start);
        
        // PHASE 2: UI Processing (can take 50ms+ without problems!)
        auto ui_start = std::chrono::steady_clock::now();
        
        // Process UI this frame
        {
            // Handle LVGL updates - can take up to 50ms
            lv_timer_handler();
            
            // Update other components
            MidiClockManager::getInstance().update();
            UnifiedMidiManager::getInstance().update();
            
            if (window_manager_) {
                window_manager_->update();
            }
            
            if (midi_monitor_tab_ptr_) {
                midi_monitor_tab_ptr_->getMonitor().update();
            }
        }
        
        auto ui_end = std::chrono::steady_clock::now();
        auto ui_duration = std::chrono::duration_cast<std::chrono::microseconds>(ui_end - ui_start);
        
        // PHASE 3: Performance monitoring
        auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(ui_end - loop_start);
        
        static int loop_counter = 0;
        if (++loop_counter % 6000 == 0) {
            printDetailedPerformanceStats(midi_duration, ui_duration, total_duration);
        }
        
        // Sleep for base loop rate (timing thread handles MIDI precision)
        Threading::TaskManager::sleep(8);  // ~120Hz base rate
    }
    
    /**
     * @brief Clean shutdown
     */
    void shutdown() {
        if (!initialized_.load()) return;
        
        std::cout << "=== Shutting down Threaded Synth App ===" << std::endl;
        
        initialized_.store(false);
        
        // Remove and cleanup observers
        if (sequencer_midi_output_ && step_sequencer_) {
            step_sequencer_->removeSequencerObserver(sequencer_midi_output_.get());
            sequencer_midi_output_.reset();
        }
        
        if (step_sequencer_) {
            MIDI::ThreadedMidiClockManager::getInstance().removeClockObserver(step_sequencer_.get());
            step_sequencer_.reset();
        }
        
        if (metronome_observer_) {
            MIDI::ThreadedMidiClockManager::getInstance().removeClockObserver(metronome_observer_.get());
            metronome_observer_.reset();
        }
        
        // Stop UI thread
        if (ui_thread_) {
            ui_thread_->stop();
            ui_thread_.reset();
        }
        
        // Stop threaded MIDI clock manager
        MIDI::ThreadedMidiClockManager::getInstance().stop();
        
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
    void setupMemoryMonitoring() {
#ifdef MEMORY_LEAK_DETECTION
        // Start simple memory monitoring
        auto& monitor = SimpleMemoryMonitor::getInstance();
        monitor.startMonitoring();
        
        std::cout << "[ThreadedSynthApp] ✅ Simple memory monitoring initialized" << std::endl;
        MEMORY_CHECKPOINT("Application startup complete");
        
#ifdef MEMORY_STRESS_TEST
        // Run basic stress test if enabled
        std::cout << "[ThreadedSynthApp] 🧪 Running basic memory test..." << std::endl;
        MEMORY_CHECKPOINT("Pre-stress test");
        
        // Simple allocation test
        for (int i = 0; i < 100; ++i) {
            void* ptr = malloc(1024);
            MEMORY_ALLOC(1024, "stress test");
            if (ptr) {
                free(ptr);
                MEMORY_DEALLOC(1024, "stress test");
            }
        }
        
        MEMORY_CHECKPOINT("Post-stress test");
        std::cout << "[ThreadedSynthApp] ✅ Basic memory test completed" << std::endl;
#endif
#else
        std::cout << "[ThreadedSynthApp] Memory monitoring disabled" << std::endl;
#endif
    }

    void initializeParameterSystem() {
        // Initialize parameter registry with default parameters
        Parameters::initializeDefaultParameters();
        
        // Initialize parameter manager
        auto& param_manager = Parameters::ParameterManager::getInstance();
        param_manager.initialize();
        
        // Initialize MIDI parameter bridge
        auto& midi_bridge = Parameters::MidiParameterBridge::getInstance();
        midi_bridge.initialize();
        
        std::cout << "[ThreadedSynthApp] Parameter system components initialized" << std::endl;
    }
    
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
            (void)mouse; // Suppress unused warning
            auto mousewheel = lv_sdl_mousewheel_create();
            (void)mousewheel; // Suppress unused warning
            auto keyboard = lv_sdl_keyboard_create();
            (void)keyboard; // Suppress unused warning
            
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
        
        // Register MidiHandler as RT observer for immediate MIDI clock sending
        MidiClockManager::getInstance().addRTObserver(midi_handler_.get());
        std::cout << "[ThreadedSynthApp] MidiHandler registered as RT clock observer" << std::endl;
        
        // Initialize unified parameter system
        initializeParameterSystem();
        std::cout << "[ThreadedSynthApp] ⚡ Unified parameter system initialized" << std::endl;
        
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
            
            // Process threaded clock events (RT thread -> UI thread)
            MIDI::ThreadedMidiClockManager::getInstance().processEvents();
            
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
