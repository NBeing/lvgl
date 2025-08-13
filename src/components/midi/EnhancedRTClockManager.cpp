#include "EnhancedRTClockManager.h"
#include "components/parameter/ParameterManager.h"
#include "components/parameter/MidiParameterBridge.h"
#include <iostream>
#include <algorithm>

namespace MIDI {

EnhancedRTClockManager& EnhancedRTClockManager::getInstance() {
    static EnhancedRTClockManager instance;
    return instance;
}

bool EnhancedRTClockManager::start() {
    if (running_.load()) return true;
    
    running_.store(true);
    thread_start_time_ = std::chrono::steady_clock::now();
    
    std::cout << "[Enhanced RT Clock] Starting enhanced real-time thread..." << std::endl;
    
    rt_thread_ = Threading::TaskManager::createTask(
        "EnhancedRTClock",
        [this]() { enhancedRTThreadLoop(); },
        Threading::TaskManager::Priority::HIGH,       // High priority (was CRITICAL)
        Threading::TaskManager::Core::CORE_0,
        8192  // Larger stack for event processing
    );
    
    if (!rt_thread_) {
        running_.store(false);
        std::cerr << "[Enhanced RT Clock] Failed to create RT thread!" << std::endl;
        return false;
    }
    
    std::cout << "[Enhanced RT Clock] ⚡ Enhanced RT thread started with event scheduling" << std::endl;
    return true;
}

void EnhancedRTClockManager::stop() {
    if (!running_.load()) return;
    
    std::cout << "[Enhanced RT Clock] Stopping enhanced RT thread..." << std::endl;
    running_.store(false);
    
    if (rt_thread_) {
        // Wait for thread to finish (join equivalent)
        // Note: ThreadingAbstraction doesn't expose join, thread will cleanup automatically
        rt_thread_.reset();
    }
    
    std::cout << "[Enhanced RT Clock] Enhanced RT thread stopped" << std::endl;
}

void EnhancedRTClockManager::scheduleNoteOn(uint8_t channel, uint8_t note, uint8_t velocity,
                                           std::chrono::steady_clock::time_point when) {
    auto time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        when - thread_start_time_).count();
    
    RTScheduledEvent event(RTScheduledEvent::MIDI_NOTE_ON, time_us);
    event.channel = channel;
    event.data1 = note;
    event.data2 = velocity;
    
    if (addEventToQueue(event)) {
        rt_stats_.events_scheduled++;
        std::cout << "[Enhanced RT Clock] 📅 Scheduled Note ON Ch:" << (int)channel 
                  << " Note:" << (int)note << " at +" << time_us << "μs" << std::endl;
    } else {
        rt_stats_.events_dropped++;
        std::cout << "[Enhanced RT Clock] ❌ Event queue full - dropped Note ON" << std::endl;
    }
}

void EnhancedRTClockManager::scheduleNoteOff(uint8_t channel, uint8_t note,
                                            std::chrono::steady_clock::time_point when) {
    auto time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        when - thread_start_time_).count();
    
    RTScheduledEvent event(RTScheduledEvent::MIDI_NOTE_OFF, time_us);
    event.channel = channel;
    event.data1 = note;
    event.data2 = 0;
    
    if (addEventToQueue(event)) {
        rt_stats_.events_scheduled++;
    } else {
        rt_stats_.events_dropped++;
    }
}

void EnhancedRTClockManager::scheduleCC(uint8_t channel, uint8_t cc, uint8_t value,
                                       std::chrono::steady_clock::time_point when) {
    auto time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        when - thread_start_time_).count();
    
    RTScheduledEvent event(RTScheduledEvent::MIDI_CC, time_us);
    event.channel = channel;
    event.data1 = cc;
    event.data2 = value;
    
    if (addEventToQueue(event)) {
        rt_stats_.events_scheduled++;
    } else {
        rt_stats_.events_dropped++;
    }
}

void EnhancedRTClockManager::scheduleSequencerStep(int track_id, int step_number,
                                                   std::chrono::steady_clock::time_point when) {
    auto time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        when - thread_start_time_).count();
    
    RTScheduledEvent event(RTScheduledEvent::SEQUENCER_STEP, time_us);
    event.track_id = track_id;
    event.step_number = step_number;
    
    if (addEventToQueue(event)) {
        rt_stats_.events_scheduled++;
    } else {
        rt_stats_.events_dropped++;
    }
}

