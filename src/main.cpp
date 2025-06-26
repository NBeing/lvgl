// src/main.cpp - Main application entry point
#include <lvgl.h>
#include <iostream>
#include <chrono>

#if defined(ESP32_BUILD)
#include <LovyanGFX.hpp>

// --- Display driver setup for ST7796S with adjacent pins ---
class LGFX_ST7796S : public lgfx::LGFX_Device {
    lgfx::Panel_ST7796   _panel_instance;
    lgfx::Bus_SPI        _bus_instance;
    lgfx::Light_PWM      _light_instance;
    lgfx::Touch_XPT2046  _touch_instance;

public:
    LGFX_ST7796S(void) {
        { // SPI bus config
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI3_HOST; // <-- Use SPI3_HOST for ESP32-S3
            cfg.spi_mode = 0;
            cfg.freq_write = 10000000; // 10 MHz
            cfg.freq_read  = 4000000;  // 8 MHz            
            cfg.spi_3wire  = false;
            cfg.use_lock   = true;
            cfg.dma_channel = 1;
            cfg.pin_sclk = 12;   // SCK
            cfg.pin_mosi = 11;   // MOSI
            cfg.pin_miso = -1;   // Not used
            cfg.pin_dc   = 13;   // DC/RS
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        { // Panel config
            auto cfg = _panel_instance.config();
            cfg.pin_cs = 10;     // CS
            cfg.pin_rst = 9;    // RESET (or -1 if tied to 3.3V)
            cfg.pin_busy = -1;
            cfg.memory_width  = 320;
            cfg.memory_height = 480;
            cfg.panel_width   = 320;
            cfg.panel_height  = 480;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
            cfg.readable   = false;
            cfg.invert     = true;
            cfg.rgb_order  = false;
            cfg.dlen_16bit = false;
            cfg.bus_shared = false;
            _panel_instance.config(cfg);
        }
        { // Backlight config (optional)
            auto cfg = _light_instance.config();
            cfg.pin_bl = 21;         // LED (PWM OK)
            cfg.invert = false;
            cfg.freq   = 44100;
            cfg.pwm_channel = 7;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }
        { // Touch config
            auto cfg = _touch_instance.config();
            cfg.x_min = 0;
            cfg.x_max = 479;
            cfg.y_min = 0;
            cfg.y_max = 319;
            cfg.pin_int = 4;    // T_IRQ
            cfg.bus_shared = false;
            cfg.spi_host = SPI2_HOST; // Use a different SPI bus than display
            cfg.freq = 1000000;
            cfg.pin_sclk = 17;  // T_CLK
            cfg.pin_mosi = 14;  // T_DIN
            cfg.pin_miso = 15;  // T_D0
            cfg.pin_cs   = 16;  // T_CS
            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }
        setPanel(&_panel_instance);
    }
};

LGFX_ST7796S tft;

// --- LVGL 9.3.0 display buffer and flush callback ---
static lv_color_t *buf1 = nullptr;
size_t buffer_pixels = 480 * 40;
static lv_display_t *display = nullptr;

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();
    lv_display_flush_ready(disp);
}

