#include "MidiClockManager.h"
#include "UnifiedMidiManager.h"
#include <iostream>
#include <algorithm>

MidiClockManager& MidiClockManager::getInstance() {
    static MidiClockManager instance;
    return instance;
}

void MidiClockManager::play() {
    TransportState old_state = transport_state_;
    
    if (transport_state_ == TransportState::STOPPED) {
        // Starting from stop - reset tick counter
        current_tick_ = 0;
        first_tick_ = true;
        
        if (settings_.send_transport) {
            sendMidiStart();
        }
    } else if (transport_state_ == TransportState::PAUSED) {
        // Continuing from pause
        if (settings_.send_transport) {
            sendMidiContinue();
        }
    }
    
    transport_state_ = TransportState::PLAYING;
    last_tick_time_ = std::chrono::steady_clock::now();
    
    std::cout << "MidiClockManager: Transport PLAY (BPM: " << settings_.bpm << ")" << std::endl;
    notifyTransportChanged(old_state, transport_state_);
}

void MidiClockManager::pause() {
    if (transport_state_ != TransportState::PLAYING) return;
    
    TransportState old_state = transport_state_;
    transport_state_ = TransportState::PAUSED;
    
    // Note: MIDI standard doesn't have a pause message, so we send stop
    if (settings_.send_transport) {
        sendMidiStop();
    }
    
    std::cout << "MidiClockManager: Transport PAUSE" << std::endl;
    notifyTransportChanged(old_state, transport_state_);
}

void MidiClockManager::stop() {
    if (transport_state_ == TransportState::STOPPED) return;
    
    TransportState old_state = transport_state_;
    transport_state_ = TransportState::STOPPED;
    current_tick_ = 0;
    
    if (settings_.send_transport) {
        sendMidiStop();
    }
    
    std::cout << "MidiClockManager: Transport STOP" << std::endl;
    notifyTransportChanged(old_state, transport_state_);
}

void MidiClockManager::continue_playback() {
    if (transport_state_ == TransportState::PAUSED) {
        play(); // Reuse play logic for continue
    }
}

void MidiClockManager::setClockSettings(const ClockSettings& settings) {
    ClockSettings old_settings = settings_;
    settings_ = settings;
    
    std::cout << "MidiClockManager: Settings updated - BPM: " << settings_.bpm 
              << ", PPQN: " << settings_.ppqn 
              << ", Mode: " << (int)settings_.mode << std::endl;
    
    if (old_settings.bpm != settings_.bpm) {
        notifyBPMChanged();
    }
}

void MidiClockManager::setBPM(float bpm) {
    bpm = std::clamp(bpm, 60.0f, 200.0f); // Reasonable BPM range
    if (settings_.bpm != bpm) {
        settings_.bpm = bpm;
        std::cout << "MidiClockManager: BPM changed to " << bpm << std::endl;
        notifyBPMChanged();
    }
}

void MidiClockManager::setPPQN(int ppqn) {
    // MIDI standard supports 24 PPQN, but some devices use different values
    ppqn = std::clamp(ppqn, 12, 96);
    if (settings_.ppqn != ppqn) {
        settings_.ppqn = ppqn;
        std::cout << "MidiClockManager: PPQN changed to " << ppqn << std::endl;
    }
}

void MidiClockManager::setClockMode(ClockMode mode) {
    if (settings_.mode != mode) {
        settings_.mode = mode;
        std::cout << "MidiClockManager: Clock mode changed to " << (int)mode << std::endl;
        
        if (mode == ClockMode::EXTERNAL) {
            // Reset external sync state
            external_tick_count_ = 0;
            detected_bpm_ = 120.0f;
        }
    }
}

