#pragma once

#include "components/input/InputEvent.h"
#include "components/input/InputObserver.h"
#include "components/input/InputManager.h"
#include <vector>
#include <string>
#include <chrono>

/**
 * @brief Test utilities for Input System
 */
namespace Input {
namespace Test {

    /**
     * @brief Mock observer for testing
     */
    class MockObserver : public Observer {
    private:
        std::vector<Event> received_events_;
        
    public:
        void onInputEvent(const Event& event) override {
            received_events_.push_back(event);
        }
        
        const std::vector<Event>& getReceivedEvents() const {
            return received_events_;
        }
        
        void clearEvents() {
            received_events_.clear();
        }
        
        size_t getEventCount() const {
            return received_events_.size();
        }
        
        bool hasEvent(EventType type, KeyCode key) const {
            for (const auto& event : received_events_) {
                if (event.type == type && event.key_code == key) {
                    return true;
                }
            }
            return false;
        }
    };

    /**
     * @brief Mock action observer for testing
     */
    class MockActionObserver : public ActionObserver {
    private:
        struct ActionEvent {
            std::string action;
            Event event;
        };
        std::vector<ActionEvent> received_actions_;
        
    public:
        void onAction(const std::string& action, const Event& event) override {
            received_actions_.push_back({action, event});
        }
        
        const std::vector<ActionEvent>& getReceivedActions() const {
            return received_actions_;
        }
        
        void clearActions() {
            received_actions_.clear();
        }
        
        size_t getActionCount() const {
            return received_actions_.size();
        }
        
        bool hasAction(const std::string& action) const {
            for (const auto& ae : received_actions_) {
                if (ae.action == action) {
                    return true;
                }
            }
            return false;
        }
    };

    /**
     * @brief Event builder for easy test event creation
     */
    class EventBuilder {
    private:
        Event event_;
        
    public:
        EventBuilder() {
            event_.type = EventType::KEY_PRESS;
            event_.source = Source::VIRTUAL;
            event_.key_code = KeyCode::SPACE;
            event_.modifiers = 0;
            event_.analog_value = 0;
            event_.velocity = 0;
            event_.timestamp = getCurrentTimeMs();
            event_.is_repeat = false;
        }
        
        EventBuilder& type(EventType t) { event_.type = t; return *this; }
        EventBuilder& source(Source s) { event_.source = s; return *this; }
        EventBuilder& key(KeyCode k) { event_.key_code = k; return *this; }
        EventBuilder& modifiers(uint8_t m) { event_.modifiers = m; return *this; }
        EventBuilder& analog(int16_t a) { event_.analog_value = a; return *this; }
        EventBuilder& velocity(uint8_t v) { event_.velocity = v; return *this; }
        EventBuilder& timestamp(uint32_t t) { event_.timestamp = t; return *this; }
        EventBuilder& repeat(bool r) { event_.is_repeat = r; return *this; }
        
        EventBuilder& shift() { 
            event_.modifiers |= static_cast<uint8_t>(Modifier::SHIFT); 
            return *this; 
        }
        
        EventBuilder& ctrl() { 
            event_.modifiers |= static_cast<uint8_t>(Modifier::CTRL); 
            return *this; 
        }
        
        EventBuilder& alt() { 
            event_.modifiers |= static_cast<uint8_t>(Modifier::ALT); 
            return *this; 
        }
        
        Event build() const { return event_; }
        
    private:
        uint32_t getCurrentTimeMs() const {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count();
        }
    };

    /**
     * @brief Test runner utilities
     */
    class TestRunner {
    public:
        static bool runAllTests();
        
    private:
        static bool testEventCreation();
        static bool testObserverPattern();
        static bool testInputManager();
        static bool testActionMapping();
        static bool testKeyRepeat();
        static bool testSynthIntegration();
        
        static void reportTest(const std::string& test_name, bool passed);
        static bool assertEqual(const std::string& desc, int expected, int actual);
        static bool assertTrue(const std::string& desc, bool condition);
    };

}} // namespace Input::Test