void EnhancedRTClockManager::scheduleCustomCallback(void (*callback)(void*), void* user_data,
                                                   std::chrono::steady_clock::time_point when) {
    auto time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        when - thread_start_time_).count();
    
    RTScheduledEvent event(RTScheduledEvent::CUSTOM_CALLBACK, time_us);
    event.rt_callback = callback;
    event.user_data = user_data;
    
    if (addEventToQueue(event)) {
        rt_stats_.events_scheduled++;
    } else {
        rt_stats_.events_dropped++;
    }
}

// MIDI input processing (called from MIDI callbacks - RT-safe!)
void EnhancedRTClockManager::processMidiInputRT(uint8_t status, uint8_t data1, uint8_t data2) {
    // Parse MIDI message type
    uint8_t message_type = status & 0xF0;
    uint8_t channel = status & 0x0F;
    
    RTMidiInputEvent::Type event_type;
    
    switch (message_type) {
        case 0x90: // Note On
            if (data2 > 0) {
                event_type = RTMidiInputEvent::NOTE_INPUT;
                std::cout << "[Enhanced RT] 🎹 RT Note ON Ch:" << (int)channel 
                          << " Note:" << (int)data1 << " Vel:" << (int)data2 << std::endl;
            } else {
                event_type = RTMidiInputEvent::NOTE_INPUT; // Note On with vel=0 = Note Off
            }
            break;
            
        case 0x80: // Note Off
            event_type = RTMidiInputEvent::NOTE_INPUT;
            std::cout << "[Enhanced RT]   RT Note OFF Ch:" << (int)channel 
                      << " Note:" << (int)data1 << std::endl;
            break;
            
        case 0xB0: // Control Change
            event_type = RTMidiInputEvent::CC_INPUT;
            std::cout << "[Enhanced RT] 🎛️ RT CC Ch:" << (int)channel 
                      << " CC:" << (int)data1 << " Val:" << (int)data2 << std::endl;
            break;
            
        default:
            // Unsupported message type for RT processing
            return;
    }
    
    // Add to RT input buffer for processing
    addMidiInputToBuffer(status, data1, data2, event_type);
}

void EnhancedRTClockManager::processMidiClockInputRT() {
    std::cout << "[Enhanced RT] ⏰ RT MIDI Clock received" << std::endl;
    addMidiInputToBuffer(0xF8, 0, 0, RTMidiInputEvent::CLOCK_INPUT);
}

void EnhancedRTClockManager::processMidiStartInputRT() {
    std::cout << "[Enhanced RT] ▶️ RT MIDI Start received" << std::endl;
    addMidiInputToBuffer(0xFA, 0, 0, RTMidiInputEvent::TRANSPORT_INPUT);
}

void EnhancedRTClockManager::processMidiStopInputRT() {
    std::cout << "[Enhanced RT] ⏹️ RT MIDI Stop received" << std::endl;
    addMidiInputToBuffer(0xFC, 0, 0, RTMidiInputEvent::TRANSPORT_INPUT);
}

void EnhancedRTClockManager::processMidiContinueInputRT() {
    std::cout << "[Enhanced RT] ⏯️ RT MIDI Continue received" << std::endl;
    addMidiInputToBuffer(0xFB, 0, 0, RTMidiInputEvent::TRANSPORT_INPUT);
}

// Enhanced RT thread main loop
void EnhancedRTClockManager::enhancedRTThreadLoop() {
    std::cout << "[Enhanced RT Clock] ⚡ Enhanced RT thread main loop started" << std::endl;
    
#ifdef ESP32_BUILD
    // Note: Core affinity is set by TaskManager::createTask with Core::CORE_0
    std::cout << "[Enhanced RT Clock] Thread created for Core 0 (RT core)" << std::endl;
#endif
    
    last_tick_time_ = std::chrono::steady_clock::now();
    first_tick_ = true;
    
    while (running_.load()) {
        // Update cached values periodically
        static auto last_cache_update = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_cache_update).count() > 100) {
            updateCachedValues();
            last_cache_update = now;
        }
        
        // PHASE 1: Process scheduled events (EXISTING)
        processScheduledEventsRT();
        
        // PHASE 2: Process MIDI input events (NEW!)
        processMidiInputEventsRT();
        
        // PHASE 3: Process parameter changes (NEW UNIFIED SYSTEM!)
        processParameterEventsRT();
        
        // PHASE 4: Generate clock ticks (EXISTING)
        processClockGenerationRT();
        
        // PHASE 3: Update statistics
        rt_stats_.queue_size = event_count_.load();
        rt_stats_.max_queue_size = std::max(rt_stats_.max_queue_size, rt_stats_.queue_size);
        
        // High-precision sleep for next cycle
        uint32_t interval_us = getTickIntervalMicros();
        // Use smaller interval for more responsive event processing
        uint32_t processing_interval = std::min(interval_us, static_cast<uint32_t>(100)); // Max 100μs between checks
        Threading::TaskManager::sleepMicroseconds(processing_interval);
        
