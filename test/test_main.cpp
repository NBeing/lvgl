/**
 * @brief Test Runner for Cross-Platform LVGL Applications
 * 
 * Works on both ESP32 and Linux desktop builds
 * Integrates with PlatformIO and native build systems
 */

#include "LVGLTestFramework.h"
#include "RTSafeMidiTests.cpp"

#ifdef ESP32_BUILD
#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(2000); // Wait for serial monitor
    
    Serial.println("🧪 Starting ESP32 LVGL RT-Safe MIDI Tests");
    
    // Initialize LVGL for ESP32
    lv_init();
    
    // Run tests
    runRTSafeMidiTests();
    
    Serial.println("✅ Tests completed on ESP32");
}

void loop() {
    lv_timer_handler();
    delay(5);
}

#else
// Desktop build
#include <iostream>

int main() {
    std::cout << "🧪 Starting Desktop LVGL RT-Safe MIDI Tests" << std::endl;
    
    // Initialize LVGL for desktop
    lv_init();
    
    // Run tests
    runRTSafeMidiTests();
    
    std::cout << "✅ Tests completed on desktop" << std::endl;
    return 0;
}

#endif
