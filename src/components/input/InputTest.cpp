#include "InputTest.h"
#include "SynthInputHandler.h"
#include "components/midi/MidiClockManager.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

namespace Input {
namespace Test {

    bool TestRunner::runAllTests() {
        std::cout << "\n========== Input System Tests ==========\n" << std::endl;
        
        bool all_passed = true;
        
        all_passed &= testEventCreation();
        all_passed &= testObserverPattern();
        all_passed &= testInputManager();
        all_passed &= testActionMapping();
        all_passed &= testKeyRepeat();
        all_passed &= testSynthIntegration();
        
        std::cout << "\n========== Test Results ==========\n";
        std::cout << "Overall: " << (all_passed ? "PASSED" : "FAILED") << std::endl;
        std::cout << "====================================\n" << std::endl;
        
        return all_passed;
    }

    bool TestRunner::testEventCreation() {
        std::cout << "Testing Event Creation..." << std::endl;
        
        // Test basic event creation
        Event event = EventBuilder()
            .type(EventType::KEY_PRESS)
            .source(Source::KEYBOARD)
            .key(KeyCode::SPACE)
            .shift()
            .build();
        
        bool passed = true;
        passed &= assertEqual("Event type", static_cast<int>(EventType::KEY_PRESS), static_cast<int>(event.type));
        passed &= assertEqual("Event source", static_cast<int>(Source::KEYBOARD), static_cast<int>(event.source));
        passed &= assertEqual("Event key", static_cast<int>(KeyCode::SPACE), static_cast<int>(event.key_code));
        passed &= assertTrue("Shift modifier", event.hasModifier(Modifier::SHIFT));
        passed &= assertTrue("Not Ctrl modifier", !event.hasModifier(Modifier::CTRL));
        
        // Test toString
        std::string str = event.toString();
        passed &= assertTrue("ToString not empty", !str.empty());
        
        reportTest("Event Creation", passed);
        return passed;
    }

    bool TestRunner::testObserverPattern() {
        std::cout << "Testing Observer Pattern..." << std::endl;
        
        Subject subject;
        MockObserver observer1, observer2;
        
        // Test adding observers
        subject.addObserver(&observer1);
        subject.addObserver(&observer2);
        bool passed = assertEqual("Observer count", 2, static_cast<int>(subject.getObserverCount()));
        
        // Test event notification
        Event event = EventBuilder().key(KeyCode::A).build();
        subject.notifyObservers(event);
        
        passed &= assertEqual("Observer1 events", 1, static_cast<int>(observer1.getEventCount()));
        passed &= assertEqual("Observer2 events", 1, static_cast<int>(observer2.getEventCount()));
        passed &= assertTrue("Observer1 has event", observer1.hasEvent(EventType::KEY_PRESS, KeyCode::A));
        passed &= assertTrue("Observer2 has event", observer2.hasEvent(EventType::KEY_PRESS, KeyCode::A));
        
        // Test removing observer
        subject.removeObserver(&observer1);
        passed &= assertEqual("Observer count after removal", 1, static_cast<int>(subject.getObserverCount()));
        
        observer1.clearEvents();
        observer2.clearEvents();
        subject.notifyObservers(event);
        
        passed &= assertEqual("Observer1 events after removal", 0, static_cast<int>(observer1.getEventCount()));
        passed &= assertEqual("Observer2 events after removal", 1, static_cast<int>(observer2.getEventCount()));
        
        reportTest("Observer Pattern", passed);
        return passed;
    }

    bool TestRunner::testInputManager() {
        std::cout << "Testing Input Manager..." << std::endl;
        
        auto& manager = Manager::getInstance();
        manager.clearMappings();
        
        MockObserver observer;
        manager.addObserver(&observer);
        
        bool passed = true;
        
        // Test event injection
        Event event = EventBuilder().key(KeyCode::SPACE).build();
        manager.injectEvent(event);
        
        passed &= assertEqual("Manager observer events", 1, static_cast<int>(observer.getEventCount()));
        passed &= assertTrue("Manager has event", observer.hasEvent(EventType::KEY_PRESS, KeyCode::SPACE));
        
        // Test key state tracking
        passed &= assertTrue("Key is pressed", manager.isKeyPressed(KeyCode::SPACE));
        
        // Send key release
        Event release_event = EventBuilder()
            .type(EventType::KEY_RELEASE)
            .key(KeyCode::SPACE)
            .build();
        manager.injectEvent(release_event);
        
        passed &= assertTrue("Key is released", !manager.isKeyPressed(KeyCode::SPACE));
        
        manager.removeObserver(&observer);
        reportTest("Input Manager", passed);
        return passed;
    }

