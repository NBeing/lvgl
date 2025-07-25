#include "ThreadedMidiClockManager.h"
#include <iostream>

namespace MIDI {

ThreadedMidiClockManager& ThreadedMidiClockManager::getInstance() {
    static ThreadedMidiClockManager instance;
    return instance;
}

bool ThreadedMidiClockManager::start() {
    if (running_.load()) return true;
    
    std::cout << "[ThreadedMidiClock] Starting real-time MIDI clock thread..." << std::endl;
    
    // Update cached values from MidiClockManager
    updateCachedValues();
    
    running_.store(true);
    
    // Create high-priority real-time thread
    clock_thread_ = Threading::TaskManager::createTask(
        "RT_MIDI_Clock",
        [this]() { clockThreadLoop(); },
        Threading::TaskManager::Priority::CRITICAL,  // Highest priority for timing
        Threading::TaskManager::Core::CORE_0,        // Dedicated core for RT
        4096  // Minimal stack for RT thread
    );
    
    if (!clock_thread_) {
        running_.store(false);
        std::cout << "[ThreadedMidiClock] Failed to create RT clock thread!" << std::endl;
        return false;
    }
    
    std::cout << "[ThreadedMidiClock] Real-time MIDI clock thread started successfully" << std::endl;
    return true;
}

void ThreadedMidiClockManager::stop() {
    if (!running_.load()) return;
    
    std::cout << "[ThreadedMidiClock] Stopping real-time MIDI clock thread..." << std::endl;
    
    running_.store(false);
    
    if (clock_thread_) {
        clock_thread_->stop();
        clock_thread_.reset();
    }
    
    std::cout << "[ThreadedMidiClock] Real-time MIDI clock thread stopped" << std::endl;
}

void ThreadedMidiClockManager::addClockObserver(TypedObserver<ClockEvent>* observer) {
    clock_subject_.addObserver(observer);
}

void ThreadedMidiClockManager::removeClockObserver(TypedObserver<ClockEvent>* observer) {
    clock_subject_.removeObserver(observer);
}

void ThreadedMidiClockManager::processEvents() {
    // Process queued events from RT thread (called from UI thread)
    clock_subject_.processQueuedEvents();
}

// Transport control (thread-safe delegation to MidiClockManager)
void ThreadedMidiClockManager::play() {
    MidiClockManager::getInstance().play();
    transport_changed_.store(true);
}

void ThreadedMidiClockManager::pause() {
    MidiClockManager::getInstance().pause();
    transport_changed_.store(true);
}

void ThreadedMidiClockManager::stop_transport() {
    MidiClockManager::getInstance().stop();
    transport_changed_.store(true);
}

void ThreadedMidiClockManager::continue_playback() {
    MidiClockManager::getInstance().continue_playback();
    transport_changed_.store(true);
}

// Settings (thread-safe with cache update)
void ThreadedMidiClockManager::setBPM(float bpm) {
    MidiClockManager::getInstance().setBPM(bpm);
    cached_bpm_.store(bpm);
}

void ThreadedMidiClockManager::setClockMode(MidiClockManager::ClockMode mode) {
    MidiClockManager::getInstance().setClockMode(mode);
    cached_clock_mode_.store(mode);
}

void ThreadedMidiClockManager::setSyncSource(MidiClockManager::SyncSource source) {
    MidiClockManager::getInstance().setSyncSource(source);
}

// Status (thread-safe)
MidiClockManager::TransportState ThreadedMidiClockManager::getTransportState() const {
    return MidiClockManager::getInstance().getTransportState();
}

float ThreadedMidiClockManager::getBPM() const {
    return cached_bpm_.load();
}

int ThreadedMidiClockManager::getCurrentTick() const {
    return rt_tick_count_.load();
}

int ThreadedMidiClockManager::getCurrentBeat() const {
    return rt_tick_count_.load() / cached_ppqn_.load();
}

bool ThreadedMidiClockManager::isClockRunning() const {
    return cached_clock_running_.load();
}

// MIDI message handling (thread-safe delegation)
void ThreadedMidiClockManager::handleMidiClockMessage(MidiClockManager::SyncSource source) {
    MidiClockManager::getInstance().handleMidiClockMessage(source);
}

void ThreadedMidiClockManager::handleMidiStartMessage(MidiClockManager::SyncSource source) {
    MidiClockManager::getInstance().handleMidiStartMessage(source);
    transport_changed_.store(true);
}

void ThreadedMidiClockManager::handleMidiStopMessage(MidiClockManager::SyncSource source) {
    MidiClockManager::getInstance().handleMidiStopMessage(source);
    transport_changed_.store(true);
}

void ThreadedMidiClockManager::handleMidiContinueMessage(MidiClockManager::SyncSource source) {
    MidiClockManager::getInstance().handleMidiContinueMessage(source);
    transport_changed_.store(true);
}

// Real-time clock thread main loop
void ThreadedMidiClockManager::clockThreadLoop() {
    std::cout << "[RT MIDI Clock] Real-time clock thread started (Core 0, Critical Priority)" << std::endl;
    
    last_tick_time_ = std::chrono::steady_clock::now();
    first_tick_ = true;
    
    while (running_.load()) {
        // Update cached values periodically (every ~100ms)
        static auto last_cache_update = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_cache_update).count() > 100) {
            updateCachedValues();
            last_cache_update = now;
        }
        
