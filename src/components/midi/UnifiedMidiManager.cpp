#include "UnifiedMidiManager.h"
#include "HardwareMidiBackend.h"
#include "ESP32USBMidiBackend.h"

#if !defined(ESP32_BUILD)
#include "RtMidiBackend.h"
#endif

#include "components/ui/MidiMonitor.h"
#include "components/ui/MidiLogQueue.h"
#include <iostream>
#include <algorithm>

// Static storage for shared MIDI handler
static std::shared_ptr<MidiHandler> shared_midi_handler_ = nullptr;

UnifiedMidiManager& UnifiedMidiManager::getInstance() {
    static UnifiedMidiManager instance;
    return instance;
}

void UnifiedMidiManager::setMidiMonitor(UI::MidiMonitor* monitor) {
    midi_monitor_ = monitor;
}

void UnifiedMidiManager::logMidiInput(uint8_t status, uint8_t data1, uint8_t data2) {
    // Use global queue - thread/interrupt safe!
    uint8_t msg_type = status & 0xF0;
    uint8_t channel = (status & 0x0F) + 1; // Convert to 1-based
    char buf[75]; // Fit in queue message size
    
    switch (msg_type) {
        case 0x90:
            if (data2 > 0) {
                snprintf(buf, sizeof(buf), "NoteOn ch%d %d %d", channel, data1, data2);
            } else {
                snprintf(buf, sizeof(buf), "NoteOff ch%d %d %d", channel, data1, data2);
            }
            break;
        case 0x80:
            snprintf(buf, sizeof(buf), "NoteOff ch%d %d %d", channel, data1, data2);
            break;
        case 0xB0:
            snprintf(buf, sizeof(buf), "CC ch%d %d %d", channel, data1, data2);
            break;
        case 0xC0:
            snprintf(buf, sizeof(buf), "PC ch%d %d", channel, data1);
            break;
        case 0xE0:
            snprintf(buf, sizeof(buf), "PB ch%d %d", channel, (data2 << 7) | data1);
            break;
        default:
            if (status >= 0xF8) {
                snprintf(buf, sizeof(buf), "RT 0x%02X", status);
            } else {
                snprintf(buf, sizeof(buf), "?? 0x%02X %d %d", status, data1, data2);
            }
            break;
    }
    
    UI::MidiLogQueue::getInstance().logInput(buf);
}

// Global callback function to avoid circular dependency
extern "C" void logHardwareMidiInput(uint8_t status, uint8_t data1, uint8_t data2) {
    // Now always safe with global queue approach!
    UnifiedMidiManager::getInstance().logMidiInput(status, data1, data2);
}

void UnifiedMidiManager::setSharedMidiHandler(std::shared_ptr<MidiHandler> handler) {
    shared_midi_handler_ = handler;
    std::cout << "[UnifiedMidiManager] Shared MidiHandler set" << std::endl;
}

void UnifiedMidiManager::initialize() {
    if (initialized_) return;
    
    std::cout << "=== Unified MIDI Manager Initializing ===" << std::endl;
    
    createBackends();
    
    // Try to initialize all available backends
    for (auto& backend : backends_) {
        if (backend->initialize()) {
            std::cout << "✅ " << backend->getName() << " initialized successfully" << std::endl;
        } else {
            std::cout << "❌ " << backend->getName() << " failed to initialize" << std::endl;
        }
    }
    
    initialized_ = true;
    
    // Print status summary
    auto available_backends = getAvailableBackends();
    std::cout << "📊 MIDI Backend Summary:" << std::endl;
    for (const auto& info : available_backends) {
        std::cout << "  - " << info.name << ": " 
                  << (info.status == ConnectionStatus::CONNECTED ? "✅ Connected" : 
                      info.status == ConnectionStatus::ERROR ? "❌ Error" : "⏸️ Disconnected")
                  << " (In:" << (info.supports_input ? "✅" : "❌") 
                  << " Out:" << (info.supports_output ? "✅" : "❌") << ")" << std::endl;
    }
    
    std::cout << "=== Unified MIDI Manager Ready ===" << std::endl;
}

void UnifiedMidiManager::cleanup() {
    for (auto& backend : backends_) {
        backend->cleanup();
    }
    backends_.clear();
    initialized_ = false;
    std::cout << "Unified MIDI Manager cleaned up" << std::endl;
}

