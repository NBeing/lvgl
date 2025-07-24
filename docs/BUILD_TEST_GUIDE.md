# Threaded Architecture - Build Test

## Quick Compilation Test

To test if the new architecture compiles correctly, try building with the threaded architecture disabled first:

```bash
# Test current (original) architecture
pio run -e desktop
```

If that works, then enable the threaded architecture:

```bash
# Edit src/main.cpp and change:
# #define USE_THREADED_ARCHITECTURE 1

# Then test threaded architecture
pio run -e desktop
```

## Expected Build Flow

### Phase 1: Original Architecture (USE_THREADED_ARCHITECTURE = 0)
- Should build exactly as before
- Uses existing SynthApp and MidiHandler
- No threading changes

### Phase 2: Threaded Architecture (USE_THREADED_ARCHITECTURE = 1)
- Compiles new threading system
- Uses ThreadedSynthApp
- Minimal UI with clock tab only

## Troubleshooting Common Build Issues

### 1. Missing includes
```cpp
// Add to files that use std::cout
#include <iostream>

// Add to files that use std::string
#include <string>

// Add to files that use std::algorithm
#include <algorithm>
```

### 2. LVGL compatibility
Make sure LVGL version supports the functions used:
- `lv_obj_create()`
- `lv_label_create()`
- `lv_btn_create()`
- `lv_obj_add_event_cb()`

### 3. Platform-specific issues

**ESP32:**
- Verify FreeRTOS includes work
- Check stack sizes (increase if needed)
- Ensure Core 0/1 pinning is supported

**Desktop:**
- Verify std::thread support
- Check SDL/LVGL desktop setup
- Ensure threading libraries are linked

## Build Configuration

The implementation is designed to:
1. **Not break existing builds** - old code path preserved
2. **Compile on both platforms** - conditional compilation
3. **Start minimal** - just clock tab initially
4. **Scale incrementally** - add tabs one by one

## Next Steps After Successful Build

1. **Run with original architecture** - verify no regression
2. **Switch to threaded architecture** - test basic functionality  
3. **Validate clock display** - check UI appears
4. **Test clock controls** - start/stop buttons work
5. **Monitor console output** - verify threading messages
6. **Add more functionality** - incrementally migrate tabs

## Verification Checklist

- [ ] Original architecture builds
- [ ] Original architecture runs  
- [ ] Threaded architecture builds
- [ ] Threaded architecture runs
- [ ] Clock tab displays
- [ ] Clock controls work
- [ ] Threading messages appear in console
- [ ] No crashes or freezing
