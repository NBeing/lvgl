#pragma once

/**
 * @brief LVGL Cross-Platform Test Framework
 * 
 * Combines Unity (embedded-focused) with our custom RT-safe testing
 * Works on both ESP32 and Linux desktop builds
 */

#ifdef ESP32_BUILD
    // ESP32: Use Unity for lightweight testing
    #include "unity.h"
    #define LVGL_TEST_CASE(name) void test_##name(void)
    #define LVGL_ASSERT_EQ(expected, actual) TEST_ASSERT_EQUAL(expected, actual)
    #define LVGL_ASSERT_TRUE(condition) TEST_ASSERT_TRUE(condition)
    #define LVGL_ASSERT_NOT_NULL(ptr) TEST_ASSERT_NOT_NULL(ptr)
    
    // ESP32 RT-safe constraints (stricter)
    #define MAX_RT_MICROSECONDS 50  // ESP32 has tighter timing
    #define MAX_RT_MEMORY 0         // No allocation on ESP32
    
#else
    // Desktop: Use our enhanced framework
    #include "TestFramework.h"
    #define LVGL_TEST_CASE(name) TEST(#name)
    #define LVGL_ASSERT_EQ(expected, actual) ASSERT_EQ(expected, actual)
    #define LVGL_ASSERT_TRUE(condition) ASSERT_TRUE(condition)
    #define LVGL_ASSERT_NOT_NULL(ptr) ASSERT_NOT_NULL(ptr)
    
    // Desktop RT-safe constraints (more relaxed)
    #define MAX_RT_MICROSECONDS 100 // Desktop can handle slightly longer
    #define MAX_RT_MEMORY 0         // Still no allocation
#endif

// Cross-platform LVGL-specific test utilities
#include "lvgl.h"

namespace LVGLTest {

/**
 * @brief LVGL-specific test utilities that work on both platforms
 */
class LVGLTestHelper {
public:
    // LVGL object lifecycle testing
    static lv_obj_t* createTestScreen() {
        lv_obj_t* screen = lv_obj_create(NULL);
        lv_scr_load(screen);
        return screen;
    }
    
    static void cleanupTestScreen(lv_obj_t* screen) {
        if (screen) {
            lv_obj_del(screen);
        }
    }
    
    // MIDI control testing
    static lv_obj_t* createTestDial(uint8_t cc_number) {
        lv_obj_t* dial = lv_arc_create(lv_scr_act());
        lv_arc_set_range(dial, 0, 127);
        // Attach CC number as user data
        lv_obj_set_user_data(dial, (void*)(uintptr_t)cc_number);
        return dial;
    }
    
    // Event simulation
    static void simulateDialChange(lv_obj_t* dial, int16_t value) {
        lv_arc_set_value(dial, value);
        lv_event_send(dial, LV_EVENT_VALUE_CHANGED, NULL);
    }
    
    // RT-safe timing validation
    static bool validateRTTiming(std::function<void()> operation) {
        auto start = lv_tick_get();
        operation();
        auto duration = lv_tick_get() - start;
        return duration * 1000 < MAX_RT_MICROSECONDS; // Convert ms to μs
    }
    
    // Memory leak detection for LVGL objects
    static size_t getObjectCount() {
        // Use LVGL's built-in memory monitoring
        #if LV_USE_MEM_MONITOR
        lv_mem_monitor_t mon;
        lv_mem_monitor(&mon);
        return mon.used_cnt;
        #else
        return 0;
        #endif
    }
};

/**
 * @brief Mock MIDI interface for testing
 */
class MockMidiInterface {
private:
    struct MidiMessage {
        uint8_t status;
        uint8_t data1;
        uint8_t data2;
    };
    
    std::vector<MidiMessage> sent_messages_;
    std::vector<MidiMessage> received_messages_;
    
public:
    void sendCC(uint8_t channel, uint8_t controller, uint8_t value) {
        sent_messages_.push_back({
            static_cast<uint8_t>(0xB0 | (channel - 1)),
            controller,
            value
        });
    }
    
    void simulateReceiveCC(uint8_t channel, uint8_t controller, uint8_t value) {
        received_messages_.push_back({
            static_cast<uint8_t>(0xB0 | (channel - 1)),
            controller,
            value
        });
    }
    
    bool wasControllerSent(uint8_t controller, uint8_t value) const {
        for (const auto& msg : sent_messages_) {
            if (msg.data1 == controller && msg.data2 == value) {
                return true;
            }
        }
        return false;
    }
    
    void clear() {
        sent_messages_.clear();
        received_messages_.clear();
    }
    
    size_t getSentMessageCount() const { return sent_messages_.size(); }
};

} // namespace LVGLTest

// Cross-platform test macros
#define LVGL_RT_TEST(name) \
    LVGL_TEST_CASE(name) { \
        auto start_objects = LVGLTest::LVGLTestHelper::getObjectCount(); \
        auto test_screen = LVGLTest::LVGLTestHelper::createTestScreen();

#define END_LVGL_RT_TEST() \
        LVGLTest::LVGLTestHelper::cleanupTestScreen(test_screen); \
        auto end_objects = LVGLTest::LVGLTestHelper::getObjectCount(); \
        LVGL_ASSERT_EQ(start_objects, end_objects); /* No object leaks */ \
    }

#define ASSERT_RT_SAFE(operation) \
    LVGL_ASSERT_TRUE(LVGLTest::LVGLTestHelper::validateRTTiming([&](){ operation; }))

#define ASSERT_MIDI_CC_SENT(mock, controller, value) \
    LVGL_ASSERT_TRUE(mock.wasControllerSent(controller, value))
