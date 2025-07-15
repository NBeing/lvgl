#pragma once
#include <queue>
#include <string>

namespace UI {

// Simple, thread-safe MIDI log queue
class MidiLogQueue {
public:
    struct LogEntry {
        enum Type { MIDI_INPUT, MIDI_OUTPUT };
        Type type;
        char message[80]; // Fixed size, no dynamic allocation
    };
    
    static MidiLogQueue& getInstance() {
        static MidiLogQueue instance;
        return instance;
    }
    
    // Thread/interrupt safe - just add to queue
    void logInput(const char* msg);
    void logOutput(const char* msg);
    
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
    
    void addEntry(LogEntry::Type type, const char* msg);
};

} // namespace UI
