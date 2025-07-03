#pragma once
// This file is part of the LGFX_ST7796S library for the ESP32-S3 platform.
// It provides a display driver setup for the ST7796S display using the LovyanGFX library.
//// This code is designed to work with the ESP32-S3 and uses SPI3 for display communication
// and SPI2 for touch communication. The display is configured with adjacent pins for SPI, DC, CS, and RESET.
// The touch controller is configured with its own SPI bus and pins for T_CLK, T_DIN, T_D0, and T_CS.
// The backlight is controlled via PWM on a specified pin.
// The display resolution is set to 320x480 pixels, and the touch coordinates are mapped to the same resolution.
// The display uses a 10 MHz write frequency and a 4 MHz read frequency.
// The panel is configured to use 16-bit data length, with no dummy read bits,
// and the RGB order is set to false (BGR format).
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
            cfg.x_max = 319;
            cfg.y_min = 0;
            cfg.y_max = 479;
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

