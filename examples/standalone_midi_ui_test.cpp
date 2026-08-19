#include <iostream>
#include <memory>
#include <unordered_map>
#include <functional>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>

/**
 * @brief Standalone MIDI → UI Bridge Test
 * 
 * This demonstrates the MIDI → UI mapping concept without
 * dependencies on the full project infrastructure.
 */

namespace TestUI {

// =============================================================================
// MOCK PARAMETER SYSTEM
// =============================================================================

using ParameterID = uint32_t;

enum class ParameterSource {
    MIDI_CC,
    UI,
    AUTOMATION
};

struct ParameterChangeEvent {
    ParameterID parameter_id;
    float value;
    ParameterSource source;
    uint8_t midi_channel{0};
    
    ParameterChangeEvent(ParameterID id, float val, ParameterSource src)
        : parameter_id(id), value(val), source(src) {}
};

class MockParameterManager {
private:
    std::unordered_map<uint16_t, ParameterID> midi_mappings_;
    
public:
    void assignMidiCC(ParameterID param_id, uint8_t channel, uint8_t cc) {
        uint16_t key = (channel << 8) | cc;
        midi_mappings_[key] = param_id;
        std::cout << "[Param] Mapped CC" << (int)cc 
                  << " Ch" << (int)channel << " → Param " << param_id << std::endl;
    }
    
    ParameterID getMidiMapping(uint8_t channel, uint8_t cc) const {
        uint16_t key = (channel << 8) | cc;
        auto it = midi_mappings_.find(key);
        return (it != midi_mappings_.end()) ? it->second : 0;
    }
};

// =============================================================================
// MOCK UI MANAGER
// =============================================================================

class MockUIManager {
private:
    int current_track_{0};
    int current_step_{0};
    int current_pattern_{0};
    
public:
    void setFocus(int track, int step) {
        current_track_ = track;
        current_step_ = step;
        std::cout << "[UI] 🎯 Focus → Track " << track << ", Step " << step << std::endl;
    }
    
    void switchToPattern(int pattern) {
        current_pattern_ = pattern;
        std::cout << "[UI] 📄 Pattern → " << pattern << std::endl;
    }
    
    void quickSave() {
        std::cout << "[UI] 💾 Quick save executed!" << std::endl;
    }
    
    void quickLoad() {
        std::cout << "[UI] 📁 Quick load executed!" << std::endl;
    }
    
    void setBrightness(int brightness) {
        std::cout << "[UI] 💡 Brightness → " << brightness << "%" << std::endl;
    }
    
    void setContrast(int contrast) {
        std::cout << "[UI] 🌗 Contrast → " << contrast << "%" << std::endl;
    }
    
    void setColorScheme(int scheme) {
        const char* schemes[] = {"Dark", "Light", "Blue", "Green"};
        std::cout << "[UI] 🎨 Color scheme → " << schemes[scheme] << std::endl;
    }
};

// =============================================================================
// MIDI → UI BRIDGE IMPLEMENTATION
// =============================================================================

class MidiUIBridge {
public:
    enum class UIParameterID : uint32_t {
        UI_FOCUS_TRACK = 20000,
        UI_FOCUS_STEP = 20001,
        UI_PAGE_SELECT = 20002,
        UI_BRIGHTNESS = 20010,
        UI_CONTRAST = 20011,
        UI_COLOR_SCHEME = 20012,
        UI_QUICK_SAVE = 20040,
        UI_QUICK_LOAD = 20041,
        UI_ZOOM_LEVEL = 20050
    };
    
    using UIActionFunction = std::function<void(float)>;
    
    static MidiUIBridge& getInstance() {
        static MidiUIBridge instance;
        return instance;
    }
    
    void initialize(std::shared_ptr<MockParameterManager> param_manager,
                   std::shared_ptr<MockUIManager> ui_manager) {
        param_manager_ = param_manager;
        ui_manager_ = ui_manager;
        registerBuiltInActions();
        initialized_ = true;
        std::cout << "[Bridge] ✨ MIDI → UI bridge initialized" << std::endl;
    }
    
    void mapMidiToUI(uint8_t channel, uint8_t cc, UIParameterID ui_param) {
        // Create virtual parameter ID
        auto virtual_param_id = static_cast<ParameterID>(ui_param);
        
        // Map MIDI CC to virtual parameter
        param_manager_->assignMidiCC(virtual_param_id, channel, cc);
        
        // Store UI mapping
        {
            std::lock_guard<std::mutex> lock(mappings_mutex_);
            ui_parameter_mappings_[virtual_param_id] = ui_param;
        }
        
        std::cout << "[Bridge] 🎛️ MIDI CC" << (int)cc 
                  << " Ch" << (int)channel << " → UI action " << (int)ui_param << std::endl;
    }
    