void hal_setup_display() {
    tft.begin();
    tft.setRotation(1); // Landscape

    buffer_pixels = tft.width() * 40; // 40 lines, adjust as needed
    if (buf1) heap_caps_free(buf1);
    buf1 = (lv_color_t *)heap_caps_malloc(buffer_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA);

    display = lv_display_create(tft.width(), tft.height());
    lv_display_set_flush_cb(display, my_disp_flush);
    lv_display_set_buffers(display, buf1, NULL, buffer_pixels * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
}

void hal_init() {
    // No-op for ESP32, but required for cross-platform compatibility
}

void hal_delay(int ms) {
    delay(ms);
}
#else
// Desktop simulator HAL
#include <SDL2/SDL.h>
#include <functional>
#include <unordered_map>
#include <memory>
#include <thread>
#include "hal.h"
#endif

// Forward declarations
void create_demo_ui();
void handle_events();

// ==============================================
// MIDI DIAL WIDGET CLASS - 8-Bit Style
// ==============================================

class MidiDial {
public:
    MidiDial(lv_obj_t* parent, const char* label = "", int x = 0, int y = 0);
    ~MidiDial();
    
    void setValue(int value);
    int getValue() const { return current_value_; }
    void setRange(int min_val, int max_val);
    void setColor(lv_color_t color);
    void setPosition(int x, int y);
    void onValueChanged(std::function<void(int)> callback);
    lv_obj_t* getObject() { return container_; }
    void setMidiCC(int cc_number) { midi_cc_ = cc_number; }
    int getMidiCC() const { return midi_cc_; }

private:
    lv_obj_t* container_;
    lv_obj_t* value_label_;
    lv_obj_t* name_label_;
    lv_obj_t* arc_display_;
    
    int current_value_;
    int min_value_;
    int max_value_;
    int midi_cc_;
    lv_color_t arc_color_;
    std::function<void(int)> value_callback_;
    
    void updateDisplay();
    void setupStyling();
    static void click_event_cb(lv_event_t* e);
};

// Static map to link LVGL objects back to C++ instances
static std::unordered_map<lv_obj_t*, MidiDial*> dial_widget_map;

MidiDial::MidiDial(lv_obj_t* parent, const char* label, int x, int y) 
    : current_value_(0)
    , min_value_(0) 
    , max_value_(127)
    , midi_cc_(-1)
    , arc_color_(lv_color_hex(0xFF00FF00))
{
    // Create container object
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, 80, 80);
    lv_obj_set_pos(container_, x, y);
    
    // Store mapping for static callbacks
    dial_widget_map[container_] = this;
    
    // Create parameter name label
    name_label_ = lv_label_create(container_);
    lv_label_set_text(name_label_, label);
    lv_obj_set_style_text_color(name_label_, lv_color_hex(0xFFCCCCCC), 0);
    lv_obj_set_style_text_font(name_label_, &lv_font_montserrat_10, 0);
    lv_obj_align(name_label_, LV_ALIGN_TOP_MID, 0, 2);
    
    // Create arc display using LVGL's built-in arc widget
    arc_display_ = lv_arc_create(container_);
    lv_obj_set_size(arc_display_, 50, 50);
    lv_obj_align(arc_display_, LV_ALIGN_CENTER, 0, -5);
    
    // Configure arc for semicircle (8-bit style)
    lv_arc_set_bg_angles(arc_display_, 180, 0);  // Background semicircle
    lv_arc_set_angles(arc_display_, 180, 180);   // Start with no fill
    lv_arc_set_range(arc_display_, min_value_, max_value_);
    
    // Style the arc for 8-bit look
    lv_obj_set_style_arc_color(arc_display_, lv_color_hex(0xFF444444), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_display_, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_display_, arc_color_, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_display_, 6, LV_PART_INDICATOR);
    
    // Hide the knob for cleaner look
    lv_obj_set_style_bg_opa(arc_display_, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_opa(arc_display_, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc_display_, 0, LV_PART_KNOB);
    
    // Create value display label
    value_label_ = lv_label_create(container_);
    lv_label_set_text(value_label_, "000");
    lv_obj_set_style_text_color(value_label_, arc_color_, 0);
    lv_obj_set_style_text_font(value_label_, &lv_font_montserrat_12, 0);
    lv_obj_align(value_label_, LV_ALIGN_BOTTOM_MID, 0, -2);
    
    // Set up event handlers for click interaction
    lv_obj_add_event_cb(container_, click_event_cb, LV_EVENT_CLICKED, this);
    
    // Apply 8-bit retro styling
    setupStyling();
    
    // Initialize display
    updateDisplay();
    
    std::cout << "Created MIDI dial: " << label << std::endl;
}

MidiDial::~MidiDial() {
    dial_widget_map.erase(container_);
}

void MidiDial::setValue(int value) {
    if (value < min_value_) value = min_value_;
    if (value > max_value_) value = max_value_;
    
    if (current_value_ != value) {
        current_value_ = value;
        updateDisplay();
        
        if (value_callback_) {
            value_callback_(value);
        }
        
        std::cout << "MIDI Dial value changed to: " << value << std::endl;
    }
}

void MidiDial::setRange(int min_val, int max_val) {
    min_value_ = min_val;
    max_value_ = max_val;
    setValue(current_value_);
}

void MidiDial::setColor(lv_color_t color) {
    arc_color_ = color;
    lv_obj_set_style_text_color(value_label_, color, 0);
    lv_obj_set_style_arc_color(arc_display_, color, LV_PART_INDICATOR);
}

void MidiDial::setPosition(int x, int y) {
    lv_obj_set_pos(container_, x, y);
}

void MidiDial::onValueChanged(std::function<void(int)> callback) {
    value_callback_ = callback;
}

void MidiDial::updateDisplay() {
    char text[8];
    snprintf(text, sizeof(text), "%03d", current_value_);
    lv_label_set_text(value_label_, text);
    
    // Update arc value using LVGL's built-in arc widget
    lv_arc_set_value(arc_display_, current_value_);
    
    // Calculate angle for the arc (semicircle from 180° to 0°)
    int value_range = max_value_ - min_value_;
    if (value_range > 0) {
        int angle_range = 180;  // Semicircle
        int current_angle = 180 - ((current_value_ * angle_range) / value_range);
        lv_arc_set_angles(arc_display_, 180, current_angle);
    }
}

void MidiDial::setupStyling() {
    // 8-bit retro container styling
    lv_obj_set_style_bg_color(container_, lv_color_hex(0xFF1a1a1a), 0);
    lv_obj_set_style_bg_opa(container_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(container_, lv_color_hex(0xFF666666), 0);
    lv_obj_set_style_border_width(container_, 2, 0);
    lv_obj_set_style_border_opa(container_, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(container_, 4, 0);
    lv_obj_set_style_pad_all(container_, 4, 0);
}

void MidiDial::click_event_cb(lv_event_t* e) {
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    
    auto it = dial_widget_map.find(obj);
    if (it != dial_widget_map.end()) {
        MidiDial* dial = it->second;
        
        // Increment value on click (for testing)
        int new_value = dial->getValue() + 10;
        if (new_value > dial->max_value_) {
            new_value = dial->min_value_;
        }
        dial->setValue(new_value);
        
        std::cout << "MIDI Dial clicked! New value: " << new_value << std::endl;
    }
}

#if defined(ESP32_BUILD)
static lv_indev_t* touch_indev = nullptr;

// LVGL touch read callback
void touch_read_cb(lv_indev_t* indev_drv, lv_indev_data_t* data) {
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void hal_setup_touch() {
    touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, touch_read_cb);
}
#endif

// ==============================================
// MAIN APPLICATION CLASS
// ==============================================

class SynthApp {
public:
    void setup() {
        std::cout << "=== LVGL Synth GUI Starting ===" << std::endl;
        
        hal_init();
        lv_init();
        hal_setup_display();
        #if defined(ESP32_BUILD)
            hal_setup_touch();
        #endif
        create_demo_ui();
        
        std::cout << "Synth GUI initialized successfully!" << std::endl;
    }
    
    void loop() {
        // Update LVGL tick
        static auto last_tick = std::chrono::steady_clock::now();
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_tick);
        
        if (elapsed.count() > 0) {
            lv_tick_inc(elapsed.count());
            last_tick = current_time;
        }
        
        lv_timer_handler();
        
        #ifdef DESKTOP_BUILD
                handle_events();
        #endif
        
        hal_delay(5);
    }
};

// Global MIDI dials for testing
std::unique_ptr<MidiDial> cutoff_dial;
std::unique_ptr<MidiDial> resonance_dial;
std::unique_ptr<MidiDial> volume_dial;

void create_demo_ui() {
    // Add title
    lv_obj_t* title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "🎛️ LVGL Synth GUI - 8-Bit Style");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF00FF00), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Create MIDI dial widgets
    cutoff_dial = std::make_unique<MidiDial>(lv_screen_active(), "CUTOFF", 50, 80);
    cutoff_dial->setColor(lv_color_hex(0xFF00FF00));  // Green
    cutoff_dial->setMidiCC(74);
    cutoff_dial->setValue(64);
    
    resonance_dial = std::make_unique<MidiDial>(lv_screen_active(), "RESONANCE", 200, 80);
    resonance_dial->setColor(lv_color_hex(0xFFFF3030));  // Red
    resonance_dial->setMidiCC(71);
    resonance_dial->setValue(32);
    
    volume_dial = std::make_unique<MidiDial>(lv_screen_active(), "VOLUME", 350, 80);
    volume_dial->setColor(lv_color_hex(0xFF3080FF));  // Blue
    volume_dial->setMidiCC(7);
    volume_dial->setValue(100);
    
    // Set up value change callbacks
    cutoff_dial->onValueChanged([](int value) {
        std::cout << "Filter Cutoff changed to: " << value << " (CC74)" << std::endl;
    });
    
    resonance_dial->onValueChanged([](int value) {
        std::cout << "Resonance changed to: " << value << " (CC71)" << std::endl;
    });
    
    volume_dial->onValueChanged([](int value) {
        std::cout << "Volume changed to: " << value << " (CC7)" << std::endl;
    });
    
    // Add instructions
    lv_obj_t* instructions = lv_label_create(lv_screen_active());
    lv_label_set_text(instructions, "Click dials to change values • F11=Fullscreen • ESC=Exit");
    lv_obj_set_style_text_color(instructions, lv_color_hex(0xFFCCCCCC), 0);
    lv_obj_set_style_text_font(instructions, &lv_font_montserrat_10, 0);
    lv_obj_align(instructions, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    // Add a test MIDI CC simulation button
    lv_obj_t* test_btn = lv_button_create(lv_screen_active());
    lv_obj_set_size(test_btn, 150, 40);
    lv_obj_align(test_btn, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_bg_color(test_btn, lv_color_hex(0xFF8000FF), 0);  // Purple
    
    lv_obj_t* btn_label = lv_label_create(test_btn);
    lv_label_set_text(btn_label, "Simulate MIDI CC");
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xFFFFFFFF), 0);
    lv_obj_center(btn_label);
    
    // Add event for MIDI simulation
    lv_obj_add_event_cb(test_btn, [](lv_event_t* e) {
        (void)e;  // Suppress unused parameter warning
        static int test_value = 0;
        test_value = (test_value + 25) % 128;
        
        std::cout << "🎵 Simulating MIDI CC - setting all dials to: " << test_value << std::endl;
        
        cutoff_dial->setValue(test_value);
        resonance_dial->setValue(test_value);
        volume_dial->setValue(test_value);
    }, LV_EVENT_CLICKED, nullptr);
    
    // Add screen background
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xFF101010), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);
    
    std::cout << "MIDI Synth GUI created with 3 dials!" << std::endl;
}

// MIDI CC handler function
void handleMidiCC(int cc_number, int value) {
    std::cout << "MIDI CC received: CC" << cc_number << " = " << value << std::endl;
    
    if (cutoff_dial && cutoff_dial->getMidiCC() == cc_number) {
        cutoff_dial->setValue(value);
    } else if (resonance_dial && resonance_dial->getMidiCC() == cc_number) {
        resonance_dial->setValue(value);
    } else if (volume_dial && volume_dial->getMidiCC() == cc_number) {
        volume_dial->setValue(value);
    }
}

// Global app instance
SynthApp app;

#ifdef DESKTOP_BUILD
int main() {

    app.setup();
    
    std::cout << "Starting main loop (press Ctrl+C or close window to exit)" << std::endl;
    
    while (true) {
        app.loop();
    }
    
    return 0;
}
#endif
#ifdef ESP32_BUILD
void setup() {
    Serial.begin(115200);
    app.setup();
}

void loop() {
    app.loop();
}
#endif