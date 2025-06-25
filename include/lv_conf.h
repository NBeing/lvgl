#ifndef LV_CONF_H
#define LV_CONF_H

// Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888)
#define LV_COLOR_DEPTH 32

// Memory settings
#define LV_MEM_CUSTOM           0
#define LV_MEM_SIZE             (128U * 1024U)  // Increased memory for better performance

// Performance optimizations
#define LV_USE_PERF_MONITOR     0  // Disable performance monitoring overlay
#define LV_USE_MEM_MONITOR      0  // Disable memory monitoring

// Rendering optimizations
#define LV_DISP_DEF_REFR_PERIOD 5   // Faster refresh (every 5ms instead of 30ms)

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

// Log settings
#define LV_USE_LOG              1
#if LV_USE_LOG
  // #define LV_LOG_LEVEL          LV_LOG_LEVEL_ERROR  // Only show errors
  #define LV_LOG_PRINTF         1
#endif

// Tick settings - simplified for cross-platform
#define LV_TICK_CUSTOM          0

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

#endif /*LV_CONF_H*/