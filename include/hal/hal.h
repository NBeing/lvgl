// src/hal/hal.h - Common HAL interface
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Common HAL functions
void hal_init();
void hal_setup_display();
void hal_delay(int ms);
unsigned long hal_get_tick();

#ifdef __cplusplus
}
#endif