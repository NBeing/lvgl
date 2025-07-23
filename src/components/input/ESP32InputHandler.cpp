#include "ESP32InputHandler.h"
#include <iostream>

#if defined(ESP32_BUILD)

namespace Input {

    bool ESP32Handler::initialize() {
        std::cout << "ESP32InputHandler: Initializing..." << std::endl;
        initialized_ = true;
        return true;
    }

    void ESP32Handler::shutdown() {
        std::cout << "ESP32InputHandler: Shutting down..." << std::endl;
        buttons_.clear();
        encoders_.clear();
        initialized_ = false;
    }

    void ESP32Handler::update() {
        if (!initialized_) return;
        
        updateButtons();
        updateEncoders();
    }

    void ESP32Handler::addButton(int pin, KeyCode key_code, bool pull_up, uint32_t debounce_ms, uint32_t hold_threshold_ms) {
        ButtonConfig config{
            pin,
            key_code,
            pull_up,
            debounce_ms,
            false, // current_state
            false, // last_state
            0,     // last_change_time
            0,     // press_start_time
            false, // hold_sent
            hold_threshold_ms
        };
        
        buttons_.push_back(config);
        
        // Configure GPIO - would use ESP32 GPIO API here
        std::cout << "ESP32InputHandler: Added button on pin " << pin << std::endl;
    }

    void ESP32Handler::removeButton(int pin) {
        buttons_.erase(
            std::remove_if(buttons_.begin(), buttons_.end(),
                [pin](const ButtonConfig& config) { return config.pin == pin; }),
            buttons_.end()
        );
    }

    void ESP32Handler::addEncoder(int pin_a, int pin_b, KeyCode key_code_cw, KeyCode key_code_ccw) {
        EncoderConfig config{
            pin_a,
            pin_b,
            key_code_cw,
            key_code_ccw,
            0, // last_a_state
            0, // last_b_state
            0  // position
        };
        
        encoders_.push_back(config);
        
        std::cout << "ESP32InputHandler: Added encoder on pins " << pin_a << ", " << pin_b << std::endl;
    }

    void ESP32Handler::removeEncoder(int pin_a, int pin_b) {
        encoders_.erase(
            std::remove_if(encoders_.begin(), encoders_.end(),
                [pin_a, pin_b](const EncoderConfig& config) { 
                    return config.pin_a == pin_a && config.pin_b == pin_b; 
                }),
            encoders_.end()
        );
    }

    void ESP32Handler::updateButtons() {
        uint32_t current_time = getCurrentTimeMs();
        
        for (auto& button : buttons_) {
            // Read GPIO state - would use ESP32 GPIO API here
            bool gpio_state = false; // Placeholder
            
            // Debouncing logic
            if (gpio_state != button.last_state) {
                if (current_time - button.last_change_time >= button.debounce_ms) {
                    button.current_state = gpio_state;
                    button.last_change_time = current_time;
                    
                    if (button.current_state) {
                        // Button pressed
                        button.press_start_time = current_time;
                        button.hold_sent = false;
                        sendButtonEvent(button, EventType::BUTTON_PRESS);
                    } else {
                        // Button released
                        sendButtonEvent(button, EventType::BUTTON_RELEASE);
                    }
                }
                button.last_state = gpio_state;
            }
            
            // Check for hold event
            if (button.current_state && !button.hold_sent) {
                if (current_time - button.press_start_time >= button.hold_threshold_ms) {
                    button.hold_sent = true;
                    sendButtonEvent(button, EventType::BUTTON_HOLD);
                }
            }
        }
    }

    void ESP32Handler::updateEncoders() {
        for (auto& encoder : encoders_) {
            // Read encoder pins - would use ESP32 GPIO API here
            int a_state = 0; // Placeholder
            int b_state = 0; // Placeholder
            
            // Encoder logic
            if (a_state != encoder.last_a_state) {
                if (a_state == b_state) {
                    encoder.position++;
                    sendEncoderEvent(encoder.key_code_cw, 1);
                } else {
                    encoder.position--;
                    sendEncoderEvent(encoder.key_code_ccw, -1);
                }
                encoder.last_a_state = a_state;
            }
            encoder.last_b_state = b_state;
        }
    }

    uint32_t ESP32Handler::getCurrentTimeMs() const {
        // Would use ESP32 timer API here
        return 0; // Placeholder
    }

    void ESP32Handler::sendButtonEvent(const ButtonConfig& button, EventType type) {
        Event event{
            type,
            Source::HARDWARE_BUTTON,
            button.key_code,
            0, // modifiers
            0, // analog_value
            0, // velocity
            getCurrentTimeMs(),
            false // is_repeat
        };
        
        notifyObservers(event);
    }

    void ESP32Handler::sendEncoderEvent(KeyCode key_code, int direction) {
        Event event{
            EventType::ENCODER_TURN,
            Source::ENCODER,
            key_code,
            0, // modifiers
            static_cast<int16_t>(direction), // analog_value
            0, // velocity
            getCurrentTimeMs(),
            false // is_repeat
        };
        
        notifyObservers(event);
    }

} // namespace Input

#endif // ESP32_BUILD