    bool TestRunner::testActionMapping() {
        std::cout << "Testing Action Mapping..." << std::endl;
        
        auto& manager = Manager::getInstance();
        manager.clearMappings();
        
        MockActionObserver action_observer;
        
        // Map key to action
        manager.mapKey(KeyCode::SPACE, "test_action");
        manager.registerAction("test_action", &action_observer);
        
        bool passed = true;
        
        // Test action triggering
        Event event = EventBuilder().key(KeyCode::SPACE).build();
        manager.injectEvent(event);
        
        passed &= assertEqual("Action events", 1, static_cast<int>(action_observer.getActionCount()));
        passed &= assertTrue("Has test action", action_observer.hasAction("test_action"));
        
        // Test action with modifiers
        manager.mapKey(KeyCode::SPACE, "shift_action", static_cast<uint8_t>(Modifier::SHIFT));
        manager.registerAction("shift_action", &action_observer);
        
        action_observer.clearActions();
        Event shift_event = EventBuilder().key(KeyCode::SPACE).shift().build();
        manager.injectEvent(shift_event);
        
        passed &= assertEqual("Shift action events", 1, static_cast<int>(action_observer.getActionCount()));
        passed &= assertTrue("Has shift action", action_observer.hasAction("shift_action"));
        
        manager.unregisterAction("test_action", &action_observer);
        manager.unregisterAction("shift_action", &action_observer);
        
        reportTest("Action Mapping", passed);
        return passed;
    }

    bool TestRunner::testKeyRepeat() {
        std::cout << "Testing Key Repeat..." << std::endl;
        
        auto& manager = Manager::getInstance();
        manager.setKeyRepeatSettings(100, 50); // Fast repeat for testing
        
        MockObserver observer;
        manager.addObserver(&observer);
        
        bool passed = true;
        
        // Send key press
        Event press_event = EventBuilder().key(KeyCode::A).build();
        manager.injectEvent(press_event);
        
        observer.clearEvents();
        
        // Simulate time passing and update calls
        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            manager.update();
        }
        
        // Should have generated repeat events
        passed &= assertTrue("Has repeat events", observer.getEventCount() > 0);
        
        // Send key release
        Event release_event = EventBuilder()
            .type(EventType::KEY_RELEASE)
            .key(KeyCode::A)
            .build();
        manager.injectEvent(release_event);
        
        manager.removeObserver(&observer);
        reportTest("Key Repeat", passed);
        return passed;
    }

    bool TestRunner::testSynthIntegration() {
        std::cout << "Testing Synth Integration..." << std::endl;
        
        auto& manager = Manager::getInstance();
        manager.clearMappings();
        manager.setupDefaultMappings();
        
        SynthInputHandler synth_handler;
        bool init_result = synth_handler.initialize();
        
        bool passed = assertTrue("Synth handler init", init_result);
        
        // Test transport control
        auto& clock_manager = MidiClockManager::getInstance();
        auto initial_state = clock_manager.getTransportState();
        
        // Simulate spacebar press for play/pause
        Event space_event = EventBuilder().key(KeyCode::SPACE).build();
        manager.injectEvent(space_event);
        
        // Check if transport state changed
        auto new_state = clock_manager.getTransportState();
        passed &= assertTrue("Transport state changed", new_state != initial_state);
        
        // Test BPM control
        float initial_bpm = clock_manager.getBPM();
        
        Event plus_event = EventBuilder().key(KeyCode::PLUS).build();
        manager.injectEvent(plus_event);
        
        float new_bpm = clock_manager.getBPM();
        passed &= assertTrue("BPM increased", new_bpm > initial_bpm);
        
        synth_handler.shutdown();
        
        reportTest("Synth Integration", passed);
        return passed;
    }

    void TestRunner::reportTest(const std::string& test_name, bool passed) {
        std::cout << "  " << test_name << ": " << (passed ? "PASSED" : "FAILED") << std::endl;
    }

    bool TestRunner::assertEqual(const std::string& desc, int expected, int actual) {
        if (expected != actual) {
            std::cout << "    FAIL: " << desc << " - Expected: " << expected << ", Actual: " << actual << std::endl;
            return false;
        }
        return true;
    }

    bool TestRunner::assertTrue(const std::string& desc, bool condition) {
        if (!condition) {
            std::cout << "    FAIL: " << desc << " - Expected: true, Actual: false" << std::endl;
            return false;
        }
        return true;
    }

}} // namespace Input::Test
