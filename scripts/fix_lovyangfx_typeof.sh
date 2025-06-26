#!/bin/bash
# filepath: fix_lovyangfx_typeof.sh

# Path to LovyanGFX in PlatformIO libdeps
LOVYANGFX_DIR=".pio/libdeps/rymcu-esp32-s3-devkitc-1/LovyanGFX"

if [ ! -d "$LOVYANGFX_DIR" ]; then
  echo "LovyanGFX directory not found: $LOVYANGFX_DIR"
  exit 1
fi

echo "Patching all 'typeof' to '__typeof__' in $LOVYANGFX_DIR ..."

# Find all .cpp, .hpp, .h files and replace typeof with __typeof__ (but not _typeof)
find "$LOVYANGFX_DIR" -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) \
  -exec sed -i 's/\([^_]\)typeof/\1__typeof__/g' {} +


# ---- PATCH ESP32 ARDUINO CORE portmacro.h MACRO ----
# This script patches the portYIELD_FROM_ISR macro in the ESP32 Arduino core
# to use vPortEvaluateYieldFromISR(0) instead of vPortYieldFromISR
# This is necessary for compatibility with the LovyanGFX library.
PORTMACRO_H="$HOME/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/include/freertos/port/xtensa/include/freertos/portmacro.h"

if [ -f "$PORTMACRO_H" ]; then
  echo "Patching portYIELD_FROM_ISR macro in $PORTMACRO_H ..."
  sed -i '/#define portYIELD_FROM_ISR(/c\
#define portYIELD_FROM_ISR() vPortEvaluateYieldFromISR(0)\
#define portYIELD_FROM_ISR_ARG(x) vPortEvaluateYieldFromISR(0, x)' "$PORTMACRO_H"
  echo "portYIELD_FROM_ISR macro patched."

  # Patch all usages of portYIELD_FROM_ISR(x) to portYIELD_FROM_ISR_ARG(x)
  find "$HOME/.platformio/packages/framework-arduinoespressif32/cores/esp32" \
    -type f -name '*.cpp' -o -name '*.h' | xargs sed -i 's/portYIELD_FROM_ISR(\([^)]\+\))/portYIELD_FROM_ISR_ARG(\1)/g'
  find "$HOME/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/include/arduino_tinyusb/tinyusb/src/osal" \
    -type f -name '*.h' | xargs sed -i 's/portYIELD_FROM_ISR(\([^)]\+\))/portYIELD_FROM_ISR_ARG(\1)/g'
else
  echo "Warning: portmacro.h not found at $PORTMACRO_H, skipping macro patch."
fi

echo "Patch complete."