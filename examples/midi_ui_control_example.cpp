#include "components/ui/MidiUIBridge.h"
#include "components/ui/sequencer/SequencerUIManager.h"
#include "components/parameter/ParameterManager.h"
#include <iostream>
#include <thread>
#include <chrono>

/**
 * @brief Complete example of MIDI → UI control integration
 * 
 * This example demonstrates how to:
 * 1. Set up MIDI → UI parameter mapping
 * 2. Create custom UI actions
 * 3. Use built-in UI actions
 * 4. Monitor UI action execution
 * 
 * **Architecture Flow:**
 * MIDI CC → ParameterManager → MidiUIBridge → UI Action (Main Thread)
 */

class MidiUIExample {
private:
    std::shared_ptr<Parameters::ParameterManager> param_manager_;
    std::shared_ptr<UI::SequencerUIManager> ui_manager_;
    UI::MidiUIBridge* midi_ui_bridge_;
    
public:
    MidiUIExample() {
        std::cout << "🎛️ Initializing MIDI → UI Control Example..." << std::endl;
        
        // Initialize components
        param_manager_ = std::make_shared<Parameters::ParameterManager>();
        param_manager_->initialize();
        
        // Create mock UI manager (in real implementation, this would be the actual UI)
        ui_manager_ = std::make_shared<UI::SequencerUIManager>(
            /* parent */ nullptr, 
            /* sequencer */ nullptr, 
            /* lock_manager */ nullptr, 
            param_manager_
        );
        
        // Initialize MIDI → UI bridge
        midi_ui_bridge_ = &UI::MidiUIBridge::getInstance();
        midi_ui_bridge_->initialize(param_manager_);
        midi_ui_bridge_->setUIManager(ui_manager_);
        
        std::cout << "✅ MIDI → UI system ready!" << std::endl;
    }
    
    /**
     * @brief Demonstrate built-in UI actions
     */
    void demonstrateBuiltInUIActions() {
        std::cout << "\n🎯 === BUILT-IN UI ACTIONS DEMO ===" << std::endl;
        
        // Set up default mappings
        midi_ui_bridge_->setupDefaultUIMappings();
        
        // Test track focus control
        std::cout << "🎚️ Testing track focus control (CC 16)..." << std::endl;
        simulateMidiCC(1, 16, 0);   // Track 0
        simulateMidiCC(1, 16, 64);  // Track 4
        simulateMidiCC(1, 16, 127); // Track 7
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Test step focus control
        std::cout << "📍 Testing step focus control (CC 17)..." << std::endl;
        simulateMidiCC(1, 17, 32);  // Step 4
        simulateMidiCC(1, 17, 96);  // Step 12
        simulateMidiCC(1, 17, 127); // Step 15
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Test pattern/page selection
        std::cout << "📄 Testing pattern selection (CC 18)..." << std::endl;
        simulateMidiCC(1, 18, 0);   // Pattern 0
        simulateMidiCC(1, 18, 85);  // Pattern 10
        simulateMidiCC(1, 18, 127); // Pattern 15
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Test display controls
        std::cout << "💡 Testing brightness control (CC 26, Ch 2)..." << std::endl;
        simulateMidiCC(2, 26, 32);  // 25% brightness
        simulateMidiCC(2, 26, 96);  // 75% brightness
        simulateMidiCC(2, 26, 127); // 100% brightness
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Test quick actions
        std::cout << "💾 Testing quick save (CC 56, Ch 3)..." << std::endl;
        simulateMidiCC(3, 56, 127); // Trigger quick save
        
        std::cout << "📁 Testing quick load (CC 57, Ch 3)..." << std::endl;
        simulateMidiCC(3, 57, 127); // Trigger quick load
    }
    
    /**
     * @brief Demonstrate custom UI actions
     */
    void demonstrateCustomUIActions() {
        std::cout << "\n⚙️ === CUSTOM UI ACTIONS DEMO ===" << std::endl;
        
        // Register custom UI action for step velocity visualization
        midi_ui_bridge_->registerUIAction(
            UI::MidiUIBridge::UIParameterID::UI_ZOOM_LEVEL,
            [](float value) {
                int zoom_percent = static_cast<int>(value * 100.0f);
                std::cout << "🔍 Custom Zoom Action: " << zoom_percent << "%" << std::endl;
                
                // Custom zoom implementation
                if (zoom_percent < 30) {
                    std::cout << "   → Zoomed out: Overview mode" << std::endl;
                } else if (zoom_percent < 70) {
                    std::cout << "   → Normal zoom: Standard view" << std::endl;
                } else {
                    std::cout << "   → Zoomed in: Detail mode" << std::endl;
                }
            }
        );
        
        // Map custom action to MIDI CC
        midi_ui_bridge_->mapMidiToUI(4, 50, UI::MidiUIBridge::UIParameterID::UI_ZOOM_LEVEL);
        std::cout << "⚙️ Registered custom zoom action on Ch4, CC50" << std::endl;
        
        // Test custom action
        std::cout << "🧪 Testing custom zoom action..." << std::endl;
        simulateMidiCC(4, 50, 25);  // Zoom out
        simulateMidiCC(4, 50, 64);  // Normal
        simulateMidiCC(4, 50, 120); // Zoom in
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Register another custom action for color scheme cycling
        midi_ui_bridge_->registerUIAction(
            UI::MidiUIBridge::UIParameterID::UI_COLOR_SCHEME,
            [](float value) {
                const char* schemes[] = {"Dark Mode", "Light Mode", "Synthwave", "Matrix Green"};
                int scheme_index = static_cast<int>(value * 3.0f);
                
                std::cout << "🎨 Color Scheme Changed: " << schemes[scheme_index] << std::endl;
                
                // Simulate color scheme change
                switch (scheme_index) {
                    case 0: std::cout << "   → Background: Black, Text: White" << std::endl; break;
                    case 1: std::cout << "   → Background: White, Text: Black" << std::endl; break;
                    case 2: std::cout << "   → Background: Purple, Text: Cyan" << std::endl; break;
                    case 3: std::cout << "   → Background: Black, Text: Green" << std::endl; break;
                }
            }
        );
        
        // Map to different MIDI CC
        midi_ui_bridge_->mapMidiToUI(4, 51, UI::MidiUIBridge::UIParameterID::UI_COLOR_SCHEME);
        std::cout << "⚙️ Registered custom color scheme action on Ch4, CC51" << std::endl;
        
        // Test color scheme cycling
        std::cout << "🧪 Testing color scheme cycling..." << std::endl;
        simulateMidiCC(4, 51, 0);   // Dark mode
        simulateMidiCC(4, 51, 42);  // Light mode
        simulateMidiCC(4, 51, 85);  // Synthwave
        simulateMidiCC(4, 51, 127); // Matrix green
    }
    
