#pragma once

#include "ParameterChangeEvent.h"
#include "ParameterRegistry.h"
#include "ParameterObserver.h"
#include "components/threading/LockFreeQueue.h"
#include "components/threading/ThreadSafeSubject.h"
#include <vector>
#include <unordered_map>
#include <atomic>
#include <memory>

namespace Parameters {

/**
 * @brief Central parameter manager and event dispatcher
 * 
 * This is the heart of the unified parameter system. It receives parameter
 * change events from any source (touch, MIDI, automation) and routes them
 * to the appropriate processors and observers based on parameter metadata.
 */
class ParameterManager {
public:
    static ParameterManager& getInstance();
    
    // Initialization
    void initialize();
    void shutdown();
    
    // Observer management
    void addObserver(std::shared_ptr<ParameterObserver> observer);
    void removeObserver(std::shared_ptr<ParameterObserver> observer);
    
    // Parameter change processing (thread-safe)
    void processParameterChange(const ParameterChangeEvent& event);
    
    // Direct parameter setting (normalized values)
    void setParameter(ParameterID id, float normalized_value, ParameterSource source = ParameterSource::INTERNAL);
    void setParameterFromReal(ParameterID id, float real_value, ParameterSource source = ParameterSource::INTERNAL);
    
    // Parameter value queries (thread-safe)
    float getParameterNormalized(ParameterID id) const;
    float getParameterReal(ParameterID id) const;
    std::string getParameterDisplayValue(ParameterID id) const;
    
    // MIDI learn system
    void startMidiLearn(ParameterID id);
    void stopMidiLearn();
    bool isMidiLearning() const { return midi_learn_parameter_id_.load() != 0; }
    ParameterID getMidiLearnParameter() const { return midi_learn_parameter_id_.load(); }
    
    // MIDI mapping management
    void assignMidiCC(ParameterID parameter_id, uint8_t channel, uint8_t cc);
    void removeMidiCC(uint8_t channel, uint8_t cc);
    ParameterID getMidiMapping(uint8_t channel, uint8_t cc) const;
    
    // Automation support
    void recordParameterChange(const ParameterChangeEvent& event);
    void playbackAutomation(const std::vector<ParameterChangeEvent>& events);
    
    // Preset support
    std::unordered_map<ParameterID, float> captureCurrentState() const;
    void loadPresetState(const std::unordered_map<ParameterID, float>& state);
    
    // RT thread processing (called from RT thread)
    void processRTEvents();
    
    // UI thread processing (called from UI thread)  
    void processUIEvents();
    
    // Statistics
    struct Statistics {
        std::atomic<uint64_t> events_processed{0};
        std::atomic<uint64_t> rt_events_processed{0};
        std::atomic<uint64_t> ui_events_processed{0};
        std::atomic<uint64_t> events_dropped{0};
        std::atomic<uint32_t> current_rt_queue_size{0};
        std::atomic<uint32_t> current_ui_queue_size{0};
        std::atomic<uint32_t> max_rt_queue_size{0};
        std::atomic<uint32_t> max_ui_queue_size{0};
    };
    
    const Statistics& getStatistics() const { return stats_; }
    
private:
    ParameterManager() = default;
    
    // Event routing
    void routeParameterEvent(const ParameterChangeEvent& event);
    void notifyRTObservers(const ParameterChangeEvent& event);
    void notifyUIObservers(const ParameterChangeEvent& event);
    
    // MIDI processing
    void handleMidiCCEvent(const ParameterChangeEvent& event);
    
    // Observer management
    std::vector<std::shared_ptr<RTParameterObserver>> rt_observers_;
    std::vector<std::shared_ptr<UIParameterObserver>> ui_observers_;
    
    // Event queues
    static constexpr size_t MAX_RT_EVENTS = 1024;
    static constexpr size_t MAX_UI_EVENTS = 2048;
    
    LockFreeQueue<ParameterChangeEvent, MAX_RT_EVENTS> rt_event_queue_;
    ThreadSafeSubject<ParameterChangeEvent> ui_event_subject_;
    
    // Current parameter values (atomic for thread-safety)
    mutable std::unordered_map<ParameterID, std::atomic<float>> parameter_values_;
    
    // MIDI learn system
    std::atomic<ParameterID> midi_learn_parameter_id_{0};
    
    // MIDI CC mappings (channel << 8 | cc) -> parameter_id
    std::unordered_map<uint16_t, ParameterID> midi_cc_mappings_;
    
    // Statistics
    Statistics stats_;
    
    // Initialization state
    std::atomic<bool> initialized_{false};
};

} // namespace Parameters
