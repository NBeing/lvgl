/**
 * @brief Backlight troubleshooting utility for ESP32-S3 ST7796S displays
 * 
 * This utility helps identify the correct backlight pin by testing common pins
 * and providing manual backlight control functions.
 */

#pragma once
#include <Arduino.h>

class BacklightTroubleshooter {
public:
    /**
     * @brief Test common backlight pins to find the correct one
     * Call this function and observe which pin lights up your display
     */
    static void testCommonBacklightPins() {
        Serial.println("🔍 Testing common ESP32-S3 backlight pins...");
        
        // Common backlight pins for ESP32-S3 boards
        int common_pins[] = {2, 21, 38, 45, 47, 48};
        int num_pins = sizeof(common_pins) / sizeof(common_pins[0]);
        
        for (int i = 0; i < num_pins; i++) {
            int pin = common_pins[i];
            Serial.printf("Testing pin %d...\n", pin);
            
            // Configure pin as output
            pinMode(pin, OUTPUT);
            
            // Turn on backlight
            digitalWrite(pin, HIGH);
            delay(2000); // 2 seconds on
            
            // Turn off backlight  
            digitalWrite(pin, LOW);
            delay(1000); // 1 second off
            
            Serial.printf("Pin %d test complete\n", pin);
        }
        
        Serial.println("✅ Backlight pin test complete!");
        Serial.println("🔍 Which pin lit up your display? Update LGFX_ST7796S.h with that pin number.");
    }
    
    /**
     * @brief Manual backlight control for testing
     */
    static void setBacklight(int pin, bool state) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, state ? HIGH : LOW);
        Serial.printf("📱 Backlight pin %d set to %s\n", pin, state ? "ON" : "OFF");
    }
    
    /**
     * @brief PWM backlight control for testing
     */
    static void setBacklightPWM(int pin, int brightness, int channel = 0) {
        ledcSetup(channel, 5000, 8); // 5kHz, 8-bit resolution
        ledcAttachPin(pin, channel);
        ledcWrite(channel, brightness); // 0-255
        Serial.printf("📱 Backlight pin %d PWM set to %d/255\n", pin, brightness);
    }
    
    /**
     * @brief Check if pin supports PWM output
     */
    static bool canPinDoPWM(int pin) {
        // ESP32-S3 GPIO pins that support PWM
        int pwm_pins[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 
                         19, 20, 21, 35, 36, 37, 38, 39, 40, 41, 42, 45, 46, 47, 48};
        int num_pwm_pins = sizeof(pwm_pins) / sizeof(pwm_pins[0]);
        
        for (int i = 0; i < num_pwm_pins; i++) {
            if (pwm_pins[i] == pin) {
                return true;
            }
        }
        return false;
    }
};
