#pragma once

#include "lvgl.h"

// Forward declare all fonts you use (adjust as needed)
extern lv_font_t PressStart2P_6;
extern lv_font_t PressStart2P_8;
extern lv_font_t PressStart2P_12;
extern lv_font_t PressStart2P_24;
extern const lv_font_t lv_font_montserrat_6;
extern const lv_font_t lv_font_montserrat_8;
extern const lv_font_t lv_font_montserrat_12;
extern const lv_font_t lv_font_montserrat_24;


// Struct-based font set abstraction
typedef struct FontSet {
    const lv_font_t* small;
    const lv_font_t* med;
    const lv_font_t* lg;
    const lv_font_t* xl;
} FontSet;

// Externally defined in FontConfig.cpp
extern const FontSet FontA;
extern const FontSet FontB;
