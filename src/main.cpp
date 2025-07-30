#include <lvgl.h>
#include <iostream>

// Choose between old and new architecture
#define USE_THREADED_ARCHITECTURE 1

#if USE_THREADED_ARCHITECTURE
    #include "components/app/ThreadedSynthApp.h"
#else
    #include "components/app/SynthApp.h"
#endif

// Include memory monitoring
#include "components/memory/SimpleMemoryMonitor.h"

#ifdef ESP32_BUILD
    #include <Arduino.h>
    // ESP32-specific includes
    
    // Custom tick function for ESP32 (required by LVGL)
    uint32_t millis_cb() {
        return millis();
    }
#else
    // Desktop-specific includes
#endif

// Global application instance
#if USE_THREADED_ARCHITECTURE
    ThreadedSynthApp app;
#else
    SynthApp app;
#endif

#ifdef ESP32_BUILD
    // ESP32 implementation
    void setup() {
        Serial.begin(115200);
        delay(2000); // Wait for serial
        
        std::cout << "=== ESP32 Threaded MIDI Synth Starting ===" << std::endl;
        
        #if USE_THREADED_ARCHITECTURE
            if (!app.initialize()) {
                std::cout << "Failed to initialize threaded app!" << std::endl;
                return;
            }
            std::cout << "Threaded app initialized successfully!" << std::endl;
        #else
            app.setup();
        #endif
    }
    
    void loop() {
        #if USE_THREADED_ARCHITECTURE
            app.loop(); // Much simpler - threads do the work
        #else
            app.loop(); // Old single-threaded approach
        #endif
    }
    
#else
    // Desktop implementation
    int main() {
        std::cout << "=== Desktop Threaded MIDI Synth Starting ===" << std::endl;
        
        // Initialize memory monitoring early
        auto& memory_monitor = SimpleMemoryMonitor::getInstance();
        memory_monitor.startMonitoring();
        MEMORY_CHECKPOINT("Application main() started");
        
        #if USE_THREADED_ARCHITECTURE
            MEMORY_CHECKPOINT("Pre-threaded app initialization");
            
            if (!app.initialize()) {
                std::cout << "Failed to initialize threaded app!" << std::endl;
                memory_monitor.printSummary();
                return -1;
            }
            
            std::cout << "Threaded app initialized successfully!" << std::endl;
            MEMORY_CHECKPOINT("Threaded app initialization complete");
            
            // Main loop
            int loop_count = 0;
            while (true) {
                app.loop();
                
                // Print memory summary periodically
                if (++loop_count % 10000 == 0) {
                    MEMORY_CHECKPOINT("Main loop checkpoint " + std::to_string(loop_count));
                    if (loop_count % 100000 == 0) {
                        memory_monitor.printSummary();
                    }
                }
            }
        #else
            app.setup();
            while (true) {
                app.loop();
            }
        #endif
        
        return 0;
    }
#endif

// Test functions for validating the architecture
namespace Testing {
    void testMidiClock() {
        #if USE_THREADED_ARCHITECTURE
            auto& midi_clock = app.getMidiClock();
            std::cout << "=== Testing MIDI Clock ===" << std::endl;
            
            // Test BPM setting
            midi_clock.setBPM(140.0f);
            std::cout << "Set BPM to 140, current: " << midi_clock.getBPM() << std::endl;
            
            // Test clock start/stop
            std::cout << "Starting clock..." << std::endl;
            midi_clock.play();
            
            // Let it run for a bit
            Threading::TaskManager::sleep(2000);
            
            std::cout << "Current state: " << (midi_clock.isRunning() ? "Playing" : "Stopped") << std::endl;
            
            std::cout << "Stopping clock..." << std::endl;
            midi_clock.stop();
            
            std::cout << "=== MIDI Clock Test Complete ===" << std::endl;
        #endif
    }
    
    void runBasicTests() {
        #if USE_THREADED_ARCHITECTURE
            std::cout << "\n=== Running Basic Architecture Tests ===" << std::endl;
            testMidiClock();
        #endif
    }
}

// Optional: Expose test functions for debugging
#ifdef ENABLE_TESTING
    extern "C" {
        void run_architecture_tests() {
            Testing::runBasicTests();
        }
    }
#endif
