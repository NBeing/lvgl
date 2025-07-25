#pragma once

#include "MidiClockManager.h"
#include "MidiEvents.h"
#include "components/threading/ThreadingAbstraction.h"
#include <atomic>
#include <memory>
#include <vector>
#include <chrono>

namespace MIDI {

/**
 * @brief Threaded wrapper for MidiClockManager with observer pattern
 * 
 * Adds real-time thread for precise clock generation and thread-safe
 * event queuing to notify UI components without blocking RT thread.
 */
class ThreadedMidiClockManager {
public:
    static ThreadedMidiClockManager& getInstance();
    
    // Lifecycle
    bool start();
    void stop();
    bool isRunning() const { return running_.load(); }
    
    // Observer management (thread-safe)
    void addClockObserver(TypedObserver<ClockEvent>* observer);
    void removeClockObserver(TypedObserver<ClockEvent>* observer);
    
    // Event processing (call from UI thread)
    void processEvents();
    
    // Transport control (thread-safe)
    void play();
    void pause();
    void stop_transport();
    void continue_playback();
    
    // Settings (thread-safe)
    void setBPM(float bpm);
    void setClockMode(MidiClockManager::ClockMode mode);
    void setSyncSource(MidiClockManager::SyncSource source);
    
    // Status (thread-safe)
    MidiClockManager::TransportState getTransportState() const;
    float getBPM() const;
    int getCurrentTick() const;
    int getCurrentBeat() const;
    bool isClockRunning() const;
    
    // Get underlying manager for advanced features
    MidiClockManager& getClockManager() { return MidiClockManager::getInstance(); }
    
    // MIDI message handling (called from MIDI input thread)
    void handleMidiClockMessage(MidiClockManager::SyncSource source = MidiClockManager::SyncSource::AUTO_DETECT);
    void handleMidiStartMessage(MidiClockManager::SyncSource source = MidiClockManager::SyncSource::AUTO_DETECT);
    void handleMidiStopMessage(MidiClockManager::SyncSource source = MidiClockManager::SyncSource::AUTO_DETECT);
    void handleMidiContinueMessage(MidiClockManager::SyncSource source = MidiClockManager::SyncSource::AUTO_DETECT);

private:
    ThreadedMidiClockManager() = default;
    ~ThreadedMidiClockManager() { stop(); }
    ThreadedMidiClockManager(const ThreadedMidiClockManager&) = delete;
    ThreadedMidiClockManager& operator=(const ThreadedMidiClockManager&) = delete;
    
    // Real-time clock thread
    void clockThreadLoop();
    void generateClockTick();
    void checkTransportChanges();
    
    // Event generation (called from RT thread)
    void enqueueClockEvent(ClockEvent::Type type, int tick_count = 0);
    
    // Thread state
    std::atomic<bool> running_{false};
    std::unique_ptr<Threading::ThreadHandle> clock_thread_;
    
    // Observer management (protected by lock-free queue)
    ThreadSafeSubject<ClockEvent> clock_subject_;
    
    // Cached state for RT thread (avoid calling into MidiClockManager)
    std::atomic<float> cached_bpm_{120.0f};
    std::atomic<int> cached_ppqn_{24};
    std::atomic<bool> cached_clock_running_{false};
    std::atomic<MidiClockManager::ClockMode> cached_clock_mode_{MidiClockManager::ClockMode::INTERNAL};
    
    // RT thread timing
    std::chrono::steady_clock::time_point last_tick_time_;
    std::atomic<int> rt_tick_count_{0};
    bool first_tick_ = true;
    
    // State synchronization
    std::atomic<bool> transport_changed_{false};
    MidiClockManager::TransportState last_transport_state_ = MidiClockManager::TransportState::STOPPED;
    
    // Update cached values from MidiClockManager
    void updateCachedValues();
    
    // Calculate precise timing
    uint32_t getTickIntervalMicros() const;
};

} // namespace MIDI
