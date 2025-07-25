#pragma once

#ifdef ESP32_BUILD
    #include <esp_heap_caps.h>
    #include <esp_psram.h>
    #include <Arduino.h>  // For Serial
#endif

#include <cstddef>
#include <cstdint>
#include <vector>
#include <map>
#include <queue>
#include <deque>
#include <iostream>

/**
 * @brief PSRAM-aware memory manager for large buffers
 * 
 * Automatically uses PSRAM when available for large allocations,
 * falls back to regular SRAM for smaller ones.
 */
class PSRAMManager {
public:
    /**
     * @brief Allocate memory with PSRAM preference for large buffers
     * @param size Size in bytes
     * @param use_psram_hint True to prefer PSRAM, false for SRAM
     * @return Pointer to allocated memory or nullptr on failure
     */
    static void* allocate(size_t size, bool use_psram_hint = true) {
        #ifdef ESP32_BUILD
            if (use_psram_hint && size > 1024 && esp_psram_is_initialized()) {
                // Try PSRAM first for large allocations
                void* ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                if (ptr) {
                    return ptr;
                }
            }
            
            // Fallback to regular memory
            return heap_caps_malloc(size, MALLOC_CAP_8BIT);
        #else
            // Desktop: use regular malloc
            return malloc(size);
        #endif
    }
    
    /**
     * @brief Free memory allocated with allocate()
     */
    static void deallocate(void* ptr) {
        if (!ptr) return;
        
        #ifdef ESP32_BUILD
            heap_caps_free(ptr);
        #else
            free(ptr);
        #endif
    }
    
    /**
     * @brief Check if pointer is in PSRAM
     */
    static bool isPSRAM(void* ptr) {
        #ifdef ESP32_BUILD
            return heap_caps_get_allocated_size(ptr) > 0 && 
                   !heap_caps_check_integrity(MALLOC_CAP_SPIRAM, true);
        #else
            return false;  // Desktop has no PSRAM
        #endif
    }
    
    /**
     * @brief Get PSRAM info
     */
    static void printMemoryInfo() {
        #ifdef ESP32_BUILD
            if (esp_psram_is_initialized()) {
                size_t psram_size = esp_psram_get_size();
                size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                size_t free_sram = heap_caps_get_free_size(MALLOC_CAP_8BIT) - free_psram;
                
                std::cout << "PSRAM: " << (psram_size / 1024) << " KB total, " 
                          << (free_psram / 1024) << " KB free" << std::endl;
                std::cout << "SRAM:  " << (free_sram / 1024) << " KB free" << std::endl;
            } else {
                std::cout << "PSRAM: Not initialized" << std::endl;
            }
        #endif
    }
};

/**
 * @brief PSRAM-aware allocator for STL containers
 */
template<typename T>
class PSRAMAllocator {
public:
    using value_type = T;
    
    PSRAMAllocator() = default;
    
    template<typename U>
    PSRAMAllocator(const PSRAMAllocator<U>&) {}
    
    T* allocate(std::size_t n) {
        return static_cast<T*>(PSRAMManager::allocate(n * sizeof(T), n > 64));
    }
    
    void deallocate(T* ptr, std::size_t) {
        PSRAMManager::deallocate(ptr);
    }
    
    template<typename U>
    bool operator==(const PSRAMAllocator<U>&) const { return true; }
    
    template<typename U>
    bool operator!=(const PSRAMAllocator<U>&) const { return false; }
};

// Convenience type aliases for PSRAM containers
template<typename T>
using PSRAMVector = std::vector<T, PSRAMAllocator<T>>;

template<typename Key, typename Value>
using PSRAMMap = std::map<Key, Value, std::less<Key>, 
                         PSRAMAllocator<std::pair<const Key, Value>>>;

template<typename T>
using PSRAMQueue = std::queue<T, std::deque<T, PSRAMAllocator<T>>>;