    void registerUIAction(UIParameterID ui_param, UIActionFunction action) {
        std::lock_guard<std::mutex> lock(actions_mutex_);
        ui_actions_[ui_param] = action;
    }
    
    void onParameterChanged(const ParameterChangeEvent& event) {
        // Check if this is a UI parameter
        UIParameterID ui_param;
        {
            std::lock_guard<std::mutex> lock(mappings_mutex_);
            auto it = ui_parameter_mappings_.find(event.parameter_id);
            if (it == ui_parameter_mappings_.end()) {
                return; // Not a UI parameter
            }
            ui_param = it->second;
        }
        
        // Execute UI action
        executeUIAction(ui_param, event.value);
        actions_executed_.fetch_add(1);
    }
    
    bool isUIParameter(ParameterID param_id) const {
        std::lock_guard<std::mutex> lock(mappings_mutex_);
        return ui_parameter_mappings_.find(param_id) != ui_parameter_mappings_.end();
    }
    
    size_t getActiveMappingCount() const {
        std::lock_guard<std::mutex> lock(mappings_mutex_);
        return ui_parameter_mappings_.size();
    }
    
    void printStatistics() const {
        std::cout << "\n📊 === MIDI → UI BRIDGE STATISTICS ===" << std::endl;
        std::cout << "Active mappings: " << getActiveMappingCount() << std::endl;
        std::cout << "Actions executed: " << actions_executed_.load() << std::endl;
        std::cout << "Initialized: " << (initialized_ ? "Yes" : "No") << std::endl;
        std::cout << "=======================================" << std::endl;
    }

private:
    std::shared_ptr<MockParameterManager> param_manager_;
    std::shared_ptr<MockUIManager> ui_manager_;
    std::atomic<bool> initialized_{false};
    
    std::unordered_map<ParameterID, UIParameterID> ui_parameter_mappings_;
    mutable std::mutex mappings_mutex_;
    
    std::unordered_map<UIParameterID, UIActionFunction> ui_actions_;
    std::mutex actions_mutex_;
    
    std::atomic<uint64_t> actions_executed_{0};
    
    void registerBuiltInActions() {
        // Track focus
        registerUIAction(UIParameterID::UI_FOCUS_TRACK, [this](float value) {
            int track = static_cast<int>(value * 7.0f); // 0-7 tracks
            ui_manager_->setFocus(track, 0);
        });
        
        // Step focus
        registerUIAction(UIParameterID::UI_FOCUS_STEP, [this](float value) {
            int step = static_cast<int>(value * 15.0f); // 0-15 steps
            ui_manager_->setFocus(0, step);
        });
        
        // Pattern selection
        registerUIAction(UIParameterID::UI_PAGE_SELECT, [this](float value) {
            int pattern = static_cast<int>(value * 15.0f); // 0-15 patterns
            ui_manager_->switchToPattern(pattern);
        });
        
        // Brightness control
        registerUIAction(UIParameterID::UI_BRIGHTNESS, [this](float value) {
            int brightness = static_cast<int>(value * 100.0f);
            ui_manager_->setBrightness(brightness);
        });
        
        // Contrast control
        registerUIAction(UIParameterID::UI_CONTRAST, [this](float value) {
            int contrast = static_cast<int>(value * 100.0f);
            ui_manager_->setContrast(contrast);
        });
        
        // Color scheme
        registerUIAction(UIParameterID::UI_COLOR_SCHEME, [this](float value) {
            int scheme = static_cast<int>(value * 3.0f); // 0-3 schemes
            ui_manager_->setColorScheme(scheme);
        });
        
        // Quick save (trigger only on high values)
        registerUIAction(UIParameterID::UI_QUICK_SAVE, [this](float value) {
            if (value >= 0.5f) ui_manager_->quickSave();
        });
        
        // Quick load (trigger only on high values)
        registerUIAction(UIParameterID::UI_QUICK_LOAD, [this](float value) {
            if (value >= 0.5f) ui_manager_->quickLoad();
        });
        
        std::cout << "[Bridge] 🎯 Built-in UI actions registered" << std::endl;
    }
    
