#pragma once
#include <queue>
#include <string>
#include <cstdint>  // For uint8_t

namespace UI {

// Simple, thread-safe MIDI log queue
class MidiLogQueue {
public:
    struct LogEntry {
        enum Type { MIDI_INPUT, MIDI_OUTPUT };
        enum Source { HARDWARE, USB, INTERNAL };
        
        Type type;
        Source source;
        unsigned long timestamp;
        char hex_data[32];      // Raw MIDI data in hex format
        char friendly_name[48]; // Human-readable description
    };
    
    static MidiLogQueue& getInstance() {
        static MidiLogQueue instance;
        return instance;
    }
    
    // Thread/interrupt safe - just add to queue
    void logInput(const char* msg);  // Legacy method
    void logOutput(const char* msg); // Legacy method
    
    // Enhanced logging with detailed information
    void logMidiMessage(LogEntry::Type type, LogEntry::Source source, const char* hex_data, const char* friendly_name);
    
    // Utility to parse MIDI data and create friendly names
    static void parseMidiMessage(uint8_t status, uint8_t data1, uint8_t data2, char* hex_out, char* friendly_out);
    
    // Safe to call from main loop
    bool popEntry(LogEntry& entry);
    void clear();
    
private:
    static const int MAX_ENTRIES = 20;
    LogEntry entries_[MAX_ENTRIES];
    volatile int write_index_ = 0;
    volatile int read_index_ = 0;
    volatile int count_ = 0;
    
    MidiLogQueue() = default;
};

} // namespace UI
