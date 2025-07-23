#pragma once

#include "InputEvent.h"
#include "InputObserver.h"
#include <unordered_map>
#include <chrono>

#if !defined(ESP32_BUILD)

namespace Input {

    /**
     * @brief Desktop Input Handler using SDL2
     * 
     * Handles keyboard input on desktop platforms
     */
    class DesktopHandler : public Subject {
    private:
        bool initialized_ = false;
        
        // Key state tracking
        std::unordered_map<int, bool> key_states_;
        std::unordered_map<int, std::chrono::steady_clock::time_point> key_press_times_;
        
        // Modifier state
        uint8_t current_modifiers_ = 0;
        
    public:
        DesktopHandler() = default;
        ~DesktopHandler() = default;
        
        bool initialize();
        void shutdown();
        void update();
        
        // SDL event handling
        void handleSDLEvent(const void* sdl_event);  // void* to avoid SDL dependency in header
        
        bool isInitialized() const { return initialized_; }
        
    private:
        KeyCode mapSDLKeyToKeyCode(int sdl_key) const;
        uint8_t updateModifiers(int sdl_key, bool pressed);
        uint32_t getCurrentTimeMs() const;
        void sendEvent(EventType type, KeyCode key_code, bool is_repeat = false);
    };

} // namespace Input

#endif // !ESP32_BUILD
