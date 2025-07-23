#include "InputManager.h"
#if !defined(ESP32_BUILD)
#include "DesktopInputHandler.h"
#endif
#if defined(ESP32_BUILD)
#include "ESP32InputHandler.h"
#endif
#include <iostream>
#include <chrono>
#include <algorithm>

namespace Input {

    std::unique_ptr<Manager> Manager::instance_ = nullptr;

    Manager& Manager::getInstance() {
        if (!instance_) {
            instance_ = std::unique_ptr<Manager>(new Manager());
        }
        return *instance_;
    }

    void Manager::initialize() {
        std::cout << "InputManager: Initializing..." << std::endl;
        
        #if defined(ESP32_BUILD)
        // Initialize ESP32 handler
        esp32_handler_ = std::make_unique<ESP32Handler>();
        if (esp32_handler_->initialize()) {
            esp32_handler_->addObserver(this);
            std::cout << "InputManager: ESP32 handler initialized" << std::endl;
        }
        #else
        // Initialize desktop handler
        desktop_handler_ = std::make_unique<DesktopHandler>();
        if (desktop_handler_->initialize()) {
            desktop_handler_->addObserver(this);
            std::cout << "InputManager: Desktop handler initialized" << std::endl;
        }
        #endif
        
        setupDefaultMappings();
        std::cout << "InputManager: Initialization complete" << std::endl;
    }

    void Manager::shutdown() {
        std::cout << "InputManager: Shutting down..." << std::endl;
        
        clearObservers();
        action_observers_.clear();
        action_mappings_.clear();
        
        #if defined(ESP32_BUILD)
        if (esp32_handler_) {
            esp32_handler_->shutdown();
            esp32_handler_.reset();
        }
        #else
        if (desktop_handler_) {
            desktop_handler_->shutdown();
            desktop_handler_.reset();
        }
        #endif
    }

    void Manager::update() {
        if (!enabled_) return;
        
        #if defined(ESP32_BUILD)
        if (esp32_handler_) {
            esp32_handler_->update();
        }
        #else
        if (desktop_handler_) {
            desktop_handler_->update();
        }
        #endif
        
        updateKeyStates();
        handleKeyRepeat();
    }

    void Manager::mapKey(KeyCode key, const std::string& action, uint8_t modifiers, Source source) {
        ActionMapping mapping{action, key, modifiers, source};
        
        // Remove existing mapping for this key+modifiers combination
        action_mappings_.erase(
            std::remove_if(action_mappings_.begin(), action_mappings_.end(),
                [&](const ActionMapping& m) {
                    return m.key_code == key && m.required_modifiers == modifiers;
                }),
            action_mappings_.end()
        );
        
        action_mappings_.push_back(mapping);
        std::cout << "InputManager: Mapped key " << static_cast<int>(key) 
                  << " to action '" << action << "'" << std::endl;
    }

    void Manager::unmapKey(KeyCode key, const std::string& action) {
        action_mappings_.erase(
            std::remove_if(action_mappings_.begin(), action_mappings_.end(),
                [&](const ActionMapping& m) {
                    return m.key_code == key && m.action_name == action;
                }),
            action_mappings_.end()
        );
    }

    void Manager::clearMappings() {
        action_mappings_.clear();
        std::cout << "InputManager: Cleared all key mappings" << std::endl;
    }

    void Manager::registerAction(const std::string& action, ActionObserver* observer) {
        if (observer) {
            action_observers_[action].push_back(observer);
            std::cout << "InputManager: Registered observer for action '" << action << "'" << std::endl;
        }
    }

    void Manager::unregisterAction(const std::string& action, ActionObserver* observer) {
        auto it = action_observers_.find(action);
        if (it != action_observers_.end()) {
            auto& observers = it->second;
            observers.erase(
                std::remove(observers.begin(), observers.end(), observer),
                observers.end()
            );
            if (observers.empty()) {
                action_observers_.erase(it);
            }
        }
    }