#ifdef ESP32_BUILD
        // Periodically yield to prevent watchdog timeout
        static int enhanced_watchdog_counter = 0;
        if (++enhanced_watchdog_counter >= 200) { // Every ~200 iterations
            taskYIELD();
            enhanced_watchdog_counter = 0;
        }
#endif
    }
    
    std::cout << "[Enhanced RT Clock] Enhanced RT thread loop exited" << std::endl;
}

void EnhancedRTClockManager::processScheduledEventsRT() {
    auto current_time_us = getCurrentTimeUs();
    int events_processed = 0;
    
    // Process all events that are due
    while (events_processed < 10) { // Limit per cycle
        RTScheduledEvent event(RTScheduledEvent::MIDI_NOTE_ON, 0); // Initialize with defaults
        
        if (!getNextDueEvent(event, current_time_us)) {
            break; // No more events
        }
        
        auto processing_start = std::chrono::steady_clock::now();
        
        switch (event.type) {
            case RTScheduledEvent::MIDI_NOTE_ON:
            case RTScheduledEvent::MIDI_NOTE_OFF:
            case RTScheduledEvent::MIDI_CC:
                executeMidiEventRT(event);
                break;
                
            case RTScheduledEvent::SEQUENCER_STEP:
                executeSequencerEventRT(event);
                break;
                
            case RTScheduledEvent::CUSTOM_CALLBACK:
                executeCustomCallbackRT(event);
                break;
                
            default:
                break;
        }
        
        // Measure latency
        auto processing_end = std::chrono::steady_clock::now();
        auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
            processing_end - processing_start).count();
        updateRTStats(latency_us);
        
        rt_stats_.events_executed++;
        events_processed++;
    }
    
    if (events_processed > 0) {
        std::cout << "[Enhanced RT Clock] ⚡ Processed " << events_processed 
                  << " events in RT thread" << std::endl;
    }
}

void EnhancedRTClockManager::processMidiInputEventsRT() {
    RTMidiInputEvent input_event;
    int events_processed = 0;
    
    // Process all pending MIDI input events
    while (getNextMidiInputEvent(input_event) && events_processed < 20) { // Higher limit for input
        handleMidiInputInRT(input_event);
        events_processed++;
    }
    
    if (events_processed > 0) {
        std::cout << "[Enhanced RT Clock] 🎹 Processed " << events_processed 
                  << " MIDI input events in RT thread" << std::endl;
    }
}

void EnhancedRTClockManager::processClockGenerationRT() {
    // Original clock generation logic
    if (cached_clock_running_.load() && cached_clock_mode_.load() == MidiClockManager::ClockMode::INTERNAL) {
        generateClockTick();
    }
}

void EnhancedRTClockManager::processParameterEventsRT() {
    // Process parameter changes that affect audio in RT thread
    auto& param_manager = Parameters::ParameterManager::getInstance();
    param_manager.processRTEvents();
}

void EnhancedRTClockManager::executeMidiEventRT(const RTScheduledEvent& event) {
    if (!midi_handler_) return;
    
    switch (event.type) {
        case RTScheduledEvent::MIDI_NOTE_ON:
            midi_handler_->sendNoteOn(event.channel, event.data1, event.data2);
            std::cout << "[Enhanced RT Clock] 🎼 RT Note ON Ch:" << (int)event.channel 
                      << " Note:" << (int)event.data1 << std::endl;
            break;
            
        case RTScheduledEvent::MIDI_NOTE_OFF:
            midi_handler_->sendNoteOff(event.channel, event.data1, 0);
            std::cout << "[Enhanced RT Clock]   RT Note OFF Ch:" << (int)event.channel 
                      << " Note:" << (int)event.data1 << std::endl;
            break;
            
        case RTScheduledEvent::MIDI_CC:
            midi_handler_->sendControlChange(event.channel, event.data1, event.data2);
            std::cout << "[Enhanced RT Clock] 🎛️ RT CC Ch:" << (int)event.channel 
                      << " CC:" << (int)event.data1 << " Val:" << (int)event.data2 << std::endl;
            break;
            
        default:
            break;
    }
}