void UnifiedMidiManager::createBackends() {
    backends_.clear();
    
#if !defined(ESP32_BUILD)
    // Add RtMidi backend for desktop with shared MidiHandler
    auto rtmidi_backend = std::make_unique<RtMidiBackend>();
    if (shared_midi_handler_) {
        rtmidi_backend->setMidiHandler(shared_midi_handler_);
        std::cout << "[UnifiedMidiManager] Using shared MidiHandler for RtMidi backend" << std::endl;
    }
    backends_.push_back(std::move(rtmidi_backend));
#endif
    
#if defined(ESP32_BUILD)
    // Add Hardware MIDI backend for ESP32 (Serial MIDI: TX on pin 3, RX on pin 46)
    auto hw_backend = std::make_unique<HardwareMidiBackend>();
    hw_backend->setPins(3, 5); // TX on pin 3, RX on pin 5 for MIDI input
    backends_.push_back(std::move(hw_backend));
    
    // Add USB MIDI backend for ESP32
    backends_.push_back(std::make_unique<ESP32USBMidiBackend>());
#else
    // Add Hardware MIDI backend for desktop simulation
    backends_.push_back(std::make_unique<HardwareMidiBackend>());
#endif
}

std::vector<UnifiedMidiManager::BackendInfo> UnifiedMidiManager::getAvailableBackends() const {
    std::vector<BackendInfo> info_list;
    
    for (const auto& backend : backends_) {
        BackendInfo info;
        info.type = backend->getType();
        info.name = backend->getName();
        info.status = backend->getStatus();
        info.supports_input = backend->supportsInput();
        info.supports_output = backend->supportsOutput();
        info.messages_sent = backend->getMessagesSent();
        info.messages_received = backend->getMessagesReceived();
        info_list.push_back(info);
    }
    
    return info_list;
}

bool UnifiedMidiManager::enableBackend(BackendType type) {
    auto* backend = getBackend(type);
    if (!backend) {
        std::cout << "Backend type " << (int)type << " not found" << std::endl;
        return false;
    }
    
    return backend->initialize();
}

bool UnifiedMidiManager::disableBackend(BackendType type) {
    auto* backend = getBackend(type);
    if (!backend) return false;
    
    backend->cleanup();
    return true;
}

bool UnifiedMidiManager::isBackendEnabled(BackendType type) const {
    auto* backend = getBackend(type);
    return backend && backend->getStatus() == ConnectionStatus::CONNECTED;
}

// MIDI Output methods - send to all connected backends
void UnifiedMidiManager::sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    uint8_t status = 0x90 | ((channel - 1) & 0x0F); // Convert to 0-15 range
    
    // Log to global queue - safe on all platforms!
    char buf[60];
    snprintf(buf, sizeof(buf), "NoteOn ch%d %d %d", channel, note, velocity);
    UI::MidiLogQueue::getInstance().logOutput(buf);
    
    for (auto& backend : backends_) {
        if (backend->getStatus() == ConnectionStatus::CONNECTED && backend->supportsOutput()) {
            backend->sendMessage(status, note & 0x7F, velocity & 0x7F);
        }
    }
}

void UnifiedMidiManager::sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    uint8_t status = 0x80 | ((channel - 1) & 0x0F);
    
    // Log to global queue - safe on all platforms!
    char buf[60];
    snprintf(buf, sizeof(buf), "NoteOff ch%d %d %d", channel, note, velocity);
    UI::MidiLogQueue::getInstance().logOutput(buf);
    
    for (auto& backend : backends_) {
        if (backend->getStatus() == ConnectionStatus::CONNECTED && backend->supportsOutput()) {
            backend->sendMessage(status, note & 0x7F, velocity & 0x7F);
        }
    }
}

void UnifiedMidiManager::sendControlChange(uint8_t channel, uint8_t cc, uint8_t value) {
    uint8_t status = 0xB0 | ((channel - 1) & 0x0F);
    
    // Log to global queue - safe on all platforms!
    char buf[60];
    snprintf(buf, sizeof(buf), "CC ch%d %d %d", channel, cc, value);
    UI::MidiLogQueue::getInstance().logOutput(buf);
    
    for (auto& backend : backends_) {
        if (backend->getStatus() == ConnectionStatus::CONNECTED && backend->supportsOutput()) {
            backend->sendMessage(status, cc & 0x7F, value & 0x7F);
        }
    }
}

void UnifiedMidiManager::sendProgramChange(uint8_t channel, uint8_t program) {
    uint8_t status = 0xC0 | ((channel - 1) & 0x0F);
#if !defined(ESP32_BUILD)
    if (midi_monitor_) {
        char buf[64];
        snprintf(buf, sizeof(buf), "ProgramChange ch%d %d", channel, program);
        midi_monitor_->logOutput(buf);
    }
#endif
    for (auto& backend : backends_) {
        if (backend->getStatus() == ConnectionStatus::CONNECTED && backend->supportsOutput()) {
            backend->sendMessage(status, program & 0x7F, 0);
        }
    }
}