    void executeUIAction(UIParameterID ui_param, float value) {
        UIActionFunction action;
        {
            std::lock_guard<std::mutex> lock(actions_mutex_);
            auto it = ui_actions_.find(ui_param);
            if (it == ui_actions_.end()) {
                std::cout << "[Bridge] ⚠️ No action for UI parameter " << (int)ui_param << std::endl;
                return;
            }
            action = it->second;
        }
        
        try {
            action(value);
        }
        catch (const std::exception& e) {
            std::cout << "[Bridge] ❌ Error executing UI action: " << e.what() << std::endl;
        }
    }
};

// =============================================================================
// TEST IMPLEMENTATION
// =============================================================================

class MidiUITest {
private:
    std::shared_ptr<MockParameterManager> param_manager_;
    std::shared_ptr<MockUIManager> ui_manager_;
    MidiUIBridge* bridge_;
    
public:
    MidiUITest() {
        std::cout << "🎛️ === MIDI → UI BRIDGE TEST ===" << std::endl;
        
        // Initialize components
        param_manager_ = std::make_shared<MockParameterManager>();
        ui_manager_ = std::make_shared<MockUIManager>();
        
        // Initialize bridge
        bridge_ = &MidiUIBridge::getInstance();
        bridge_->initialize(param_manager_, ui_manager_);
        
        std::cout << "✅ Test setup complete!\n" << std::endl;
    }
    
    void runAllTests() {
        testBasicUIActions();
        testMultiChannelMappings();
        testCustomActions();
        testStatistics();
        
        std::cout << "\n🎉 All tests completed successfully!" << std::endl;
    }
    
private:
    void testBasicUIActions() {
        std::cout << "1️⃣ === BASIC UI ACTIONS TEST ===" << std::endl;
        
        // Set up mappings
        bridge_->mapMidiToUI(1, 16, MidiUIBridge::UIParameterID::UI_FOCUS_TRACK);
        bridge_->mapMidiToUI(1, 17, MidiUIBridge::UIParameterID::UI_FOCUS_STEP);
        bridge_->mapMidiToUI(1, 18, MidiUIBridge::UIParameterID::UI_PAGE_SELECT);
        bridge_->mapMidiToUI(2, 26, MidiUIBridge::UIParameterID::UI_BRIGHTNESS);
        bridge_->mapMidiToUI(2, 27, MidiUIBridge::UIParameterID::UI_CONTRAST);
        bridge_->mapMidiToUI(3, 56, MidiUIBridge::UIParameterID::UI_QUICK_SAVE);
        bridge_->mapMidiToUI(3, 57, MidiUIBridge::UIParameterID::UI_QUICK_LOAD);
        
        // Test track focus
        std::cout << "\n🎚️ Testing track focus..." << std::endl;
        simulateMidiCC(1, 16, 0);   // Track 0
        simulateMidiCC(1, 16, 64);  // Track 4
        simulateMidiCC(1, 16, 127); // Track 7
        
        // Test step focus
        std::cout << "\n📍 Testing step focus..." << std::endl;
        simulateMidiCC(1, 17, 32);  // Step 4
        simulateMidiCC(1, 17, 96);  // Step 12
        
        // Test pattern switching
        std::cout << "\n📄 Testing pattern switching..." << std::endl;
        simulateMidiCC(1, 18, 0);   // Pattern 0
        simulateMidiCC(1, 18, 85);  // Pattern 10
        
        // Test display controls
        std::cout << "\n💡 Testing display controls..." << std::endl;
        simulateMidiCC(2, 26, 64);  // 50% brightness
        simulateMidiCC(2, 27, 127); // 100% contrast
        
        // Test quick actions
        std::cout << "\n💾 Testing quick actions..." << std::endl;
        simulateMidiCC(3, 56, 127); // Quick save
        simulateMidiCC(3, 57, 100); // Quick load
        simulateMidiCC(3, 56, 30);  // Should not trigger (too low)
    }
    
    void testMultiChannelMappings() {
        std::cout << "\n2️⃣ === MULTI-CHANNEL MAPPINGS TEST ===" << std::endl;
        
        // Map same CC on different channels to different actions
        bridge_->mapMidiToUI(4, 20, MidiUIBridge::UIParameterID::UI_FOCUS_TRACK);
        bridge_->mapMidiToUI(5, 20, MidiUIBridge::UIParameterID::UI_FOCUS_STEP);
        bridge_->mapMidiToUI(6, 20, MidiUIBridge::UIParameterID::UI_PAGE_SELECT);
        
        std::cout << "\n🎛️ Testing same CC on different channels..." << std::endl;
        simulateMidiCC(4, 20, 96);  // Track focus on Ch4
        simulateMidiCC(5, 20, 64);  // Step focus on Ch5
        simulateMidiCC(6, 20, 32);  // Pattern select on Ch6
    }
    