    bool Manager::isKeyPressed(KeyCode key) const {
        auto it = key_states_.find(key);
        return it != key_states_.end() && it->second;
    }

    uint32_t Manager::getKeyPressTime(KeyCode key) const {
        auto it = key_press_times_.find(key);
        return it != key_press_times_.end() ? it->second : 0;
    }

    void Manager::setKeyRepeatSettings(uint32_t delay_ms, uint32_t rate_ms) {
        key_repeat_delay_ = delay_ms;
        key_repeat_rate_ = rate_ms;
    }

    void Manager::injectEvent(const Event& event) {
        processEvent(event);
    }

    void Manager::setupDefaultMappings() {
        // Transport controls
        mapKey(KeyCode::SPACE, "transport_play_pause");
        mapKey(KeyCode::ESCAPE, "transport_stop");
        mapKey(KeyCode::ENTER, "transport_continue");
        
        // BPM controls
        mapKey(KeyCode::PLUS, "bpm_increase");
        mapKey(KeyCode::MINUS, "bpm_decrease");
        
        // Tab navigation
        mapKey(KeyCode::TAB, "next_tab");
        mapKey(KeyCode::TAB, "prev_tab", static_cast<uint8_t>(Modifier::SHIFT));
        
        // Parameter navigation
        mapKey(KeyCode::ARROW_UP, "param_up");
        mapKey(KeyCode::ARROW_DOWN, "param_down");
        mapKey(KeyCode::ARROW_LEFT, "param_dec");
        mapKey(KeyCode::ARROW_RIGHT, "param_inc");
        
        // Quick actions
        mapKey(KeyCode::F1, "help");
        mapKey(KeyCode::F5, "refresh");
        
        std::cout << "InputManager: Default key mappings configured" << std::endl;
    }

    void Manager::processEvent(const Event& event) {
        if (!enabled_) return;
        
        // Update key states
        if (event.type == EventType::KEY_PRESS || event.type == EventType::BUTTON_PRESS) {
            key_states_[event.key_code] = true;
            key_press_times_[event.key_code] = event.timestamp;
        } else if (event.type == EventType::KEY_RELEASE || event.type == EventType::BUTTON_RELEASE) {
            key_states_[event.key_code] = false;
            key_press_times_.erase(event.key_code);
        }
        
        // Notify direct observers
        notifyObservers(event);
        
        // Check for action mappings
        for (const auto& mapping : action_mappings_) {
            if (mapping.matches(event)) {
                notifyActionObservers(mapping.action_name, event);
            }
        }
    }

    void Manager::updateKeyStates() {
        // This is called from platform handlers via the observer interface
    }

    void Manager::handleKeyRepeat() {
        uint32_t current_time = getCurrentTimeMs();
        
        for (const auto& [key, pressed] : key_states_) {
            if (pressed) {
                auto press_time_it = key_press_times_.find(key);
                if (press_time_it != key_press_times_.end()) {
                    uint32_t press_duration = current_time - press_time_it->second;
                    
                    if (press_duration >= key_repeat_delay_) {
                        // Calculate if it's time for a repeat
                        uint32_t repeat_elapsed = press_duration - key_repeat_delay_;
                        if (repeat_elapsed % key_repeat_rate_ < 20) { // 20ms tolerance
                            Event repeat_event{
                                EventType::KEY_HOLD,
                                Source::VIRTUAL,
                                key,
                                0, // modifiers
                                0, // analog_value
                                0, // velocity
                                current_time,
                                true // is_repeat
                            };
                            processEvent(repeat_event);
                        }
                    }
                }
            }
        }
    }

    void Manager::notifyActionObservers(const std::string& action, const Event& event) {
        auto it = action_observers_.find(action);
        if (it != action_observers_.end()) {
            for (auto* observer : it->second) {
                if (observer) {
                    observer->onAction(action, event);
                }
            }
        }
    }

    uint32_t Manager::getCurrentTimeMs() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }

    // Implement Observer interface to receive events from platform handlers
    void Manager::onInputEvent(const Event& event) {
        processEvent(event);
    }

} // namespace Input
