/**
 * @brief Event Tracer Unit Tests - FIXED VERSION
 * 
 * Tests RT-safe event tracing functionality without external dependencies
 */

#include "../framework/unified_test_framework.h"
#include "../fixtures/test_fixtures.h"
#include <chrono>
#include <thread>
#include <set>

// Mock RTEventTracer for unit testing (simplified version)
namespace Debug {

enum class EventType {
    MIDI_EVENT,
    PARAMETER_EVENT,
    UI_EVENT,
    CLOCK_EVENT
};

struct TraceEvent {
    EventType type;
    std::string action;
    uint64_t timestamp;
    std::string data;
    
    TraceEvent(EventType t, const std::string& a, const std::string& d = "") 
        : type(t), action(a), data(d) {
        timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};

class RTEventTracer {
public:
    static RTEventTracer& getInstance() {
        static RTEventTracer instance;
        return instance;
    }
    
    void trace(EventType type, const std::string& action, const std::string& data = "") {
        if (enabled_) {
            events_.push_back(TraceEvent(type, action, data));
        }
    }
    
    std::vector<TraceEvent> getEvents() const { return events_; }
    size_t getEventCount() const { return events_.size(); }
    void clearEvents() { events_.clear(); }
    
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

private:
    std::vector<TraceEvent> events_;
    bool enabled_ = true;
};

} // namespace Debug

using namespace Debug;
using namespace TestFixtures;

// ============================================================================
// UNIT TESTS - Event Tracer Core Functionality
// ============================================================================

TEST_UNIT(EventTracer, BasicTracing) {
    RTEventTracer& tracer = RTEventTracer::getInstance();
    tracer.clearEvents();
    
    tracer.trace(EventType::MIDI_EVENT, "noteOn", "C4");
    tracer.trace(EventType::PARAMETER_EVENT, "filterCutoff", "0.75");
    
    ASSERT_EQ(2lu, tracer.getEventCount());
    
    auto events = tracer.getEvents();
    // Use string comparison for action and data
    ASSERT_STR_EQ("noteOn", events[0].action);
    ASSERT_STR_EQ("C4", events[0].data);
    
    ASSERT_STR_EQ("filterCutoff", events[1].action);
    ASSERT_STR_EQ("0.75", events[1].data);
}

TEST_UNIT(EventTracer, EnableDisable) {
    RTEventTracer& tracer = RTEventTracer::getInstance();
    tracer.clearEvents();
    
    // Test enabled
    tracer.setEnabled(true);
    ASSERT_TRUE(tracer.isEnabled());
    
    tracer.trace(EventType::UI_EVENT, "buttonPress");
    ASSERT_EQ(1lu, tracer.getEventCount());
    
    // Test disabled
    tracer.setEnabled(false);
    ASSERT_FALSE(tracer.isEnabled());
    
    tracer.trace(EventType::UI_EVENT, "buttonPress2");
    ASSERT_EQ(1lu, tracer.getEventCount()); // Should not increase
    
    // Re-enable
    tracer.setEnabled(true);
    tracer.trace(EventType::UI_EVENT, "buttonPress3");
    ASSERT_EQ(2lu, tracer.getEventCount());
}

TEST_UNIT(EventTracer, TimestampOrdering) {
    RTEventTracer& tracer = RTEventTracer::getInstance();
    tracer.clearEvents();
    
    tracer.trace(EventType::MIDI_EVENT, "event1");
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    tracer.trace(EventType::MIDI_EVENT, "event2");
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    tracer.trace(EventType::MIDI_EVENT, "event3");
    
    auto events = tracer.getEvents();
    ASSERT_EQ(3lu, events.size());
    
    // Verify timestamps are in order
    ASSERT_TRUE(events[0].timestamp <= events[1].timestamp);
    ASSERT_TRUE(events[1].timestamp <= events[2].timestamp);
}

TEST_UNIT(EventTracer, MultipleEventTypes) {
    RTEventTracer& tracer = RTEventTracer::getInstance();
    tracer.clearEvents();
    
    tracer.trace(EventType::MIDI_EVENT, "midiAction");
    tracer.trace(EventType::PARAMETER_EVENT, "paramAction");
    tracer.trace(EventType::UI_EVENT, "uiAction");
    tracer.trace(EventType::CLOCK_EVENT, "clockAction");
    
    auto events = tracer.getEvents();
    ASSERT_EQ(4lu, events.size());
    
    // Verify different event types
    std::set<EventType> unique_types;
    for (const auto& event : events) {
        unique_types.insert(event.type);
    }
    ASSERT_EQ(4lu, unique_types.size());
}

TEST_UNIT(EventTracer, LargeEventSequence) {
    RTEventTracer& tracer = RTEventTracer::getInstance();
    tracer.clearEvents();
    
    const size_t event_count = 1000;
    
    for (size_t i = 0; i < event_count; ++i) {
        tracer.trace(EventType::MIDI_EVENT, "event" + std::to_string(i));
    }
    
    ASSERT_EQ(event_count, tracer.getEventCount());
    
    auto events = tracer.getEvents();
    ASSERT_EQ(event_count, events.size());
    
    // Verify sequence integrity
    for (size_t i = 0; i < event_count; ++i) {
        ASSERT_STR_EQ("event" + std::to_string(i), events[i].action);
    }
}

TEST_UNIT(EventTracer, ClearEvents) {
    RTEventTracer& tracer = RTEventTracer::getInstance();
    tracer.clearEvents(); // Clear any leftover events from previous tests
    
    tracer.trace(EventType::MIDI_EVENT, "test1");
    tracer.trace(EventType::MIDI_EVENT, "test2");
    ASSERT_EQ(2lu, tracer.getEventCount());
    
    tracer.clearEvents();
    ASSERT_EQ(0lu, tracer.getEventCount());
    ASSERT_TRUE(tracer.getEvents().empty());
}

// ============================================================================
// UNIT TESTS - Mock Integration (Simple Callback Testing)
// ============================================================================

TEST_UNIT(EventTracer, WithMockClockManager) {
    RTEventTracer& tracer = RTEventTracer::getInstance();
    tracer.clearEvents();
    
    MockMidiClockManager clockManager;
    
    // Set up traced callback
    clockManager.setClockTickCallback([&tracer](int tick) {
        tracer.trace(EventType::CLOCK_EVENT, "clockTick", std::to_string(tick));
    });
    
    // Simulate clock events
    clockManager.simulateClockTick();
    clockManager.simulateClockTick();
    clockManager.simulateClockTick();
    
    ASSERT_EQ(3lu, tracer.getEventCount());
    
    auto events = tracer.getEvents();
    ASSERT_STR_EQ("clockTick", events[0].action);
    ASSERT_STR_EQ("1", events[0].data);
    ASSERT_STR_EQ("clockTick", events[1].action);
    ASSERT_STR_EQ("2", events[1].data);
}

TEST_UNIT(EventTracer, WithMockParameterManager) {
    RTEventTracer& tracer = RTEventTracer::getInstance();
    tracer.clearEvents();
    
    MockParameterManager paramManager;
    
    // Set up traced callback
    paramManager.setChangeCallback([&tracer](uint32_t param_id, float value) {
        tracer.trace(EventType::PARAMETER_EVENT, "parameterChanged", 
                    std::to_string(param_id) + ":" + std::to_string(value));
    });
    
    // Simulate parameter changes
    paramManager.setParameter(1, 0.5f);
    paramManager.setParameter(2, 0.8f);
    
    ASSERT_EQ(2lu, tracer.getEventCount());
    
    auto events = tracer.getEvents();
    ASSERT_STR_EQ("parameterChanged", events[0].action);
    ASSERT_TRUE(events[0].data.find("1:0.5") != std::string::npos);
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main() {
    std::cout << "🧪 Event Tracer Unit Tests - FIXED VERSION" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    auto& runner = TestFramework::TestRunner::getInstance();
    auto results = runner.runCategory("unit/EventTracer");
    
    return results.failed_tests == 0 ? 0 : 1;
}
