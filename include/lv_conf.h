#ifndef LV_CONF_H
#define LV_CONF_H

// Color depth
#define LV_COLOR_DEPTH 16

// Color depth - try 32-bit for desktop to see if that helps
#ifdef ESP32_BUILD
    #define LV_COLOR_DEPTH 16
    #define LV_COLOR_16_SWAP 0  // ESP32 - no swap
#else
    #define LV_COLOR_DEPTH 32   // Desktop - try 32-bit color
    // #define LV_COLOR_16_SWAP 0  // Not needed for 32-bit
#endif
// Memory management
#define LV_MEM_CUSTOM           1
#define LV_MEM_SIZE             (128U * 1024U)

// Performance optimizations
#define LV_USE_PERF_MONITOR     0
#define LV_USE_MEM_MONITOR      0
#define LV_DISP_DEF_REFR_PERIOD 5

// Tick settings
#define LV_TICK_CUSTOM          1

// Enable features
#define LV_USE_ANIMATION        1
#define LV_USE_ARC              1
#define LV_USE_BAR              1
#define LV_USE_BTN              1
#define LV_USE_BTNMATRIX        1
#define LV_USE_CANVAS           1
#define LV_USE_CHECKBOX         1
#define LV_USE_DROPDOWN         1
#define LV_USE_IMG              1
#define LV_USE_LABEL            1
#define LV_USE_LINE             1
#define LV_USE_ROLLER           1
#define LV_USE_SLIDER           1
#define LV_USE_SWITCH           1
#define LV_USE_TEXTAREA         1
#define LV_USE_TABLE            1

// Font settings
#define LV_FONT_MONTSERRAT_8    1
#define LV_FONT_MONTSERRAT_10   1
#define LV_FONT_MONTSERRAT_12   1
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_MONTSERRAT_18   1
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_22   1
#define LV_FONT_MONTSERRAT_24   1

// SDL driver settings
#ifndef ESP32_BUILD  
    #define LV_USE_SDL              1
    #if LV_USE_SDL
        #define LV_SDL_INCLUDE_PATH    <SDL2/SDL.h>
        #define LV_SDL_BUF_COUNT       2
        #define LV_SDL_FULLSCREEN      0
        #define LV_SDL_DIRECT_EXIT     1
    #endif
#else
    #define LV_USE_SDL              0
#endif

// Log settings
#define LV_USE_LOG              1
#if LV_USE_LOG
    #define LV_LOG_LEVEL            LV_LOG_LEVEL_INFO
    #define LV_LOG_PRINTF           1
#endif

#endif /*LV_CONF_H*/