void UnifiedMidiManager::sendPitchBend(uint8_t channel, uint16_t value) {
    uint8_t status = 0xE0 | ((channel - 1) & 0x0F);
    uint8_t lsb = value & 0x7F;
    uint8_t msb = (value >> 7) & 0x7F;
#if !defined(ESP32_BUILD)
    if (midi_monitor_) {
        char buf[64];
        snprintf(buf, sizeof(buf), "PitchBend ch%d %d", channel, value);
        midi_monitor_->logOutput(buf);
    }
#endif
    for (auto& backend : backends_) {
        if (backend->getStatus() == ConnectionStatus::CONNECTED && backend->supportsOutput()) {
            backend->sendMessage(status, lsb, msb);
        }
    }
}

void UnifiedMidiManager::sendClockPulse() {
#if !defined(ESP32_BUILD)
    if (midi_monitor_) midi_monitor_->logOutput("ClockPulse");
#endif
    for (auto& backend : backends_) {
        if (backend->getStatus() == ConnectionStatus::CONNECTED && backend->supportsOutput()) {
            backend->sendMessage(0xF8); // MIDI Clock
        }
    }
}

void UnifiedMidiManager::sendStart() {
#if !defined(ESP32_BUILD)
    if (midi_monitor_) midi_monitor_->logOutput("Start");
#endif
    for (auto& backend : backends_) {
        if (backend->getStatus() == ConnectionStatus::CONNECTED && backend->supportsOutput()) {
            backend->sendMessage(0xFA); // MIDI Start
        }
    }
}

void UnifiedMidiManager::sendStop() {
#if !defined(ESP32_BUILD)
    if (midi_monitor_) midi_monitor_->logOutput("Stop");
#endif
    for (auto& backend : backends_) {
        if (backend->getStatus() == ConnectionStatus::CONNECTED && backend->supportsOutput()) {
            backend->sendMessage(0xFC); // MIDI Stop
        }
    }
}

void UnifiedMidiManager::sendContinue() {
#if !defined(ESP32_BUILD)
    if (midi_monitor_) midi_monitor_->logOutput("Continue");
#endif
    for (auto& backend : backends_) {
        if (backend->getStatus() == ConnectionStatus::CONNECTED && backend->supportsOutput()) {
            backend->sendMessage(0xFB); // MIDI Continue
        }
    }
}

void UnifiedMidiManager::sendSystemReset() {
#if !defined(ESP32_BUILD)
    if (midi_monitor_) midi_monitor_->logOutput("SystemReset");
#endif
    for (auto& backend : backends_) {
        if (backend->getStatus() == ConnectionStatus::CONNECTED && backend->supportsOutput()) {
            backend->sendMessage(0xFF); // System Reset
        }
    }
}

void UnifiedMidiManager::sendActiveSensing() {
#if !defined(ESP32_BUILD)
    if (midi_monitor_) midi_monitor_->logOutput("ActiveSensing");
#endif
    for (auto& backend : backends_) {
        if (backend->getStatus() == ConnectionStatus::CONNECTED && backend->supportsOutput()) {
            backend->sendMessage(0xFE); // Active Sensing
        }
    }
}

void UnifiedMidiManager::setMidiMessageCallback(MidiMessageCallback callback) {
    message_callback_ = callback;
}

UnifiedMidiManager::ConnectionStatus UnifiedMidiManager::getOverallStatus() const {
    bool any_connected = false;
    bool any_error = false;
    
    for (const auto& backend : backends_) {
        auto status = backend->getStatus();
        if (status == ConnectionStatus::CONNECTED) {
            any_connected = true;
        } else if (status == ConnectionStatus::ERROR) {
            any_error = true;
        }
    }
    
    if (any_connected) return ConnectionStatus::CONNECTED;
    if (any_error) return ConnectionStatus::ERROR;
    return ConnectionStatus::DISCONNECTED;
}

uint32_t UnifiedMidiManager::getTotalMessagesSent() const {
    uint32_t total = 0;
    for (const auto& backend : backends_) {
        total += backend->getMessagesSent();
    }
    return total;
}

uint32_t UnifiedMidiManager::getTotalMessagesReceived() const {
    uint32_t total = 0;
    for (const auto& backend : backends_) {
        total += backend->getMessagesReceived();
    }
    return total;
}

void UnifiedMidiManager::update() {
    for (auto& backend : backends_) {
        backend->update();
    }
}

UnifiedMidiManager::MidiBackend* UnifiedMidiManager::getBackend(BackendType type) const {
    auto it = std::find_if(backends_.begin(), backends_.end(),
        [type](const std::unique_ptr<MidiBackend>& backend) {
            return backend->getType() == type;
        });
    
    return (it != backends_.end()) ? it->get() : nullptr;
}
