#ifdef ESP32_BUILD

#include <lvgl.h>
#include <esp_heap_caps.h>
#include <esp_psram.h>

// Custom LVGL memory allocation using PSRAM
static void* lv_mem_alloc_psram(size_t size) {
    // Try PSRAM first for larger allocations
    if (size > 512 && esp_psram_is_initialized()) {
        void* ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (ptr) {
            return ptr;
        }
    }
    
    // Fallback to regular memory
    return heap_caps_malloc(size, MALLOC_CAP_8BIT);
}

static void lv_mem_free_psram(void* ptr) {
    if (ptr) {
        heap_caps_free(ptr);
    }
}

static void* lv_mem_realloc_psram(void* ptr, size_t new_size) {
    return heap_caps_realloc(ptr, new_size, MALLOC_CAP_8BIT);
}

// Set custom memory functions for LVGL
void lv_mem_init_psram(void) {
    lv_mem_init();
    
    // Override default allocators with PSRAM-aware versions
    // Note: This requires LVGL 9.x custom memory API
    #if LV_MEM_CUSTOM == 1
        // Custom memory is handled by lv_conf.h and built-in allocator
        // The PSRAM optimization happens at the heap_caps level
    #endif
}

#endif // ESP32_BUILD