        // Check for transport state changes
        checkTransportChanges();
        
        // Generate clock tick if playing and internal mode
        if (cached_clock_running_.load() && cached_clock_mode_.load() == MidiClockManager::ClockMode::INTERNAL) {
            generateClockTick();
        }
        
        // Sleep for precise timing
        uint32_t interval_us = getTickIntervalMicros();
        Threading::TaskManager::sleepMicroseconds(interval_us);
    }
    
    std::cout << "[RT MIDI Clock] Real-time clock thread stopped" << std::endl;
}

void ThreadedMidiClockManager::generateClockTick() {
    auto now = std::chrono::steady_clock::now();
    
    if (first_tick_) {
        first_tick_ = false;
        last_tick_time_ = now;
        return;
    }
    
    // Calculate if it's time for next tick
    uint32_t interval_us = getTickIntervalMicros();
    auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(now - last_tick_time_).count();
    
    if (elapsed_us >= interval_us) {
        // Generate tick
        int current_tick = rt_tick_count_.fetch_add(1);
        last_tick_time_ = now;
        
        // Enqueue event for UI thread (lock-free)
        enqueueClockEvent(ClockEvent::TICK, current_tick);
        
        // Optional: Send MIDI clock message (through MidiClockManager)
        // This should be fast and non-blocking
        auto& mgr = MidiClockManager::getInstance();
        if (mgr.getClockSettings().send_clock) {
            // Note: This may briefly lock, but MidiClockManager should be fast
            // In a production system, we'd queue this too
            mgr.update(); // This triggers sendMidiClock() internally
        }
    }
}

void ThreadedMidiClockManager::checkTransportChanges() {
    if (transport_changed_.load()) {
        transport_changed_.store(false);
        
        auto new_state = MidiClockManager::getInstance().getTransportState();
        if (new_state != last_transport_state_) {
            // Enqueue transport change event
            ClockEvent::Type event_type;
            switch (new_state) {
                case MidiClockManager::TransportState::PLAYING:
                    event_type = (last_transport_state_ == MidiClockManager::TransportState::PAUSED) 
                                ? ClockEvent::CONTINUE : ClockEvent::START;
                    cached_clock_running_.store(true);
                    rt_tick_count_.store(0); // Reset tick count on start
                    first_tick_ = true;
                    break;
                case MidiClockManager::TransportState::STOPPED:
                    event_type = ClockEvent::STOP;
                    cached_clock_running_.store(false);
                    rt_tick_count_.store(0);
                    break;
                case MidiClockManager::TransportState::PAUSED:
                    event_type = ClockEvent::STOP; // Pause treated as stop for now
                    cached_clock_running_.store(false);
                    break;
            }
            
            enqueueClockEvent(event_type);
            last_transport_state_ = new_state;
        }
    }
}

void ThreadedMidiClockManager::enqueueClockEvent(ClockEvent::Type type, int tick_count) {
    ClockEvent event(type, tick_count);
    clock_subject_.enqueueEvent(event);
}

void ThreadedMidiClockManager::updateCachedValues() {
    auto& mgr = MidiClockManager::getInstance();
    const auto& settings = mgr.getClockSettings();
    
    cached_bpm_.store(settings.bpm);
    cached_ppqn_.store(settings.ppqn);
    cached_clock_mode_.store(settings.mode);
    cached_clock_running_.store(mgr.isRunning());
}

uint32_t ThreadedMidiClockManager::getTickIntervalMicros() const {
    // Calculate microseconds per MIDI clock tick
    // MIDI clock: 24 ticks per quarter note
    float bpm = cached_bpm_.load();
    int ppqn = cached_ppqn_.load();
    
    // 60 seconds per minute / BPM = seconds per beat
    // seconds per beat / PPQN = seconds per tick
    // * 1,000,000 = microseconds per tick
    return static_cast<uint32_t>((60.0f * 1000000.0f) / (bpm * ppqn));
}

} // namespace MIDI
