#include "ParameterManager.h"
#include <iostream>
#include <algorithm>

namespace Parameters {

ParameterManager& ParameterManager::getInstance() {
    static ParameterManager instance;
    return instance;
}

void ParameterManager::initialize() {
    if (initialized_.load()) return;
    
    std::cout << "[ParameterManager] Initializing unified parameter system..." << std::endl;
    
    // Initialize parameter values to defaults
    auto& registry = ParameterRegistry::getInstance();
    auto all_params = registry.getAllParameterIDs();
    
    for (ParameterID id : all_params) {
        const auto* info = registry.getParameterInfo(id);
        if (info) {
            float default_normalized = registry.normalizeValue(id, info->default_value);
            parameter_values_[id].store(default_normalized);
        }
    }
    
    initialized_.store(true);
    std::cout << "[ParameterManager] ⚡ Parameter system initialized with " 
              << all_params.size() << " parameters" << std::endl;
}

void ParameterManager::shutdown() {
    if (!initialized_.load()) return;
    
    std::cout << "[ParameterManager] Shutting down parameter system..." << std::endl;
    
    rt_observers_.clear();
    ui_observers_.clear();
    midi_cc_mappings_.clear();
    parameter_values_.clear();
    
    initialized_.store(false);
    std::cout << "[ParameterManager] Parameter system shutdown complete" << std::endl;
}

void ParameterManager::addObserver(std::shared_ptr<ParameterObserver> observer) {
    if (!observer) return;
    
    if (observer->isRTSafe()) {
        // Cast using static_cast since we know it's RT-safe
        auto rt_observer = std::static_pointer_cast<RTParameterObserver>(observer);
        rt_observers_.push_back(rt_observer);
        std::cout << "[ParameterManager] Added RT parameter observer" << std::endl;
    } else {
        // Cast using static_cast since we know it's UI observer
        auto ui_observer = std::static_pointer_cast<UIParameterObserver>(observer);
        ui_observers_.push_back(ui_observer);
        ui_event_subject_.addObserver(ui_observer.get());
        std::cout << "[ParameterManager] Added UI parameter observer" << std::endl;
    }
}

void ParameterManager::removeObserver(std::shared_ptr<ParameterObserver> observer) {
    if (!observer) return;
    
    if (observer->isRTSafe()) {
        auto rt_observer = std::static_pointer_cast<RTParameterObserver>(observer);
        rt_observers_.erase(
            std::remove(rt_observers_.begin(), rt_observers_.end(), rt_observer),
            rt_observers_.end());
    } else {
        auto ui_observer = std::static_pointer_cast<UIParameterObserver>(observer);
        ui_observers_.erase(
            std::remove(ui_observers_.begin(), ui_observers_.end(), ui_observer),
            ui_observers_.end());
        ui_event_subject_.removeObserver(ui_observer.get());
    }
}

void ParameterManager::processParameterChange(const ParameterChangeEvent& event) {
    if (!initialized_.load()) return;
    
    // Validate parameter exists
    auto& registry = ParameterRegistry::getInstance();
    if (!registry.hasParameter(event.parameter_id)) {
        std::cerr << "[ParameterManager] Unknown parameter ID: " << event.parameter_id << std::endl;
        return;
    }
    
    // Handle MIDI learn
    if (event.source == ParameterSource::MIDI_CC && isMidiLearning()) {
        ParameterID learn_param = midi_learn_parameter_id_.load();
        if (learn_param != 0) {
            assignMidiCC(learn_param, event.midi_channel, event.midi_cc);
            stopMidiLearn();
            std::cout << "[ParameterManager] 🎹 MIDI learned: CC" << (int)event.midi_cc 
                      << " → Parameter " << learn_param << std::endl;
        }
    }
    
    // Update parameter value
    parameter_values_[event.parameter_id].store(event.normalized_value);
    
    // Route to appropriate processors
    routeParameterEvent(event);
    
    // Update statistics
    stats_.events_processed.fetch_add(1);
}

void ParameterManager::routeParameterEvent(const ParameterChangeEvent& event) {
    auto& registry = ParameterRegistry::getInstance();
    const auto* info = registry.getParameterInfo(event.parameter_id);
    if (!info) return;
    
    // Route to RT thread if affects audio
    if (info->affects_audio) {
        if (rt_event_queue_.enqueue(event)) {
            stats_.current_rt_queue_size.store(rt_event_queue_.size());
            stats_.max_rt_queue_size.store(
                std::max(stats_.max_rt_queue_size.load(), static_cast<uint32_t>(rt_event_queue_.size())));
        } else {
            stats_.events_dropped.fetch_add(1);
            std::cerr << "[ParameterManager] ❌ RT event queue full!" << std::endl;
        }
    }
    
    // Route to UI thread if affects UI
    if (info->affects_ui) {
        ui_event_subject_.enqueueEvent(event);
        stats_.current_ui_queue_size.store(ui_event_subject_.getQueueSize());
        stats_.max_ui_queue_size.store(
            std::max(stats_.max_ui_queue_size.load(), static_cast<uint32_t>(ui_event_subject_.getQueueSize())));
    }
}

void ParameterManager::setParameter(ParameterID id, float normalized_value, ParameterSource source) {
    // Clamp to valid range
    normalized_value = std::max(0.0f, std::min(1.0f, normalized_value));
    
    ParameterChangeEvent event(id, normalized_value, source);
    processParameterChange(event);
}

void ParameterManager::setParameterFromReal(ParameterID id, float real_value, ParameterSource source) {
    auto& registry = ParameterRegistry::getInstance();
    float normalized = registry.normalizeValue(id, real_value);
    setParameter(id, normalized, source);
}

float ParameterManager::getParameterNormalized(ParameterID id) const {
    auto it = parameter_values_.find(id);
    return (it != parameter_values_.end()) ? it->second.load() : 0.0f;
}

float ParameterManager::getParameterReal(ParameterID id) const {
    float normalized = getParameterNormalized(id);
    auto& registry = ParameterRegistry::getInstance();
    return registry.denormalizeValue(id, normalized);
}

std::string ParameterManager::getParameterDisplayValue(ParameterID id) const {
    float real_value = getParameterReal(id);
    auto& registry = ParameterRegistry::getInstance();
    return registry.formatValue(id, real_value);
}

void ParameterManager::startMidiLearn(ParameterID id) {
    midi_learn_parameter_id_.store(id);
    std::cout << "[ParameterManager] 🎹 Started MIDI learn for parameter " << id << std::endl;
}

void ParameterManager::stopMidiLearn() {
    midi_learn_parameter_id_.store(0);
    std::cout << "[ParameterManager] MIDI learn stopped" << std::endl;
}

void ParameterManager::assignMidiCC(ParameterID parameter_id, uint8_t channel, uint8_t cc) {
    uint16_t mapping_key = (static_cast<uint16_t>(channel) << 8) | cc;
    midi_cc_mappings_[mapping_key] = parameter_id;
    std::cout << "[ParameterManager] Assigned MIDI CC" << (int)cc 
              << " Ch" << (int)channel << " → Parameter " << parameter_id << std::endl;
}

void ParameterManager::removeMidiCC(uint8_t channel, uint8_t cc) {
    uint16_t mapping_key = (static_cast<uint16_t>(channel) << 8) | cc;
    midi_cc_mappings_.erase(mapping_key);
}

ParameterID ParameterManager::getMidiMapping(uint8_t channel, uint8_t cc) const {
    uint16_t mapping_key = (static_cast<uint16_t>(channel) << 8) | cc;
    auto it = midi_cc_mappings_.find(mapping_key);
    return (it != midi_cc_mappings_.end()) ? it->second : 0;
}

void ParameterManager::processRTEvents() {
    ParameterChangeEvent event;
    int events_processed = 0;
    
    // Process up to 32 events per RT cycle to prevent blocking
    while (events_processed < 32 && rt_event_queue_.dequeue(event)) {
        notifyRTObservers(event);
        events_processed++;
    }
    
    if (events_processed > 0) {
        stats_.rt_events_processed.fetch_add(events_processed);
        stats_.current_rt_queue_size.store(rt_event_queue_.size());
    }
}

void ParameterManager::processUIEvents() {
    // Process queued UI events
    ui_event_subject_.processQueuedEvents<UIParameterObserver>();
    stats_.current_ui_queue_size.store(ui_event_subject_.getQueueSize());
}

void ParameterManager::notifyRTObservers(const ParameterChangeEvent& event) {
    // Call RT observers directly (we're already in RT thread)
    for (auto& observer : rt_observers_) {
        if (observer) {
            auto interested_params = observer->getInterestedParameters();
            if (interested_params.empty() || 
                std::find(interested_params.begin(), interested_params.end(), event.parameter_id) != interested_params.end()) {
                observer->onParameterChanged(event);
            }
        }
    }
}

std::unordered_map<ParameterID, float> ParameterManager::captureCurrentState() const {
    std::unordered_map<ParameterID, float> state;
    
    for (const auto& pair : parameter_values_) {
        state[pair.first] = pair.second.load();
    }
    
    return state;
}

void ParameterManager::loadPresetState(const std::unordered_map<ParameterID, float>& state) {
    std::cout << "[ParameterManager] Loading preset with " << state.size() << " parameters..." << std::endl;
    
    for (const auto& pair : state) {
        setParameter(pair.first, pair.second, ParameterSource::PRESET_LOAD);
    }
    
    std::cout << "[ParameterManager] Preset loaded successfully" << std::endl;
}

} // namespace Parameters