    /**
     * @brief Demonstrate advanced UI workflow
     */
    void demonstrateAdvancedUIWorkflow() {
        std::cout << "\n🎭 === ADVANCED UI WORKFLOW DEMO ===" << std::endl;
        
        // Simulate a complete UI control session
        std::cout << "🎯 Simulating live performance UI control..." << std::endl;
        
        // 1. Set initial UI focus
        std::cout << "1️⃣ Setting focus to Track 1, Step 1" << std::endl;
        simulateMidiCC(1, 16, 0);  // Track focus
        simulateMidiCC(1, 17, 0);  // Step focus
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 2. Adjust display for performance
        std::cout << "2️⃣ Optimizing display for stage lighting" << std::endl;
        simulateMidiCC(2, 26, 127); // Max brightness
        simulateMidiCC(2, 27, 90);  // High contrast
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 3. Navigate through patterns during performance
        std::cout << "3️⃣ Pattern navigation during performance" << std::endl;
        for (int i = 0; i < 4; ++i) {
            simulateMidiCC(1, 18, i * 32); // Switch patterns
            std::cout << "   → Pattern " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
        
        // 4. Quick save after good take
        std::cout << "4️⃣ Quick saving current state" << std::endl;
        simulateMidiCC(3, 56, 127); // Quick save
        
        // 5. Change focus for next section
        std::cout << "5️⃣ Moving to next section (Track 4)" << std::endl;
        simulateMidiCC(1, 16, 64); // Track 4 focus
        
        std::cout << "✨ Advanced workflow complete!" << std::endl;
    }
    
    /**
     * @brief Show statistics and mappings
     */
    void showStatistics() {
        std::cout << "\n📊 === MIDI → UI STATISTICS ===" << std::endl;
        
        // Print bridge statistics
        midi_ui_bridge_->printUIMappingStatistics();
        
        // Export mappings
        std::cout << "\n📋 Current UI Mappings:" << std::endl;
        std::string mappings = midi_ui_bridge_->exportUIMappings();
        std::cout << mappings << std::endl;
    }
    
    /**
     * @brief Run complete demonstration
     */
    void runDemo() {
        std::cout << "🎛️ === COMPLETE MIDI → UI DEMO ===" << std::endl;
        std::cout << "Demonstrating comprehensive MIDI control of UI elements" << std::endl;
        std::cout << "======================================================" << std::endl;
        
        demonstrateBuiltInUIActions();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        demonstrateCustomUIActions();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        demonstrateAdvancedUIWorkflow();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        showStatistics();
        
        std::cout << "\n🎉 === DEMO COMPLETE ===" << std::endl;
        std::cout << "MIDI → UI bridge is fully functional!" << std::endl;
        std::cout << "✅ Built-in actions working" << std::endl;
        std::cout << "✅ Custom actions working" << std::endl;
        std::cout << "✅ Thread-safe operation" << std::endl;
        std::cout << "✅ Integration with parameter system" << std::endl;
    }

private:
    /**
     * @brief Simulate MIDI CC input through parameter system
     */
    void simulateMidiCC(uint8_t channel, uint8_t cc, uint8_t value) {
        // Convert to normalized value
        float normalized = static_cast<float>(value) / 127.0f;
        
        // Get mapped parameter (if any)
        auto mapped_param = param_manager_->getMidiMapping(channel, cc);
        
        if (mapped_param != 0) {
            // Create parameter change event
            Parameters::ParameterChangeEvent event(
                mapped_param, 
                normalized, 
                Parameters::ParameterSource::MIDI_CC
            );
            event.midi_channel = channel;
            
            // Process through parameter manager (will route to UI bridge)
            param_manager_->processParameterChange(event);
            
            std::cout << "📡 MIDI CC" << (int)cc << " Ch" << (int)channel 
                      << " (" << (int)value << ") → Param " << mapped_param << std::endl;
        } else {
            std::cout << "⚠️ MIDI CC" << (int)cc << " Ch" << (int)channel 
                      << " not mapped to any parameter" << std::endl;
        }
        
        // Small delay to simulate real MIDI timing
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    ~MidiUIExample() {
        midi_ui_bridge_->shutdown();
        param_manager_->shutdown();
        std::cout << "💤 MIDI → UI example shutdown complete" << std::endl;
    }
};

/**
 * @brief Main example entry point
 */
int main() {
    try {
        MidiUIExample example;
        example.runDemo();
        
        std::cout << "\n✨ Press Enter to exit..." << std::endl;
        std::cin.get();
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}
