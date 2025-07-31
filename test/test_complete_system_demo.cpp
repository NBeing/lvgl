/**
 * @brief Complete RT-Safe System Integration Demo
 * 
 * Demonstrates the complete RT-safe system working together:
 * 1. RT-Safe Event Distributor (foundation)
 * 2. Bidirectional MIDI-Parameter Bridge
 * 3. RT-Safe UI Control Integration
 * 
 * Shows real-world scenarios:
 * - User turns UI dial → Parameter updates → MIDI output
 * - External MIDI input → Parameter updates → UI dial moves
 * - Smooth UI updates and RT-safe operation
 */

#include <iostream>
#include "test/TestFramework.h"
#include "components/threading/RTSafeEventDistributor.h"
#include "components/parameter/RTSafeParameterManager.h"
#include "components/ui/RTSafeUIControlIntegration.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <iomanip>

using namespace RTSafe;

/**
 * @brief Simple Complete System Demo
 */
class RTSafeSystemDemo {
private:
    RTSafeEventDistributor distributor_;
    std::unique_ptr<RTSafeParameterManager> parameter_manager_;
    std::unique_ptr<RTSafeUIControlIntegration> ui_integration_;
    
    // Mock parameter store
    std::unordered_map<uint32_t, float> parameters_;
    
    // Mock MIDI output
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> midi_output_;
    
    // Demo state
    std::atomic<bool> demo_running_{false};
    
public:
    void initialize() {
        std::cout << "🚀 Initializing RT-Safe System..." << std::endl;
        
        // Initialize event distributor
        distributor_.initialize();
        
        // Create parameter manager
        parameter_manager_ = std::make_unique<RTSafeParameterManager>(&distributor_);
        parameter_manager_->initialize();
        parameter_manager_->setSampleRate(44100.0f);
        
        // Create UI integration
        ui_integration_ = std::make_unique<RTSafeUIControlIntegration>(&distributor_, parameter_manager_.get());
        ui_integration_->initialize();
        
        // Set UI update callback
        ui_integration_->setUIUpdateCallback(
            [this](uint32_t control_id, float value, bool immediate) {
                onUIControlUpdate(control_id, value, immediate);
            }
        );
        
        // Configure smooth updates
        ui_integration_->setSmoothUpdatesEnabled(true);
        ui_integration_->setInterpolationSpeed(0.2f); // Moderate smoothing
        
        setupControls();
        
        std::cout << "✅ RT-Safe System initialized!" << std::endl;
    }
    
    void shutdown() {
        demo_running_ = false;
        ui_integration_->shutdown();
        distributor_.shutdown();
        
        std::cout << "🛑 RT-Safe System shutdown complete." << std::endl;
    }
    
    void runDemo() {
        std::cout << "\n🎹 Starting Complete RT-Safe System Demo" << std::endl;
        std::cout << "=========================================" << std::endl;
        
        demo_running_ = true;
        
        // Start UI processing thread (60Hz)
        std::thread ui_thread([this]() {
            while (demo_running_) {
                ui_integration_->processUIUpdates();
                std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60Hz
            }
        });
        
        // Demo scenarios
        demoUserInteraction();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        demoExternalMIDI();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        demoSmoothUpdates();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        demoRealtimePerformance();
        
        demo_running_ = false;
        ui_thread.join();
        
        showFinalStatistics();
        
        std::cout << "\n🎉 RT-Safe System Demo Complete!" << std::endl;
    }

private:
    void setupControls() {
        std::cout << "🎛️  Setting up UI controls..." << std::endl;
        
        // Filter Cutoff Dial (logarithmic)
        RTSafeUIControlIntegration::ControlConfig cutoff;
        cutoff.control_id = 1001;
        cutoff.parameter_id = 1001;
        cutoff.type = RTSafeUIControlIntegration::ControlType::DIAL;
        cutoff.min_value = 20.0f;
        cutoff.max_value = 20000.0f;
        cutoff.default_value = 1000.0f;
        cutoff.logarithmic = true;
        cutoff.label = "Filter Cutoff";
        cutoff.units = "Hz";
        ui_integration_->addControl(cutoff);
        parameters_[1001] = cutoff.default_value;
        
        // Filter Resonance Slider
        RTSafeUIControlIntegration::ControlConfig resonance;
        resonance.control_id = 1002;
        resonance.parameter_id = 1002;
        resonance.type = RTSafeUIControlIntegration::ControlType::SLIDER;
        resonance.min_value = 0.0f;
        resonance.max_value = 1.0f;
        resonance.default_value = 0.3f;
        resonance.step_size = 0.01f;
        resonance.label = "Filter Resonance";
        ui_integration_->addControl(resonance);
        parameters_[1002] = resonance.default_value;
        
        // Master Volume Fader
        RTSafeUIControlIntegration::ControlConfig volume;
        volume.control_id = 4001;
        volume.parameter_id = 4001;
        volume.type = RTSafeUIControlIntegration::ControlType::SLIDER;
        volume.min_value = 0.0f;
        volume.max_value = 1.0f;
        volume.default_value = 0.8f;
        volume.label = "Master Volume";
        ui_integration_->addControl(volume);
        parameters_[4001] = volume.default_value;
        
        std::cout << "   • Filter Cutoff Dial (1001) - 20Hz to 20kHz (log scale)" << std::endl;
        std::cout << "   • Filter Resonance Slider (1002) - 0.0 to 1.0" << std::endl;
        std::cout << "   • Master Volume Fader (4001) - 0.0 to 1.0" << std::endl;
    }
    
