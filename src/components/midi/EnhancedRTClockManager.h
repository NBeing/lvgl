#pragma once

#include "components/midi/ThreadedMidiClockManager.h"
#include "components/midi/MidiEvents.h"
#include "components/threading/ThreadingAbstraction.h"
#include "hardware/MidiHandler.h"
#include <atomic>
#include <memory>
#include <vector>
#include <chrono>
#include <queue>

namespace MIDI {

/**
 * @brief Enhanced RT Clock Thread with comprehensive event scheduling
 * 
 * Transforms the underutilized RT clock thread into a powerful event scheduler
 * that can handle precise MIDI output, sequencer events, and timing-critical operations.
 */
class EnhancedRTClockManager {
public:
    // Scheduled event types that can be processed in RT thread
    struct RTScheduledEvent {
        enum Type {
            MIDI_NOTE_ON,
            MIDI_NOTE_OFF,
            MIDI_CC,
            CLOCK_TICK,
            TRANSPORT_CHANGE,
            SEQUENCER_STEP,
            CUSTOM_CALLBACK
        };
        
        Type type;
        uint64_t execute_time_us;  // Absolute timestamp
        
        // MIDI event data
        uint8_t channel;
        uint8_t data1;
        uint8_t data2;
        
        // Sequencer event data
        int step_number;
        int track_id;
        
        // Custom callback (RT-safe function pointer)
        void (*rt_callback)(void* user_data);
        void* user_data;
        
        RTScheduledEvent(Type t, uint64_t time_us) 
            : type(t), execute_time_us(time_us), channel(0), data1(0), data2(0), 
              step_number(0), track_id(0), rt_callback(nullptr), user_data(nullptr) {}
        
        // Default constructor for array initialization
        RTScheduledEvent() 
            : type(MIDI_NOTE_ON), execute_time_us(0), channel(0), data1(0), data2(0), 
              step_number(0), track_id(0), rt_callback(nullptr), user_data(nullptr) {}
        
        // Priority queue ordering (earliest events first)
        bool operator>(const RTScheduledEvent& other) const {
            return execute_time_us > other.execute_time_us;
        }
    };
    
    static EnhancedRTClockManager& getInstance();
    
    // Lifecycle (enhanced from original)
    bool start();
    void stop();
    bool isRunning() const { return running_.load(); }
    
    // Event scheduling (called from UI thread - lock-free)
    void scheduleNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, 
                       std::chrono::steady_clock::time_point when);
    void scheduleNoteOff(uint8_t channel, uint8_t note, 
                        std::chrono::steady_clock::time_point when);
    void scheduleCC(uint8_t channel, uint8_t cc, uint8_t value,
                   std::chrono::steady_clock::time_point when);
    void scheduleSequencerStep(int track_id, int step_number,
                              std::chrono::steady_clock::time_point when);
    void scheduleCustomCallback(void (*callback)(void*), void* user_data,
                               std::chrono::steady_clock::time_point when);
    
    // MIDI input processing (called from MIDI input callbacks - RT-safe!)
    void processMidiInputRT(uint8_t status, uint8_t data1, uint8_t data2);
    void processMidiClockInputRT();
    void processMidiStartInputRT();
    void processMidiStopInputRT();
    void processMidiContinueInputRT();
    
    // Clock functionality (preserved from original)
    void addClockObserver(TypedObserver<ClockEvent>* observer);
    void removeClockObserver(TypedObserver<ClockEvent>* observer);
    void processEvents(); // Called from UI thread
    
    // Transport control
    void play();
    void pause();
    void stop_transport();
    void continue_playback();
    
    // Settings
    void setBPM(float bpm);
    void setClockMode(MidiClockManager::ClockMode mode);
    
    // Status
    float getBPM() const { return cached_bpm_.load(); }
    int getCurrentTick() const { return rt_tick_count_.load(); }
    bool isClockRunning() const { return cached_clock_running_.load(); }
    
    // Enhanced statistics
    struct RTStats {
        uint64_t events_scheduled = 0;
        uint64_t events_executed = 0;
        uint64_t events_dropped = 0;  // When queue is full
        float average_latency_us = 0.0f;
        float max_latency_us = 0.0f;
        uint32_t queue_size = 0;
        uint32_t max_queue_size = 0;
        