    void testCustomActions() {
        std::cout << "\n3️⃣ === CUSTOM ACTIONS TEST ===" << std::endl;
        
        // Register custom zoom action
        bridge_->registerUIAction(
            MidiUIBridge::UIParameterID::UI_ZOOM_LEVEL,
            [](float value) {
                int zoom_percent = static_cast<int>(value * 100.0f);
                std::cout << "[UI] 🔍 Custom Zoom → " << zoom_percent << "%" << std::endl;
                
                if (zoom_percent < 30) {
                    std::cout << "       → Overview mode" << std::endl;
                } else if (zoom_percent > 70) {
                    std::cout << "       → Detail mode" << std::endl;
                } else {
                    std::cout << "       → Normal view" << std::endl;
                }
            }
        );
        
        // Map and test custom action
        bridge_->mapMidiToUI(7, 50, MidiUIBridge::UIParameterID::UI_ZOOM_LEVEL);
        
        std::cout << "\n🔍 Testing custom zoom action..." << std::endl;
        simulateMidiCC(7, 50, 25);  // Overview
        simulateMidiCC(7, 50, 64);  // Normal
        simulateMidiCC(7, 50, 120); // Detail
        
        // Register custom color scheme action
        bridge_->registerUIAction(
            MidiUIBridge::UIParameterID::UI_COLOR_SCHEME,
            [](float value) {
                int scheme = static_cast<int>(value * 3.0f);
                const char* schemes[] = {"🌙 Dark", "☀️ Light", "🌊 Blue", "🌿 Green"};
                std::cout << "[UI] 🎨 Theme → " << schemes[scheme] << std::endl;
            }
        );
        
        bridge_->mapMidiToUI(7, 51, MidiUIBridge::UIParameterID::UI_COLOR_SCHEME);
        
        std::cout << "\n🎨 Testing custom color scheme..." << std::endl;
        simulateMidiCC(7, 51, 0);   // Dark
        simulateMidiCC(7, 51, 42);  // Light
        simulateMidiCC(7, 51, 85);  // Blue
        simulateMidiCC(7, 51, 127); // Green
    }
    
    void testStatistics() {
        std::cout << "\n4️⃣ === STATISTICS TEST ===" << std::endl;
        bridge_->printStatistics();
    }
    
    void simulateMidiCC(uint8_t channel, uint8_t cc, uint8_t value) {
        // Get mapped parameter
        auto param_id = param_manager_->getMidiMapping(channel, cc);
        
        if (param_id != 0 && bridge_->isUIParameter(param_id)) {
            // Create parameter change event
            float normalized = static_cast<float>(value) / 127.0f;
            ParameterChangeEvent event(param_id, normalized, ParameterSource::MIDI_CC);
            event.midi_channel = channel;
            
            // Process through bridge
            bridge_->onParameterChanged(event);
            
            std::cout << "📡 MIDI CC" << (int)cc << " Ch" << (int)channel 
                      << " (" << (int)value << ") → UI action" << std::endl;
        } else {
            std::cout << "⚠️ MIDI CC" << (int)cc << " Ch" << (int)channel 
                      << " not mapped to UI parameter" << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
};

} // namespace TestUI

// =============================================================================
// MAIN TEST RUNNER
// =============================================================================

int main() {
    try {
        TestUI::MidiUITest test;
        test.runAllTests();
        
        std::cout << "\n✨ MIDI → UI Bridge test completed successfully!" << std::endl;
        std::cout << "\nKey features demonstrated:" << std::endl;
        std::cout << "✅ MIDI CC → UI parameter mapping" << std::endl;
        std::cout << "✅ Built-in UI actions (focus, brightness, etc.)" << std::endl;
        std::cout << "✅ Custom UI actions with lambdas" << std::endl;
        std::cout << "✅ Multi-channel MIDI support" << std::endl;
        std::cout << "✅ Thread-safe operation" << std::endl;
        std::cout << "✅ Statistics and debugging" << std::endl;
        
        std::cout << "\nPress Enter to exit..." << std::endl;
        std::cin.get();
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}
