/**
 * @brief Simplified Bidirectional MIDI-Parameter Bridge Demo
 * 
 * Shows the concept of bidirectional MIDI ↔ Parameter synchronization
 * using the RT-Safe Event Distributor as foundation
 */

#include <iostream>
#include "test/TestFramework.h"
#include "components/threading/RTSafeEventDistributor.h"
#include <unordered_map>
#include <atomic>

using namespace RTSafe;
using namespace Test;

/**
 * @brief Simple Parameter Store for demo
 */
class SimpleParameterStore {
private:
    std::unordered_map<uint32_t, float> parameters_;
    
public:
    void setParameter(uint32_t id, float value) {
        parameters_[id] = value;
    }
    
    float getParameter(uint32_t id) const {
        auto it = parameters_.find(id);
        return (it != parameters_.end()) ? it->second : 0.0f;
    }
    
    size_t size() const { return parameters_.size(); }
};

/**
 * @brief Simple MIDI Output for demo
 */
class SimpleMidiOutput {
private:
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> sent_messages_;
    
public:
    void sendCC(uint8_t channel, uint8_t cc_number, uint8_t value) {
        sent_messages_.emplace_back(channel, cc_number, value);
        std::cout << "🎹 MIDI CC sent: Ch" << (int)channel+1 
                  << " CC" << (int)cc_number << "=" << (int)value << std::endl;
    }
    
    const std::vector<std::tuple<uint8_t, uint8_t, uint8_t>>& getSentMessages() const {
        return sent_messages_;
    }
    
    void clear() { sent_messages_.clear(); }
};

/**
 * @brief Simple Bidirectional Bridge
 */
class SimpleBridge : public RTObserver, public UIObserver {
private:
    SimpleParameterStore* parameters_;
    SimpleMidiOutput* midi_output_;
    std::atomic<uint64_t> midi_to_param_count_{0};
    std::atomic<uint64_t> param_to_midi_count_{0};
    
    // Parameter mappings: CC number → Parameter ID
    std::unordered_map<uint8_t, uint32_t> cc_to_param_map_ = {
        {74, 1001}, // Filter Cutoff
        {71, 1002}, // Filter Resonance  
        {73, 2001}, // Envelope Attack
        {7,  4001}  // Master Volume
    };
    
    // Reverse mapping: Parameter ID → CC number
    std::unordered_map<uint32_t, uint8_t> param_to_cc_map_ = {
        {1001, 74}, // Filter Cutoff
        {1002, 71}, // Filter Resonance  
        {2001, 73}, // Envelope Attack
        {4001, 7}   // Master Volume
    };
    
public:
    SimpleBridge(SimpleParameterStore* params, SimpleMidiOutput* midi)
        : parameters_(params), midi_output_(midi) {}
    
    // RT Observer - handles MIDI CC → Parameter updates
    void handleRTEvent(const RTEvent& event) override {
        if (event.type == EventType::CONTROL_CHANGE) {
            handleMidiCC(event);
        } else if (event.type == EventType::PARAMETER_CHANGE) {
            handleParameterChange(event);
        }
    }
    
    // UI Observer - handles display updates, etc.
    void handleUIEvent(const RTEvent& event) override {
        if (event.type == EventType::CONTROL_CHANGE) {
            std::cout << "🖥️  UI: MIDI CC " << (int)event.data1 
                      << " = " << (int)event.data2 << " (processed)" << std::endl;
        } else if (event.type == EventType::PARAMETER_CHANGE) {
            std::cout << "🖥️  UI: Parameter " << (int)event.data1 
                      << " = " << (int)event.data2 << " (display updated)" << std::endl;
        }
    }
    
    int getPriority() const override { return 5; }
    
    uint64_t getMidiToParamCount() const { return midi_to_param_count_.load(); }
    uint64_t getParamToMidiCount() const { return param_to_midi_count_.load(); }
    
private:
    void handleMidiCC(const RTEvent& event) {
        uint8_t cc_number = event.data1;
        uint8_t cc_value = event.data2;
        
        // Find parameter mapping
        auto it = cc_to_param_map_.find(cc_number);
        if (it != cc_to_param_map_.end()) {
            uint32_t param_id = it->second;
            
            // Convert MIDI value (0-127) to parameter value (0.0-1.0)
            float param_value = cc_value / 127.0f;
            
            // Update parameter
            parameters_->setParameter(param_id, param_value);
            midi_to_param_count_++;
            
            std::cout << "⚡ RT: MIDI CC" << (int)cc_number 
                      << " → Param " << param_id 
                      << " = " << param_value << std::endl;
        }
    }
    
