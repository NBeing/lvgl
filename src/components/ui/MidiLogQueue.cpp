#include "MidiLogQueue.h"
#include <cstring>
#include <cstdio>
#include <iostream>

#if defined(ESP32_BUILD)
#include <Arduino.h>  // For millis()
#else
#include <chrono>     // For timestamp on desktop
#endif

namespace UI {

// Helper function to get timestamp
static unsigned long getTimestamp() {
#if defined(ESP32_BUILD)
    return millis() % 100000;  // ESP32: use millis()
#else
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return ms.count() % 100000;  // Desktop: use chrono
#endif
}

void MidiLogQueue::logInput(const char* msg) {
    // Add timestamp for debugging
    char timestamped_msg[100];
    snprintf(timestamped_msg, sizeof(timestamped_msg), "[%lu] %s", 
             getTimestamp(), msg);
    // std::cout << "[Queue] Adding INPUT: " << timestamped_msg << std::endl;  // DISABLED FOR DEBUG
    addEntry(LogEntry::MIDI_INPUT, timestamped_msg);
}

void MidiLogQueue::logOutput(const char* msg) {
    // Add timestamp for debugging  
    char timestamped_msg[100];
    snprintf(timestamped_msg, sizeof(timestamped_msg), "[%lu] %s", 
             getTimestamp(), msg);
    // std::cout << "[Queue] Adding OUTPUT: " << timestamped_msg << std::endl;  // DISABLED FOR DEBUG
    addEntry(LogEntry::MIDI_OUTPUT, timestamped_msg);
}

void MidiLogQueue::addEntry(LogEntry::Type type, const char* msg) {
    // Completely lock-free, interrupt-safe circular buffer
    if (count_ >= MAX_ENTRIES) {
        // Buffer full, drop oldest
        read_index_ = (read_index_ + 1) % MAX_ENTRIES;
        count_ = count_ - 1;  // Avoid deprecated volatile operation
    }
    
    // Add new entry
    entries_[write_index_].type = type;
    snprintf(entries_[write_index_].message, sizeof(entries_[write_index_].message), "%.75s", msg);
    
    write_index_ = (write_index_ + 1) % MAX_ENTRIES;
    count_ = count_ + 1;  // Avoid deprecated volatile operation
}

bool MidiLogQueue::popEntry(LogEntry& entry) {
    if (count_ == 0) return false;
    
    // Get oldest entry
    entry = entries_[read_index_];
    read_index_ = (read_index_ + 1) % MAX_ENTRIES;
    count_ = count_ - 1;  // Avoid deprecated volatile operation
    
    return true;
}

void MidiLogQueue::clear() {
    read_index_ = 0;
    write_index_ = 0;
    count_ = 0;
}

} // namespace UI
