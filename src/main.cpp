// src/main.cpp - Main application entry point
#include <lvgl.h>
#include <iostream>
#include "components/app/SynthApp.h"

#if defined(ESP32_BUILD)
#include "hardware/LGFX_ST7796S.h"
LGFX_ST7796S tft;  // Global for SynthApp to access

// Custom tick function for ESP32
uint32_t millis_cb() {
    return millis();
}
#endif

// Global app instance
SynthApp app;

#ifdef ESP32_BUILD
void setup() {
    Serial.begin(115200);
    delay(1000);  // Short delay for serial
    
    std::cout << "=== ESP32 SynthApp Starting ===" << std::endl;
    
    app.setup();
}

void loop() {
    app.loop();
}
#else
int main() {
    app.setup();
    
    while (true) {
        app.loop();
    }
    
    return 0;
}
#endif