#include "DesktopInputHandler.h"
#include <iostream>

#if !defined(ESP32_BUILD)

namespace Input {

    bool DesktopHandler::initialize() {
        std::cout << "DesktopInputHandler: Initializing..." << std::endl;
        initialized_ = true;
        return true;
    }

    void DesktopHandler::shutdown() {
        std::cout << "DesktopInputHandler: Shutting down..." << std::endl;
        initialized_ = false;
    }

    void DesktopHandler::update() {
        // Platform-specific update logic would go here
        // For now, this is just a stub
    }

    void DesktopHandler::handleSDLEvent(const void* sdl_event) {
        // SDL event handling would go here
        // This is a stub implementation
        (void)sdl_event; // Suppress unused parameter warning
    }

    KeyCode DesktopHandler::mapSDLKeyToKeyCode(int sdl_key) const {
        // Simple key mapping - would be expanded for full SDL support
        switch (sdl_key) {
            case 32: return KeyCode::SPACE;
            case 27: return KeyCode::ESCAPE;
            case 13: return KeyCode::ENTER;
            case 9: return KeyCode::TAB;
            default: return KeyCode::SPACE; // Default fallback
        }
    }

    uint8_t DesktopHandler::updateModifiers(int sdl_key, bool pressed) {
        // Modifier key handling would go here
        (void)sdl_key;
        (void)pressed;
        return current_modifiers_;
    }

    uint32_t DesktopHandler::getCurrentTimeMs() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }

    void DesktopHandler::sendEvent(EventType type, KeyCode key_code, bool is_repeat) {
        Event event{
            type,
            Source::KEYBOARD,
            key_code,
            current_modifiers_,
            0, // analog_value
            0, // velocity
            getCurrentTimeMs(),
            is_repeat
        };
        
        notifyObservers(event);
    }

} // namespace Input

#endif // !ESP32_BUILD
