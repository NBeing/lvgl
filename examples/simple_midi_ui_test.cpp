#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

// Mock the necessary components for testing
namespace Parameters {
    using ParameterID = uint32_t;
    
    enum class ParameterSource {
        MIDI_CC,
        UI,
        AUTOMATION
    };
    
    struct ParameterChangeEvent {
        ParameterID parameter_id;
        float new_value;
        ParameterSource source;
        uint8_t midi_channel{0};
        
        ParameterChangeEvent(ParameterID id, float value, ParameterSource src)
            : parameter_id(id), new_value(value), source(src) {}
    };
    
    class ParameterManager {
    private:
        std::unordered_map<uint16_t, ParameterID> midi_mappings_;
        
    public:
        void initialize() {
            std::cout << "[ParameterManager] Initialized" << std::endl;
        }
        
        void shutdown() {
            std::cout << "[ParameterManager] Shutdown" << std::endl;
        }
        
        void assignMidiCC(ParameterID param_id, uint8_t channel, uint8_t cc) {
            uint16_t key = (channel << 8) | cc;
            midi_mappings_[key] = param_id;
            std::cout << "[ParameterManager] Mapped CC" << (int)cc 
                      << " Ch" << (int)channel << " → Param " << param_id << std::endl;
        }
        
        ParameterID getMidiMapping(uint8_t channel, uint8_t cc) const {
            uint16_t key = (channel << 8) | cc;
            auto it = midi_mappings_.find(key);
            return (it != midi_mappings_.end()) ? it->second : 0;
        }
        
        void removeMidiCC(uint8_t channel, uint8_t cc) {
            uint16_t key = (channel << 8) | cc;
            midi_mappings_.erase(key);
        }
        
        void processParameterChange(const ParameterChangeEvent& event) {
            // In real implementation, this would route to appropriate handlers
            std::cout << "[ParameterManager] Parameter " << event.parameter_id 
                      << " changed to " << event.new_value << std::endl;
        }
    };
}

// Include our MIDI UI Bridge
#include "../src/components/ui/MidiUIBridge.h"

// Mock UI Manager
namespace UI {
    class SequencerUIManager {
    private:
        int current_track_{0};
        int current_step_{0};
        int current_pattern_{0};
        
    public:
        SequencerUIManager(void*, void*, void*, std::shared_ptr<Parameters::ParameterManager>) {}
        
        void setFocus(int track, int step) {
            current_track_ = track;
            current_step_ = step;
            std::cout << "[UI] Focus set to Track " << track << ", Step " << step << std::endl;
        }
        
        void switchToPattern(int pattern) {
            current_pattern_ = pattern;
            std::cout << "[UI] Switched to Pattern " << pattern << std::endl;
        }
        
        int getCurrentTrack() const { return current_track_; }
        int getCurrentStep() const { return current_step_; }
        int getCurrentPattern() const { return current_pattern_; }
        
        void quickSave() {
            std::cout << "[UI] 💾 Quick save executed!" << std::endl;
        }
        
        void quickLoad() {
            std::cout << "[UI] 📁 Quick load executed!" << std::endl;
        }
        
        void setZoomLevel(float zoom) {
            std::cout << "[UI] 🔍 Zoom set to " << zoom << "x" << std::endl;
        }
    };
}

/**
 * @brief Simple test of MIDI → UI bridge functionality
 */
class SimpleMidiUITest {
private:
    std::shared_ptr<Parameters::ParameterManager> param_manager_;
    std::shared_ptr<UI::SequencerUIManager> ui_manager_;
    UI::MidiUIBridge* bridge_;
    
public:
    SimpleMidiUITest() {
        std::cout << "🎛️ === SIMPLE MIDI → UI TEST ===" << std::endl;
        
        // Initialize components
        param_manager_ = std::make_shared<Parameters::ParameterManager>();
        param_manager_->initialize();
        
        ui_manager_ = std::make_shared<UI::SequencerUIManager>(
            nullptr, nullptr, nullptr, param_manager_);
        
        // Initialize bridge
        bridge_ = &UI::MidiUIBridge::getInstance();
        bridge_->initialize(param_manager_);
        bridge_->setUIManager(ui_manager_);
        
        std::cout << "✅ Test setup complete!" << std::endl;
    }
    
    ~SimpleMidiUITest() {
        bridge_->shutdown();
        param_manager_->shutdown();
        std::cout << "💤 Test shutdown complete" << std::endl;
    }
    