        void reset() {
            events_scheduled = 0;
            events_executed = 0;
            events_dropped = 0;
            average_latency_us = 0.0f;
            max_latency_us = 0.0f;
            queue_size = 0;
            max_queue_size = 0;
        }
    };
    
    const RTStats& getRTStats() const { return rt_stats_; }
    void resetRTStats() { rt_stats_.reset(); }
    
    // MIDI hardware integration
    void setMidiHandler(std::shared_ptr<MidiHandler> handler) { 
        midi_handler_ = handler; 
    }

private:
    EnhancedRTClockManager() = default;
    ~EnhancedRTClockManager() { stop(); }
    EnhancedRTClockManager(const EnhancedRTClockManager&) = delete;
    EnhancedRTClockManager& operator=(const EnhancedRTClockManager&) = delete;
    
    // Thread management
    std::atomic<bool> running_{false};
    std::unique_ptr<Threading::ThreadHandle> rt_thread_;
    
    // Event scheduling (lock-free)
    static constexpr size_t MAX_RT_EVENTS = 1024;
    RTScheduledEvent rt_events_[MAX_RT_EVENTS];
    std::atomic<size_t> write_index_{0};
    std::atomic<size_t> read_index_{0};
    std::atomic<size_t> event_count_{0};
    
    // MIDI hardware
    std::shared_ptr<MidiHandler> midi_handler_;
    
    // RT input buffers (lock-free)
    static constexpr size_t MAX_INPUT_EVENTS = 256;
    struct RTMidiInputEvent {
        uint8_t status, data1, data2;
        uint64_t timestamp_us;
        enum Type { NOTE_INPUT, CC_INPUT, CLOCK_INPUT, TRANSPORT_INPUT } type;
    };
    RTMidiInputEvent input_events_[MAX_INPUT_EVENTS];
    std::atomic<size_t> input_write_index_{0};
    std::atomic<size_t> input_read_index_{0};
    std::atomic<size_t> input_count_{0};
    
    // Clock management (from original ThreadedMidiClockManager)
    ThreadSafeSubject<ClockEvent> clock_subject_;
    std::atomic<float> cached_bpm_{120.0f};
    std::atomic<int> cached_ppqn_{24};
    std::atomic<bool> cached_clock_running_{false};
    std::atomic<MidiClockManager::ClockMode> cached_clock_mode_{MidiClockManager::ClockMode::INTERNAL};
    std::atomic<int> rt_tick_count_{0};
    
    // RT thread timing
    std::chrono::steady_clock::time_point last_tick_time_;
    std::chrono::steady_clock::time_point thread_start_time_;
    bool first_tick_ = true;
    
    // Statistics
    RTStats rt_stats_;
    
    // RT thread main loop (enhanced)
    void enhancedRTThreadLoop();
    
    // Event processing (RT-safe)
    void processScheduledEventsRT();
    void processMidiInputEventsRT();  // NEW: Process MIDI input in RT
    void processParameterEventsRT();  // NEW: Process parameter changes in RT
    void processClockGenerationRT();
    void executeMidiEventRT(const RTScheduledEvent& event);
    void executeSequencerEventRT(const RTScheduledEvent& event);
    void executeCustomCallbackRT(const RTScheduledEvent& event);
    
    // MIDI input processing (RT-safe)
    void addMidiInputToBuffer(uint8_t status, uint8_t data1, uint8_t data2, 
                             RTMidiInputEvent::Type type);
    bool getNextMidiInputEvent(RTMidiInputEvent& event);
    void handleMidiInputInRT(const RTMidiInputEvent& event);
    
    // Queue management (lock-free)
    bool addEventToQueue(const RTScheduledEvent& event);
    bool getNextDueEvent(RTScheduledEvent& event, uint64_t current_time_us);
    
    // Timing utilities
    uint64_t getCurrentTimeUs() const;
    uint32_t getTickIntervalMicros() const;
    void updateRTStats(float latency_us);
    
    // Clock functionality (from original)
    void generateClockTick();
    void enqueueClockEvent(ClockEvent::Type type, int tick_count = 0);
    void updateCachedValues();
};

} // namespace MIDI
