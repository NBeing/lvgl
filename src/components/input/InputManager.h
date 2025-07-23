#pragma once

#include "InputEvent.h"
#include "InputObserver.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

namespace Input {

    // Forward declarations for platform-specific handlers
    class DesktopHandler;
    class ESP32Handler;

    /**
     * @brief Unified Input Manager - Singleton
     * 
     * Manages input from all sources and provides action-based input mapping
     */
    class Manager : public Subject, public Observer {
    private:
        static std::unique_ptr<Manager> instance_;
        
        // Platform-specific handlers
        #if !defined(ESP32_BUILD)
        std::unique_ptr<DesktopHandler> desktop_handler_;
        #endif
        #if defined(ESP32_BUILD)
        std::unique_ptr<ESP32Handler> esp32_handler_;
        #endif
        
        // Action mapping system
        std::vector<ActionMapping> action_mappings_;
        std::unordered_map<std::string, std::vector<ActionObserver*>> action_observers_;
        
        // Key state tracking
        std::unordered_map<KeyCode, bool> key_states_;
        std::unordered_map<KeyCode, uint32_t> key_press_times_;
        
        // Configuration
        uint32_t key_repeat_delay_ = 500;    // ms before key repeat starts
        uint32_t key_repeat_rate_ = 50;      // ms between repeats
        bool enabled_ = true;
        
        Manager() = default;
        
    public:
        static Manager& getInstance();
        
        // Lifecycle
        void initialize();
        void shutdown();
        void update();
        
        // Enable/disable input processing
        void setEnabled(bool enabled) { enabled_ = enabled; }
        bool isEnabled() const { return enabled_; }
        
        // Action mapping
        void mapKey(KeyCode key, const std::string& action, uint8_t modifiers = 0, Source source = Source::VIRTUAL);
        void unmapKey(KeyCode key, const std::string& action);
        void clearMappings();
        
        // Action observers
        void registerAction(const std::string& action, ActionObserver* observer);
        void unregisterAction(const std::string& action, ActionObserver* observer);
        
        // Key state queries
        bool isKeyPressed(KeyCode key) const;
        uint32_t getKeyPressTime(KeyCode key) const;
        
        // Configuration
        void setKeyRepeatSettings(uint32_t delay_ms, uint32_t rate_ms);
        
        // Manual event injection (for testing)
        void injectEvent(const Event& event);
        
        // Observer interface implementation
        void onInputEvent(const Event& event) override;
        
        // Default key mappings for synth
        void setupDefaultMappings();
        
    private:
        void processEvent(const Event& event);
        void updateKeyStates();
        void handleKeyRepeat();
        void notifyActionObservers(const std::string& action, const Event& event);
        
        uint32_t getCurrentTimeMs() const;
    };

} // namespace Input