void MidiClockManager::update() {
    if (transport_state_ != TransportState::PLAYING) return;
    
    if (settings_.mode == ClockMode::INTERNAL) {
        auto now = std::chrono::steady_clock::now();
        
        if (first_tick_) {
            first_tick_ = false;
            last_tick_time_ = now;
            return;
        }
        
        double interval_ms = getTickIntervalMs();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - last_tick_time_);
        
        if (elapsed.count() >= (interval_ms * 1000)) {
            // Time for next tick
            current_tick_++;
            last_tick_time_ = now;
            
            if (settings_.send_clock) {
                sendMidiClock();
            }
            
            notifyClockTick();
        }
    }
    // External mode timing is handled by incoming MIDI messages
}

double MidiClockManager::getTickIntervalMs() const {
    // Calculate interval between MIDI clock ticks
    // 60 seconds/minute * 1000 ms/second / (BPM * PPQN) = ms per tick
    return (60.0 * 1000.0) / (settings_.bpm * settings_.ppqn);
}

// MIDI message handlers (called by MidiHandler)
void MidiClockManager::handleMidiClockMessage() {
    if (!settings_.receive_clock || settings_.mode != ClockMode::EXTERNAL) return;
    
    auto now = std::chrono::steady_clock::now();
    
    if (external_tick_count_ > 0) {
        // Calculate BPM from incoming clock
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - last_external_clock_);
        double tick_interval_ms = elapsed.count() / 1000.0;
        
        if (tick_interval_ms > 0) {
            // BPM = 60000 / (tick_interval_ms * PPQN)
            float new_bpm = 60000.0f / (tick_interval_ms * settings_.ppqn);
            
            // Smooth the BPM detection
            detected_bpm_ = (detected_bpm_ * 0.9f) + (new_bpm * 0.1f);
            
            if (abs(detected_bpm_ - settings_.bpm) > 1.0f) {
                setBPM(detected_bpm_);
            }
        }
    }
    
    last_external_clock_ = now;
    external_tick_count_++;
    current_tick_++;
    
    notifyClockTick();
}

void MidiClockManager::handleMidiStartMessage() {
    if (!settings_.receive_transport) return;
    
    std::cout << "MidiClockManager: Received MIDI Start" << std::endl;
    current_tick_ = 0;
    external_tick_count_ = 0;
    play();
}

void MidiClockManager::handleMidiStopMessage() {
    if (!settings_.receive_transport) return;
    
    std::cout << "MidiClockManager: Received MIDI Stop" << std::endl;
    stop();
}

void MidiClockManager::handleMidiContinueMessage() {
    if (!settings_.receive_transport) return;
    
    std::cout << "MidiClockManager: Received MIDI Continue" << std::endl;
    continue_playback();
}

// Private helper methods
void MidiClockManager::sendMidiClock() {
    if (settings_.send_clock) {
        UnifiedMidiManager::getInstance().sendClockPulse();
    }
}

void MidiClockManager::sendMidiStart() {
    if (settings_.send_transport) {
        UnifiedMidiManager::getInstance().sendStart();
        std::cout << "MidiClockManager: Sending MIDI Start" << std::endl;
    }
}

void MidiClockManager::sendMidiStop() {
    if (settings_.send_transport) {
        UnifiedMidiManager::getInstance().sendStop();
        std::cout << "MidiClockManager: Sending MIDI Stop" << std::endl;
    }
}

void MidiClockManager::sendMidiContinue() {
    if (settings_.send_transport) {
        UnifiedMidiManager::getInstance().sendContinue();
        std::cout << "MidiClockManager: Sending MIDI Continue" << std::endl;
    }
}

void MidiClockManager::notifyTransportChanged(TransportState old_state, TransportState new_state) {
    if (transport_callback_) {
        transport_callback_(old_state, new_state);
    }
}

void MidiClockManager::notifyClockTick() {
    if (clock_callback_) {
        clock_callback_(current_tick_);
    }
}

void MidiClockManager::notifyBPMChanged() {
    if (bpm_callback_) {
        bpm_callback_(settings_.bpm);
    }
}