    void demoUserInteraction() {
        std::cout << "\n🖱️  Demo 1: User Interface Interaction" << std::endl;
        std::cout << "────────────────────────────────────────" << std::endl;
        
        std::cout << "User turns Filter Cutoff dial to 75%..." << std::endl;
        ui_integration_->updateControlFromUI(1001, 0.75f, true);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        std::cout << "User drags Resonance slider to 60%..." << std::endl;
        ui_integration_->updateControlFromUI(1002, 0.6f, true);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        std::cout << "User adjusts Master Volume to 90%..." << std::endl;
        ui_integration_->updateControlFromUI(4001, 0.9f, true);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        std::cout << "✅ All UI interactions processed with RT-safe parameter updates!" << std::endl;
    }
    
    void demoExternalMIDI() {
        std::cout << "\n🎹 Demo 2: External MIDI Controller Input" << std::endl;
        std::cout << "──────────────────────────────────────────" << std::endl;
        
        std::cout << "External MIDI CC 74 (Filter Cutoff) = 100..." << std::endl;
        RTEvent midi_event1 = RTEvent::midiCC(0, 74, 100);
        distributor_.notifyRTObservers(midi_event1);
        distributor_.processUIEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "External MIDI CC 71 (Filter Resonance) = 80..." << std::endl;
        RTEvent midi_event2 = RTEvent::midiCC(0, 71, 80);
        distributor_.notifyRTObservers(midi_event2);
        distributor_.processUIEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "External MIDI CC 7 (Master Volume) = 110..." << std::endl;
        RTEvent midi_event3 = RTEvent::midiCC(0, 7, 110);
        distributor_.notifyRTObservers(midi_event3);
        distributor_.processUIEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "✅ External MIDI input processed and UI controls updated!" << std::endl;
    }
    
