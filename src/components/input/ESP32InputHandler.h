#pragma once

#include "InputEvent.h"
#include "InputObserver.h"
#include <vector>

#if defined(ESP32_BUILD)

namespace Input {

    /**
     * @brief ESP32 Hardware Input Handler
     * 
     * Handles physical buttons and encoders on ESP32
     */
    class ESP32Handler : public Subject {
    private:
        struct ButtonConfig {
            int pin;
            KeyCode key_code;
            bool pull_up;
            uint32_t debounce_ms;
            bool current_state;
            bool last_state;
            uint32_t last_change_time;
            uint32_t press_start_time;
            bool hold_sent;
            uint32_t hold_threshold_ms;
        };
        
        struct EncoderConfig {
            int pin_a;
            int pin_b;
            KeyCode key_code_cw;   // Clockwise
            KeyCode key_code_ccw;  // Counter-clockwise
            int last_a_state;
            int last_b_state;
            int position;
        };
        
        std::vector<ButtonConfig> buttons_;
        std::vector<EncoderConfig> encoders_;
        bool initialized_ = false;
        
    public:
        ESP32Handler() = default;
        ~ESP32Handler() = default;
        
        bool initialize();
        void shutdown();
        void update();
        
        // Button configuration
        void addButton(int pin, KeyCode key_code, bool pull_up = true, uint32_t debounce_ms = 50, uint32_t hold_threshold_ms = 1000);
        void removeButton(int pin);
        
        // Encoder configuration  
        void addEncoder(int pin_a, int pin_b, KeyCode key_code_cw, KeyCode key_code_ccw);
        void removeEncoder(int pin_a, int pin_b);
        
        bool isInitialized() const { return initialized_; }
        
    private:
        void updateButtons();
        void updateEncoders();
        uint32_t getCurrentTimeMs() const;
        void sendButtonEvent(const ButtonConfig& button, EventType type);
        void sendEncoderEvent(KeyCode key_code, int direction);
    };

} // namespace Input

#endif // ESP32_BUILD
