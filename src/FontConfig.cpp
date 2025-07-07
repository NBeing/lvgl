#include "../include/FontConfig.h"

// Define the font sets for each platform
#if defined(ESP32_BUILD)
const FontSet FontA = { &PressStart2P_6, &PressStart2P_8, &PressStart2P_12, &PressStart2P_24 };
const FontSet FontB = { &lv_font_montserrat_8, &lv_font_montserrat_8, &lv_font_montserrat_12, &lv_font_montserrat_24 };
#else
const FontSet FontA = { &PressStart2P_6, &PressStart2P_8, &PressStart2P_12, &PressStart2P_24 };
const FontSet FontB = { &lv_font_montserrat_8, &lv_font_montserrat_8, &lv_font_montserrat_12, &lv_font_montserrat_24 };
#endif