void EnhancedRTClockManager::executeSequencerEventRT(const RTScheduledEvent& event) {
    // This could trigger step sequencer processing directly in RT thread
    // For now, just log it
    std::cout << "[Enhanced RT Clock] 🥁 RT Sequencer Step - Track:" << event.track_id 
              << " Step:" << event.step_number << std::endl;
    
    // Could enqueue to UI thread for complex sequencer logic
    enqueueClockEvent(ClockEvent::TICK, rt_tick_count_.load());
}

void EnhancedRTClockManager::executeCustomCallbackRT(const RTScheduledEvent& event) {
    if (event.rt_callback) {
        event.rt_callback(event.user_data);
        std::cout << "[Enhanced RT Clock] 🔧 RT Custom callback executed" << std::endl;
    }
}

bool EnhancedRTClockManager::addEventToQueue(const RTScheduledEvent& event) {
    size_t current_count = event_count_.load();
    if (current_count >= MAX_RT_EVENTS - 1) {
        return false; // Queue full
    }
    
    size_t write_pos = write_index_.load();
    rt_events_[write_pos] = event;
    
    // Update write index (circular buffer)
    write_index_.store((write_pos + 1) % MAX_RT_EVENTS);
    event_count_.fetch_add(1);
    
    return true;
}

bool EnhancedRTClockManager::getNextDueEvent(RTScheduledEvent& event, uint64_t current_time_us) {
    if (event_count_.load() == 0) return false;
    
    size_t read_pos = read_index_.load();
    const RTScheduledEvent& candidate = rt_events_[read_pos];
    
    if (candidate.execute_time_us <= current_time_us) {
        event = candidate;
        read_index_.store((read_pos + 1) % MAX_RT_EVENTS);
        event_count_.fetch_sub(1);
        return true;
    }
    
    return false;
}

// MIDI input buffer management (RT-safe)
void EnhancedRTClockManager::addMidiInputToBuffer(uint8_t status, uint8_t data1, uint8_t data2, 
                                                 RTMidiInputEvent::Type type) {
    if (input_count_.load() >= MAX_INPUT_EVENTS - 1) {
        // Buffer full - this is a critical error for real-time input
        std::cout << "[Enhanced RT Clock] ❌ MIDI input buffer full!" << std::endl;
        return;
    }
    
    size_t write_pos = input_write_index_.load();
    auto& event = input_events_[write_pos];
    
    event.status = status;
    event.data1 = data1;
    event.data2 = data2;
    event.type = type;
    event.timestamp_us = getCurrentTimeUs();
    
    // Update circular buffer indices
    input_write_index_.store((write_pos + 1) % MAX_INPUT_EVENTS);
    input_count_.fetch_add(1);
}

bool EnhancedRTClockManager::getNextMidiInputEvent(RTMidiInputEvent& event) {
    if (input_count_.load() == 0) return false;
    
    size_t read_pos = input_read_index_.load();
    event = input_events_[read_pos];
    
    // Update circular buffer indices
    input_read_index_.store((read_pos + 1) % MAX_INPUT_EVENTS);
    input_count_.fetch_sub(1);
    
    return true;
}

void EnhancedRTClockManager::handleMidiInputInRT(const RTMidiInputEvent& event) {
    switch (event.type) {
        case RTMidiInputEvent::NOTE_INPUT: {
            // Process note input in RT thread
            uint8_t message_type = event.status & 0xF0;
            uint8_t channel = event.status & 0x0F;
            
            if (message_type == 0x90 && event.data2 > 0) {
                // Note On - could trigger immediate sequencer response
                std::cout << "[Enhanced RT] 🎹 Processing Note ON in RT: Ch" << (int)channel 
                          << " Note:" << (int)event.data1 << std::endl;
                          
                // Example: Trigger immediate arpeggiator or chord processing
                // processArpeggiatorInRT(channel, event.data1, event.data2);
                
            } else {
                // Note Off
                std::cout << "[Enhanced RT]   Processing Note OFF in RT: Ch" << (int)channel 
                          << " Note:" << (int)event.data1 << std::endl;
            }
            break;
        }
        
        case RTMidiInputEvent::CC_INPUT: {
            uint8_t channel = event.status & 0x0F;
            std::cout << "[Enhanced RT] 🎛️ Processing CC in RT: Ch" << (int)channel 
                      << " CC:" << (int)event.data1 << " Val:" << (int)event.data2 << std::endl;
            
            // NEW: Route MIDI CC through unified parameter system
            auto& midi_bridge = Parameters::MidiParameterBridge::getInstance();
            midi_bridge.processMidiCC(channel, event.data1, event.data2);
            break;
        }
        
        case RTMidiInputEvent::CLOCK_INPUT: {
            // Handle external MIDI clock in RT
            std::cout << "[Enhanced RT] ⏰ Processing MIDI clock in RT" << std::endl;
            
            // Could sync internal clock or trigger sequencer advance
            // syncToExternalClockInRT();
            break;
        }
        
        case RTMidiInputEvent::TRANSPORT_INPUT: {
            // Handle transport messages in RT
            if (event.status == 0xFA) { // Start
                std::cout << "[Enhanced RT] ▶️ Processing MIDI Start in RT" << std::endl;
                // Could immediately start sequencer
                // startSequencerInRT();
            } else if (event.status == 0xFC) { // Stop
                std::cout << "[Enhanced RT] ⏹️ Processing MIDI Stop in RT" << std::endl;
                // Could immediately stop sequencer
                // stopSequencerInRT();
            }
            break;
        }
    }
}

