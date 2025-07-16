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
    // Legacy method - convert to enhanced format
    logMidiMessage(LogEntry::MIDI_INPUT, LogEntry::HARDWARE, "", msg);
}

void MidiLogQueue::logOutput(const char* msg) {
    // Legacy method - convert to enhanced format
    logMidiMessage(LogEntry::MIDI_OUTPUT, LogEntry::HARDWARE, "", msg);
}

void MidiLogQueue::logMidiMessage(LogEntry::Type type, LogEntry::Source source, const char* hex_data, const char* friendly_name) {
    if (count_ >= MAX_ENTRIES) {
        // Buffer full, drop oldest
        read_index_ = (read_index_ + 1) % MAX_ENTRIES;
        count_ = count_ - 1;  // Avoid deprecated volatile operation
    }
    
    // Add new entry with enhanced information
    entries_[write_index_].type = type;
    entries_[write_index_].source = source;
    entries_[write_index_].timestamp = getTimestamp();  // Use the static function directly
    snprintf(entries_[write_index_].hex_data, sizeof(entries_[write_index_].hex_data), "%.30s", hex_data);
    snprintf(entries_[write_index_].friendly_name, sizeof(entries_[write_index_].friendly_name), "%.45s", friendly_name);
    
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

void MidiLogQueue::parseMidiMessage(uint8_t status, uint8_t data1, uint8_t data2, char* hex_out, char* friendly_out) {
    // Format hex representation
    if (status >= 0xF8) {
        // Real-time message (single byte)
        snprintf(hex_out, 32, "%02X", status);
    } else if ((status & 0xF0) == 0xC0 || (status & 0xF0) == 0xD0) {
        // Program Change or Channel Pressure (2 bytes)
        snprintf(hex_out, 32, "%02X %02X", status, data1);
    } else {
        // Most messages (3 bytes)
        snprintf(hex_out, 32, "%02X %02X %02X", status, data1, data2);
    }
    
    // Parse friendly name
    uint8_t msg_type = status & 0xF0;
    uint8_t channel = (status & 0x0F) + 1; // Convert to 1-based
    
    if (status >= 0xF8) {
        // Real-time messages
        switch (status) {
            case 0xF8: snprintf(friendly_out, 48, "Clock"); break;
            case 0xFA: snprintf(friendly_out, 48, "Start"); break;
            case 0xFB: snprintf(friendly_out, 48, "Continue"); break;
            case 0xFC: snprintf(friendly_out, 48, "Stop"); break;
            case 0xFE: snprintf(friendly_out, 48, "Active Sensing"); break;
            case 0xFF: snprintf(friendly_out, 48, "Reset"); break;
            default: snprintf(friendly_out, 48, "RT 0x%02X", status); break;
        }
    } else {
        switch (msg_type) {
            case 0x80:
                snprintf(friendly_out, 48, "Note Off Ch%d Note:%d Vel:%d", channel, data1, data2);
                break;
            case 0x90:
                if (data2 == 0) {
                    snprintf(friendly_out, 48, "Note Off Ch%d Note:%d Vel:0", channel, data1);
                } else {
                    snprintf(friendly_out, 48, "Note On Ch%d Note:%d Vel:%d", channel, data1, data2);
                }
                break;
            case 0xA0:
                snprintf(friendly_out, 48, "Poly AT Ch%d Note:%d Pressure:%d", channel, data1, data2);
                break;
            case 0xB0:
                snprintf(friendly_out, 48, "CC Ch%d CC:%d Val:%d", channel, data1, data2);
                break;
            case 0xC0:
                snprintf(friendly_out, 48, "Program Ch%d Prog:%d", channel, data1);
                break;
            case 0xD0:
                snprintf(friendly_out, 48, "Ch AT Ch%d Pressure:%d", channel, data1);
                break;
            case 0xE0:
                snprintf(friendly_out, 48, "Pitch Bend Ch%d Val:%d", channel, (data2 << 7) | data1);
                break;
            default:
                snprintf(friendly_out, 48, "Unknown 0x%02X", status);
                break;
        }
    }
}

} // namespace UI