    void runTests() {
        std::cout << "\n🧪 Running MIDI → UI tests..." << std::endl;
        
        // Test 1: Track focus control
        testTrackFocus();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Test 2: Step focus control  
        testStepFocus();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Test 3: Pattern switching
        testPatternSwitching();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Test 4: Quick actions
        testQuickActions();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Test 5: Custom actions
        testCustomActions();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Test 6: Statistics
        testStatistics();
        
        std::cout << "\n🎉 All tests completed!" << std::endl;
    }
    
private:
    void testTrackFocus() {
        std::cout << "\n1️⃣ Testing Track Focus Control..." << std::endl;
        
        // Map CC 16 to track focus
        bridge_->mapMidiToUI(1, 16, UI::MidiUIBridge::UIParameterID::UI_FOCUS_TRACK);
        
        // Simulate MIDI CC inputs
        simulateMidiCC(1, 16, 0);   // Track 0
        simulateMidiCC(1, 16, 64);  // Track 4  
        simulateMidiCC(1, 16, 127); // Track 7
    }
    
    void testStepFocus() {
        std::cout << "\n2️⃣ Testing Step Focus Control..." << std::endl;
        
        // Map CC 17 to step focus
        bridge_->mapMidiToUI(1, 17, UI::MidiUIBridge::UIParameterID::UI_FOCUS_STEP);
        
        // Simulate MIDI CC inputs
        simulateMidiCC(1, 17, 32);  // Step 4
        simulateMidiCC(1, 17, 96);  // Step 12
        simulateMidiCC(1, 17, 127); // Step 15
    }
    
    void testPatternSwitching() {
        std::cout << "\n3️⃣ Testing Pattern Switching..." << std::endl;
        
        // Map CC 18 to pattern selection
        bridge_->mapMidiToUI(1, 18, UI::MidiUIBridge::UIParameterID::UI_PAGE_SELECT);
        
        // Simulate pattern changes
        simulateMidiCC(1, 18, 0);   // Pattern 0
        simulateMidiCC(1, 18, 85);  // Pattern 10
        simulateMidiCC(1, 18, 127); // Pattern 15
    }
    
    void testQuickActions() {
        std::cout << "\n4️⃣ Testing Quick Actions..." << std::endl;
        
        // Map quick save/load
        bridge_->mapMidiToUI(3, 56, UI::MidiUIBridge::UIParameterID::UI_QUICK_SAVE);
        bridge_->mapMidiToUI(3, 57, UI::MidiUIBridge::UIParameterID::UI_QUICK_LOAD);
        
        // Test quick actions (only trigger on high values)
        simulateMidiCC(3, 56, 127); // Quick save
        simulateMidiCC(3, 57, 100); // Quick load
        simulateMidiCC(3, 56, 30);  // Should not trigger (too low)
    }
    
    void testCustomActions() {
        std::cout << "\n5️⃣ Testing Custom Actions..." << std::endl;
        
        // Register custom zoom action
        bridge_->registerUIAction(
            UI::MidiUIBridge::UIParameterID::UI_ZOOM_LEVEL,
            [](float value) {
                int zoom_percent = static_cast<int>(value * 100.0f);
                std::cout << "🔍 Custom Zoom: " << zoom_percent << "%" << std::endl;
                
                if (zoom_percent < 30) {
                    std::cout << "   → Overview mode activated" << std::endl;
                } else if (zoom_percent > 70) {
                    std::cout << "   → Detail mode activated" << std::endl;
                } else {
                    std::cout << "   → Normal view" << std::endl;
                }
            }
        );
        
        // Map and test custom action
        bridge_->mapMidiToUI(4, 50, UI::MidiUIBridge::UIParameterID::UI_ZOOM_LEVEL);
        
        simulateMidiCC(4, 50, 25);  // Overview
        simulateMidiCC(4, 50, 64);  // Normal
        simulateMidiCC(4, 50, 120); // Detail
    }
    
    void testStatistics() {
        std::cout << "\n6️⃣ Testing Statistics..." << std::endl;
        
        std::cout << "Active mappings: " << bridge_->getActiveMappingCount() << std::endl;
        
        // Print statistics
        bridge_->printUIMappingStatistics();
        
        // Export mappings
        std::cout << "\nMappings export:\n" << bridge_->exportUIMappings() << std::endl;
    }
    
    void simulateMidiCC(uint8_t channel, uint8_t cc, uint8_t value) {
        // Get mapped parameter
        auto param_id = param_manager_->getMidiMapping(channel, cc);
        
        if (param_id != 0) {
            // Create parameter change event
            float normalized = static_cast<float>(value) / 127.0f;
            Parameters::ParameterChangeEvent event(param_id, normalized, Parameters::ParameterSource::MIDI_CC);
            event.midi_channel = channel;
            
            // Check if it's a UI parameter and process it
            if (bridge_->isUIParameter(param_id)) {
                bridge_->onParameterChanged(event);
            }
            
            std::cout << "📡 MIDI CC" << (int)cc << " Ch" << (int)channel 
                      << " (" << (int)value << ") processed" << std::endl;
        } else {
            std::cout << "⚠️ MIDI CC" << (int)cc << " Ch" << (int)channel 
                      << " not mapped" << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
};

int main() {
    try {
        SimpleMidiUITest test;
        test.runTests();
        
        std::cout << "\n✨ Test completed successfully!" << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}