uint64_t EnhancedRTClockManager::getCurrentTimeUs() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(
        now - thread_start_time_).count();
}

void EnhancedRTClockManager::updateRTStats(float latency_us) {
    // Update running average
    static int sample_count = 0;
    if (sample_count == 0) {
        rt_stats_.average_latency_us = latency_us;
    } else {
        rt_stats_.average_latency_us = (rt_stats_.average_latency_us * sample_count + latency_us) / (sample_count + 1);
    }
    sample_count++;
    
    // Update max
    rt_stats_.max_latency_us = std::max(rt_stats_.max_latency_us, latency_us);
}

// Clock functionality (preserved from original)
void EnhancedRTClockManager::generateClockTick() {
    auto now = std::chrono::steady_clock::now();
    
    if (first_tick_) {
        first_tick_ = false;
        last_tick_time_ = now;
        return;
    }
    
    uint32_t interval_us = getTickIntervalMicros();
    auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(now - last_tick_time_).count();
    
    if (elapsed_us >= interval_us) {
        int current_tick = rt_tick_count_.fetch_add(1);
        last_tick_time_ = now;
        
        // Enqueue clock event for UI thread
        enqueueClockEvent(ClockEvent::TICK, current_tick);
    }
}

void EnhancedRTClockManager::enqueueClockEvent(ClockEvent::Type type, int tick_count) {
    ClockEvent event(type, tick_count);
    clock_subject_.enqueueEvent(event);
}

void EnhancedRTClockManager::updateCachedValues() {
    auto& mgr = MidiClockManager::getInstance();
    const auto& settings = mgr.getClockSettings();
    
    cached_bpm_.store(settings.bpm);
    cached_ppqn_.store(settings.ppqn);
    cached_clock_mode_.store(settings.mode);
    cached_clock_running_.store(mgr.isRunning());
}

uint32_t EnhancedRTClockManager::getTickIntervalMicros() const {
    float bpm = cached_bpm_.load();
    int ppqn = cached_ppqn_.load();
    
    if (bpm <= 0 || ppqn <= 0) return 20833; // Default: ~48Hz @ 120 BPM, 24 PPQN
    
    // Calculate microseconds per tick
    float beats_per_second = bpm / 60.0f;
    float ticks_per_second = beats_per_second * ppqn;
    return static_cast<uint32_t>(1000000.0f / ticks_per_second);
}

// Delegation methods for compatibility
void EnhancedRTClockManager::addClockObserver(TypedObserver<ClockEvent>* observer) {
    clock_subject_.addObserver(observer);
}

void EnhancedRTClockManager::removeClockObserver(TypedObserver<ClockEvent>* observer) {
    clock_subject_.removeObserver(observer);
}

void EnhancedRTClockManager::processEvents() {
    clock_subject_.processQueuedEvents();
}

void EnhancedRTClockManager::play() {
    MidiClockManager::getInstance().play();
}

void EnhancedRTClockManager::pause() {
    MidiClockManager::getInstance().pause();
}

void EnhancedRTClockManager::stop_transport() {
    MidiClockManager::getInstance().stop();
}

void EnhancedRTClockManager::continue_playback() {
    MidiClockManager::getInstance().continue_playback();
}

void EnhancedRTClockManager::setBPM(float bpm) {
    MidiClockManager::getInstance().setBPM(bpm);
    cached_bpm_.store(bpm);
}

void EnhancedRTClockManager::setClockMode(MidiClockManager::ClockMode mode) {
    MidiClockManager::getInstance().setClockMode(mode);
    cached_clock_mode_.store(mode);
}

} // namespace MIDI
