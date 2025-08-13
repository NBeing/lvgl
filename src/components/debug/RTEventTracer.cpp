#include "RTEventTracer.h"

#if defined(DESKTOP_BUILD) && defined(ENABLE_EVENT_VISUALIZER)

#include <thread>
#include <algorithm>
#include <iostream>

namespace Debug {

RTEventTracer& RTEventTracer::getInstance() {
    static RTEventTracer instance;
    return instance;
}

void RTEventTracer::traceRTEvent(const char* source, const char* target, 
                                const char* event_name, const char* data,
                                EventType type, Priority priority) {
    if (!tracing_enabled_.load(std::memory_order_relaxed)) {
        return;
    }
    
    // Get current write position
    size_t write_pos = write_index_.load(std::memory_order_relaxed);
    size_t next_pos = (write_pos + 1) & BUFFER_MASK;
    
    // Check if buffer is full (would overwrite unread data)
    size_t read_pos = read_index_.load(std::memory_order_acquire);
    if (next_pos == read_pos) {
        // Buffer is full - drop this event
        dropped_events_.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    
    // Fill in the trace entry
    EventTrace& trace = trace_buffer_[write_pos];
    safeCopyString(trace.source_name, source, sizeof(trace.source_name));
    safeCopyString(trace.target_name, target, sizeof(trace.target_name));
    safeCopyString(trace.event_name, event_name, sizeof(trace.event_name));
    safeCopyString(trace.event_data, data, sizeof(trace.event_data));
    trace.timestamp_us = getCurrentTimeMicros();
    trace.thread_id = getCurrentThreadId();
    trace.event_type = static_cast<uint8_t>(type);
    trace.priority = static_cast<uint8_t>(priority);
    
    // Make the entry available (release semantics ensure all writes are visible)
    write_index_.store(next_pos, std::memory_order_release);
    total_events_.fetch_add(1, std::memory_order_relaxed);
}

bool RTEventTracer::popTrace(EventTrace& trace) {
    size_t read_pos = read_index_.load(std::memory_order_relaxed);
    size_t write_pos = write_index_.load(std::memory_order_acquire);
    
    if (read_pos == write_pos) {
        return false; // Buffer is empty
    }
    
    // Copy the trace entry
    trace = trace_buffer_[read_pos];
    
    // Advance read position
    size_t next_read = (read_pos + 1) & BUFFER_MASK;
    read_index_.store(next_read, std::memory_order_release);
    
    return true;
}

size_t RTEventTracer::getAvailableTraces() const {
    size_t write_pos = write_index_.load(std::memory_order_acquire);
    size_t read_pos = read_index_.load(std::memory_order_relaxed);
    
    return (write_pos - read_pos) & BUFFER_MASK;
}

double RTEventTracer::getBufferUtilization() const {
    return static_cast<double>(getAvailableTraces()) / BUFFER_SIZE * 100.0;
}

void RTEventTracer::resetStatistics() {
    total_events_.store(0, std::memory_order_relaxed);
    dropped_events_.store(0, std::memory_order_relaxed);
}

uint64_t RTEventTracer::getCurrentTimeMicros() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

uint32_t RTEventTracer::getCurrentThreadId() const {
    static thread_local uint32_t thread_id = 0;
    if (thread_id == 0) {
        std::hash<std::thread::id> hasher;
        thread_id = static_cast<uint32_t>(hasher(std::this_thread::get_id()));
    }
    return thread_id;
}

void RTEventTracer::safeCopyString(char* dest, const char* src, size_t max_len) {
    if (!src || !dest || max_len == 0) return;
    
    size_t len = std::min(strlen(src), max_len - 1);
    std::memcpy(dest, src, len);
    dest[len] = '\0';
}

} // namespace Debug

#endif // DESKTOP_BUILD && ENABLE_EVENT_VISUALIZER
