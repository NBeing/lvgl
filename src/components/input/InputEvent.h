#pragma once

#include <cstdint>
#include <string>

/**
 * @brief Input Event System for Cross-Platform Input Handling
 * 
 * Provides a unified interface for keyboard, button, and other input events
 * across desktop and ESP32 platforms using the observer pattern.
 */
namespace Input {

    enum class EventType {
        KEY_PRESS,
        KEY_RELEASE,
        KEY_HOLD,           // Key held down for extended period
        BUTTON_PRESS,       // Physical button press
        BUTTON_RELEASE,     // Physical button release
        BUTTON_HOLD,        // Physical button held
        ENCODER_TURN,       // Rotary encoder movement
        TOUCH_PRESS,        // Touch screen press
        TOUCH_RELEASE       // Touch screen release
    };

    enum class Source {
        KEYBOARD,           // Desktop keyboard
        HARDWARE_BUTTON,    // Physical buttons (ESP32)
        TOUCH_SCREEN,       // Touch interface
        ENCODER,            // Rotary encoders
        MIDI_CONTROLLER,    // MIDI input device
        VIRTUAL             // Software-generated events
    };

    enum class Modifier {
        NONE = 0,
        SHIFT = 1 << 0,
        CTRL = 1 << 1,
        ALT = 1 << 2,
        META = 1 << 3       // Windows/Cmd key
    };

    // Key codes - common across platforms
    enum class KeyCode {
        // Transport controls
        SPACE = 32,
        ENTER = 13,
        ESCAPE = 27,
        TAB = 9,
        
        // Arrow keys
        ARROW_UP = 256,
        ARROW_DOWN = 257,
        ARROW_LEFT = 258,
        ARROW_RIGHT = 259,
        
        // Function keys
        F1 = 290, F2, F3, F4, F5, F6,
        F7, F8, F9, F10, F11, F12,
        
        // Numbers
        NUM_0 = 48, NUM_1, NUM_2, NUM_3, NUM_4,
        NUM_5, NUM_6, NUM_7, NUM_8, NUM_9,
        
        // Letters
        A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        
        // Special
        PLUS = 43,
        MINUS = 45,
        BACKSPACE = 8,
        DELETE = 127
    };

    struct Event {
        EventType type;
        Source source;
        KeyCode key_code;
        uint8_t modifiers;      // Bitfield of Modifier enum
        int16_t analog_value;   // For encoders/sliders (-32768 to 32767)
        uint8_t velocity;       // For pressure-sensitive inputs (0-127)
        uint32_t timestamp;     // Milliseconds since boot
        bool is_repeat;         // True for auto-repeat key events
        
        // Convenience methods
        bool hasModifier(Modifier mod) const {
            return (modifiers & static_cast<uint8_t>(mod)) != 0;
        }
        
        bool isShiftPressed() const { return hasModifier(Modifier::SHIFT); }
        bool isCtrlPressed() const { return hasModifier(Modifier::CTRL); }
        bool isAltPressed() const { return hasModifier(Modifier::ALT); }
        
        // Convert to string for debugging
        std::string toString() const;
    };

    // Action-based input mapping
    struct ActionMapping {
        std::string action_name;
        KeyCode key_code;
        uint8_t required_modifiers;
        Source allowed_source;
        
        bool matches(const Event& event) const {
            return event.key_code == key_code &&
                   event.modifiers == required_modifiers &&
                   (allowed_source == Source::VIRTUAL || event.source == allowed_source);
        }
    };

} // namespace Input