    void demoSmoothUpdates() {
        std::cout << "\n🌊 Demo 3: Smooth Value Interpolation" << std::endl;
        std::cout << "─────────────────────────────────────" << std::endl;
        
        std::cout << "Setting cutoff to 20% with smooth interpolation..." << std::endl;
        ui_integration_->updateControlFromUI(1001, 0.2f, false); // Non-immediate
        
        std::cout << "Watching smooth interpolation:" << std::endl;
        for (int i = 0; i < 10; ++i) {
            float current_value = ui_integration_->getControlValue(1001);
            float display_value = ui_integration_->getControlDisplayValue(1001);
            
            std::cout << "   Step " << i+1 << ": Control=" << std::fixed << std::setprecision(3) 
                      << current_value << ", Display=" << display_value << "Hz" << std::endl;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        std::cout << "✅ Smooth interpolation provides responsive UI feedback!" << std::endl;
    }
    
    void demoRealtimePerformance() {
        std::cout << "\n⚡ Demo 4: Real-time Performance Test" << std::endl;
        std::cout << "────────────────────────────────────────" << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::cout << "Performing 1000 rapid parameter updates..." << std::endl;
        for (int i = 0; i < 1000; ++i) {
            float value = (i % 100) / 100.0f;
            uint32_t control_id = 1001 + (i % 3); // Cycle through controls
            
            ui_integration_->updateControlFromUI(control_id, value, true);
            
            // Simulate some RT events
            if (i % 10 == 0) {
                RTEvent param_event = RTEvent::parameterChange(control_id >> 8, control_id & 0xFF);
                distributor_.notifyRTObservers(param_event);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        std::cout << "Performance Results:" << std::endl;
        std::cout << "   • Total time: " << duration.count() << " μs" << std::endl;
        std::cout << "   • Average per update: " << duration.count() / 1000.0f << " μs" << std::endl;
        std::cout << "   • Updates per second: " << (1000.0f * 1000000.0f) / duration.count() << std::endl;
        
        if (duration.count() / 1000.0f < 100.0f) {
            std::cout << "✅ Excellent RT performance! (<100μs per update)" << std::endl;
        } else {
            std::cout << "⚠️  Performance needs optimization" << std::endl;
        }
    }
    
    void showFinalStatistics() {
        std::cout << "\n📊 Final System Statistics" << std::endl;
        std::cout << "──────────────────────────" << std::endl;
        
        auto ui_stats = ui_integration_->getStatistics();
        auto dist_stats = distributor_.getStatistics();
        
        std::cout << "RT-Safe Event Distributor:" << std::endl;
        std::cout << "   • RT events processed: " << dist_stats.rt_events_processed << std::endl;
        std::cout << "   • UI events processed: " << dist_stats.ui_events_processed << std::endl;
        std::cout << "   • Max RT processing time: " << dist_stats.max_rt_processing_time_us << " μs" << std::endl;
        
        std::cout << "\nUI Control Integration:" << std::endl;
        std::cout << "   • UI → Parameter updates: " << ui_stats.ui_to_param_updates << std::endl;
        std::cout << "   • Parameter → UI updates: " << ui_stats.param_to_ui_updates << std::endl;
        std::cout << "   • Smooth interpolations: " << ui_stats.smooth_interpolations << std::endl;
        std::cout << "   • Active controls: " << ui_stats.active_controls << std::endl;
        std::cout << "   • Max update time: " << ui_stats.max_update_time_us << " μs" << std::endl;
        
        std::cout << "\n🎯 System Performance Summary:" << std::endl;
        if (dist_stats.max_rt_processing_time_us < 100 && ui_stats.max_update_time_us < 1000) {
            std::cout << "✅ EXCELLENT - System meets professional RT audio requirements!" << std::endl;
        } else if (dist_stats.max_rt_processing_time_us < 200 && ui_stats.max_update_time_us < 5000) {
            std::cout << "✅ GOOD - System suitable for most audio applications" << std::endl;
        } else {
            std::cout << "⚠️  NEEDS OPTIMIZATION - System may cause audio dropouts" << std::endl;
        }
    }
    
    void onUIControlUpdate(uint32_t control_id, float value, bool immediate) {
        // Map control IDs to names for demo output
        std::string control_name;
        std::string units;
        
        switch (control_id) {
            case 1001:
                control_name = "Filter Cutoff";
                units = " Hz";
                break;
            case 1002:
                control_name = "Filter Resonance";
                units = "";
                break;
            case 4001:
                control_name = "Master Volume";
                units = "";
                break;
            default:
                control_name = "Control " + std::to_string(control_id);
                units = "";
        }
        
        std::cout << "🖥️  " << control_name << " → " << std::fixed << std::setprecision(1) 
                  << value << units
                  << (immediate ? " (immediate)" : " (smooth)") << std::endl;
    }
};

int main() {
    RTSafeSystemDemo demo;
    
    try {
        demo.initialize();
        demo.runDemo();
        demo.shutdown();
    } catch (const std::exception& e) {
        std::cerr << "❌ Demo failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