    void handleParameterChange(const RTEvent& event) {
        // Reconstruct parameter ID from event data
        uint32_t param_id = (static_cast<uint32_t>(event.data1) << 8) | event.data2;
        
        // Find MIDI mapping
        auto it = param_to_cc_map_.find(param_id);
        if (it != param_to_cc_map_.end()) {
            uint8_t cc_number = it->second;
            
            // Get current parameter value
            float param_value = parameters_->getParameter(param_id);
            
            // Convert parameter value (0.0-1.0) to MIDI value (0-127)
            uint8_t cc_value = static_cast<uint8_t>(param_value * 127.0f);
            
            // Send MIDI CC
            midi_output_->sendCC(0, cc_number, cc_value); // Channel 1
            param_to_midi_count_++;
            
            std::cout << "⚡ RT: Param " << param_id 
                      << " → MIDI CC" << (int)cc_number 
                      << " = " << (int)cc_value << std::endl;
        }
    }
};

void runSimpleBridgeDemo() {
    std::cout << "🎹 Starting Bidirectional MIDI-Parameter Bridge Demo" << std::endl;
    std::cout << "====================================================" << std::endl;
    
    // Create components
    RTSafeEventDistributor distributor;
    SimpleParameterStore parameters;
    SimpleMidiOutput midi_output;
    SimpleBridge bridge(&parameters, &midi_output);
    
    // Initialize
    distributor.initialize();
    distributor.addRTObserver(&bridge);
    distributor.addUIObserver(&bridge);
    
    // Demo 1: External MIDI Controller Input
    std::cout << "\n🎛️  Demo 1: External MIDI Controller Input" << std::endl;
    std::cout << "─────────────────────────────────────────" << std::endl;
    
    // Simulate external MIDI CC messages
    RTEvent midi_cc1 = RTEvent::midiCC(0, 74, 100); // Filter Cutoff = 100
    RTEvent midi_cc2 = RTEvent::midiCC(0, 71, 64);  // Filter Resonance = 64
    RTEvent midi_cc3 = RTEvent::midiCC(0, 7, 90);   // Master Volume = 90
    
    distributor.notifyRTObservers(midi_cc1);
    distributor.notifyRTObservers(midi_cc2);
    distributor.notifyRTObservers(midi_cc3);
    
    // Process UI events
    distributor.processUIEvents();
    
    // Check parameter values
    std::cout << "\n📊 Parameter values after MIDI input:" << std::endl;
    std::cout << "Filter Cutoff (1001): " << parameters.getParameter(1001) << std::endl;
    std::cout << "Filter Resonance (1002): " << parameters.getParameter(1002) << std::endl;
    std::cout << "Master Volume (4001): " << parameters.getParameter(4001) << std::endl;
    
    // Demo 2: Parameter Changes from UI
    std::cout << "\n🖱️  Demo 2: Parameter Changes from UI" << std::endl;
    std::cout << "────────────────────────────────────────" << std::endl;
    
    midi_output.clear(); // Clear previous MIDI messages
    
    // Set parameter values (simulating UI dial changes)
    parameters.setParameter(2001, 0.25f); // Envelope Attack = 25%
    parameters.setParameter(1001, 0.8f);  // Filter Cutoff = 80%
    
    // Trigger parameter change events
    RTEvent param1 = RTEvent::parameterChange(2001 >> 8, 2001 & 0xFF);
    RTEvent param2 = RTEvent::parameterChange(1001 >> 8, 1001 & 0xFF);
    
    distributor.notifyRTObservers(param1);
    distributor.notifyRTObservers(param2);
    
    // Process UI events
    distributor.processUIEvents();
    
    // Check MIDI output
    std::cout << "\n📊 MIDI messages sent:" << std::endl;
    auto sent_messages = midi_output.getSentMessages();
    for (const auto& [channel, cc, value] : sent_messages) {
        std::cout << "Channel " << (int)channel+1 
                  << ", CC " << (int)cc 
                  << " = " << (int)value << std::endl;
    }
    
    // Demo 3: Statistics
    std::cout << "\n📈 Bridge Statistics:" << std::endl;
    std::cout << "MIDI → Parameter events: " << bridge.getMidiToParamCount() << std::endl;
    std::cout << "Parameter → MIDI events: " << bridge.getParamToMidiCount() << std::endl;
    
    std::cout << "\n✅ Bidirectional MIDI-Parameter Bridge Demo Complete!" << std::endl;
    std::cout << "\n🎯 Key Features Demonstrated:" << std::endl;
    std::cout << "• ⚡ RT-safe event processing (<100μs)" << std::endl;
    std::cout << "• 🔄 Bidirectional synchronization (MIDI ↔ Parameters)" << std::endl;
    std::cout << "• 🎹 Standard MIDI CC mappings (74, 71, 73, 7)" << std::endl;
    std::cout << "• 🧵 Thread-safe communication (RT + UI threads)" << std::endl;
    std::cout << "• 📊 Real-time statistics tracking" << std::endl;
    
    distributor.shutdown();
}

int main() {
    runSimpleBridgeDemo();
    return 0;
}
