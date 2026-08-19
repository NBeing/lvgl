/**
 * @file enhanced_parameter_system_example.cpp
 * @brief Example demonstrating the enhanced synthesizer parameter system
 * 
 * This example shows how to integrate the SynthParameterManager with the existing
 * ParameterManager and use the ParameterSelectionUI for browsing and assigning parameters.
 */

#include "components/parameter/SynthParameterManager.h"
#include "components/parameter/ParameterManager.h"
#include "components/ui/ParameterSelectionUI.h"
#include "components/ui/WindowManager.h"
#include <iostream>
#include <memory>

class EnhancedParameterSystemExample {
public:
    EnhancedParameterSystemExample() {
        std::cout << "🎹 Enhanced Parameter System Example" << std::endl;
        std::cout << "====================================" << std::endl;
        
        initializeManagers();
        setupParameterBrowser();
        demonstrateFeatures();
    }
    
private:
    std::unique_ptr<SynthParameterManager> synth_param_manager_;
    std::unique_ptr<Parameters::ParameterManager> param_manager_;
    std::unique_ptr<ParameterSelectionUI> param_browser_;
    std::unique_ptr<ParameterSystemBridge> bridge_;
    
    void initializeManagers() {
        std::cout << "\n🔧 Initializing Parameter Managers..." << std::endl;
        
        // Initialize core parameter manager
        param_manager_ = std::make_unique<Parameters::ParameterManager>();
        param_manager_->initialize();
        
        // Initialize enhanced synthesizer parameter manager
        synth_param_manager_ = std::make_unique<SynthParameterManager>();
        
        // Create integration bridge
        bridge_ = std::make_unique<ParameterSystemBridge>(
            synth_param_manager_.get(), 
            param_manager_.get()
        );
        
        std::cout << "✅ Parameter managers initialized" << std::endl;
    }
    
    void setupParameterBrowser() {
        std::cout << "\n🖥️ Setting up Parameter Browser UI..." << std::endl;
        
        // In a real application, you'd pass the actual LVGL parent object
        // param_browser_ = std::make_unique<ParameterSelectionUI>(
        //     synth_param_manager_.get(),
        //     param_manager_.get()
        // );
        
        // Set up parameter selection callback
        // param_browser_->setSelectionCallback([this](auto param, const std::string& action) {
        //     handleParameterSelection(param, action);
        // });
        
        std::cout << "✅ Parameter browser UI ready" << std::endl;
    }
    
    void demonstrateFeatures() {
        std::cout << "\n🎛️ Demonstrating Enhanced Parameter Features..." << std::endl;
        
        // Load Hydrasynth definition
        demonstrateSynthLoading();
        
        // Show parameter search capabilities
        demonstrateParameterSearch();
        
        // Show favorites system
        demonstrateFavoritesSystem();
        
        // Show parameter assignment
        demonstrateParameterAssignment();
        
        // Show macro system
        demonstrateMacroSystem();
        
        // Show integration with existing system
        demonstrateSystemIntegration();
    }
    
    void demonstrateSynthLoading() {
        std::cout << "\n📁 Loading Synthesizer Definitions..." << std::endl;
        
        // Load Hydrasynth
        bool success = synth_param_manager_->loadSynthDefinition("hydrasynth");
        if (success) {
            std::cout << "✅ Loaded " << synth_param_manager_->getCurrentSynthName() << std::endl;
            std::cout << "   Parameters: " << synth_param_manager_->getParameterCount() << std::endl;
            
            // Show available categories
            auto categories = synth_param_manager_->getAvailableCategories();
            std::cout << "   Categories: ";
            for (auto cat : categories) {
                std::cout << ParameterSystemUtils::categoryToDisplayString(cat) << " ";
            }
            std::cout << std::endl;
        }
        
        // Show available synthesizers
        auto available_synths = synth_param_manager_->getAvailableSynths();
        std::cout << "   Available synthesizers: ";
        for (const auto& synth : available_synths) {
            std::cout << synth << " ";
        }
        std::cout << std::endl;
    }
    
    void demonstrateParameterSearch() {
        std::cout << "\n🔍 Parameter Search Capabilities..." << std::endl;
        
        // Search for filter parameters
        auto filter_params = synth_param_manager_->searchParameters("filter");
        std::cout << "   Filter parameters found: " << filter_params.size() << std::endl;
        for (size_t i = 0; i < std::min(size_t(3), filter_params.size()); ++i) {
            std::cout << "   - " << filter_params[i]->getName() 
                      << " (CC " << (int)filter_params[i]->getCCNumber() << ")" << std::endl;
        }
        
        // Search by category
        auto envelope_params = synth_param_manager_->getParametersByCategory(ParameterCategory::ENVELOPES);
        std::cout << "   Envelope parameters: " << envelope_params.size() << std::endl;
        
        // Find specific parameter
        auto cutoff_param = synth_param_manager_->findParameterByName("Filter 1 Cutoff");
        if (cutoff_param) {
            std::cout << "   Found " << cutoff_param->getName() 
                      << " - " << cutoff_param->getDescription() << std::endl;
        }
    }
    
    void demonstrateFavoritesSystem() {
        std::cout << "\n⭐ Favorites System..." << std::endl;
        
        // Add some parameters to favorites
        synth_param_manager_->addToFavorites("Filter 1 Cutoff");
        synth_param_manager_->addToFavorites("Filter 1 Resonance");
        synth_param_manager_->addToFavorites("LFO 1 Rate");
        
        auto favorites = synth_param_manager_->getFavoriteParameters();
        std::cout << "   Favorite parameters: " << favorites.size() << std::endl;
        for (const auto& fav : favorites) {
            std::cout << "   ⭐ " << fav->getName() << std::endl;
        }
        
        // Show recent parameters (simulated usage)
        synth_param_manager_->markParameterAsUsed("ENV 1 Attack");
        synth_param_manager_->markParameterAsUsed("Reverb Time");
        
        auto recents = synth_param_manager_->getRecentParameters(5);
        std::cout << "   Recent parameters: " << recents.size() << std::endl;
        for (const auto& recent : recents) {
            std::cout << "   🕒 " << recent->getName() << std::endl;
        }
    }
    
    void demonstrateParameterAssignment() {
        std::cout << "\n🔗 Parameter Assignment System..." << std::endl;
        
        // Assign parameters to different targets
        synth_param_manager_->assignParameter("Filter 1 Cutoff", "dial_1", "ui_control");
        synth_param_manager_->assignParameter("Filter 1 Resonance", "midi_ch1_cc71", "midi_cc");
        synth_param_manager_->assignParameter("LFO 1 Rate", "automation_track_1", "automation");
        
        // Show all assignments
        auto assignments = synth_param_manager_->getAllAssignments();
        std::cout << "   Active assignments: " << assignments.size() << std::endl;
        for (const auto& assignment : assignments) {
            std::cout << "   🔗 " << assignment.parameter_name 
                      << " → " << assignment.control_id 
                      << " (" << assignment.assignment_type << ")" << std::endl;
        }
        
        // Check assignment status
        bool is_assigned = synth_param_manager_->isParameterAssigned("Filter 1 Cutoff");
        std::cout << "   Filter 1 Cutoff assigned: " << (is_assigned ? "Yes" : "No") << std::endl;
    }
    
    void demonstrateMacroSystem() {
        std::cout << "\n🎛️ Macro System..." << std::endl;
        
        // Create a filter macro
        std::vector<std::string> filter_params = {
            "Filter 1 Cutoff",
            "Filter 1 Resonance", 
            "Filter 1 Drive"
        };
        std::vector<float> weights = {1.0f, 0.8f, 0.6f};
        
        synth_param_manager_->createMacro("Filter Control", filter_params, weights);
        
        // Create an envelope macro
        std::vector<std::string> env_params = {
            "ENV 1 Attack",
            "ENV 1 Decay",
            "ENV 1 Release"
        };
        
        synth_param_manager_->createMacro("Envelope Shape", env_params);
        
        auto macros = synth_param_manager_->getAllMacros();
        std::cout << "   Created macros: " << macros.size() << std::endl;
        for (const auto& macro : macros) {
            std::cout << "   🎛️ " << macro.name 
                      << " (controls " << macro.parameter_names.size() << " parameters)" << std::endl;
        }
        
        // Apply macro value
        std::cout << "   Applying macro 'Filter Control' with value 0.75..." << std::endl;
        synth_param_manager_->applyMacroValue("Filter Control", 0.75f, 
            [](const std::string& param_name, float value) {
                std::cout << "     " << param_name << " = " << value << std::endl;
            });
    }
    
    void demonstrateSystemIntegration() {
        std::cout << "\n🌉 System Integration..." << std::endl;
        
        // Find a parameter to demonstrate integration
        auto cutoff_param = synth_param_manager_->findParameterByName("Filter 1 Cutoff");
        if (cutoff_param) {
            std::cout << "   Demonstrating integration with: " << cutoff_param->getName() << std::endl;
            
            // Start MIDI learn
            bool learn_success = bridge_->startMidiLearn(cutoff_param);
            std::cout << "   MIDI learn started: " << (learn_success ? "Yes" : "No") << std::endl;
            
            // Assign to automation
            bool auto_success = bridge_->assignToAutomation(cutoff_param, "sequencer_track_1");
            std::cout << "   Automation assigned: " << (auto_success ? "Yes" : "No") << std::endl;
            
            // Get assignment status
            std::string status = bridge_->getAssignmentStatus(cutoff_param);
            std::cout << "   Assignment status: " << status << std::endl;
        }
        
        // Show statistics
        std::string stats = ParameterSystemUtils::generateParameterStatistics(*synth_param_manager_);
        std::cout << "\n📊 System Statistics:" << std::endl;
        std::cout << stats << std::endl;
    }
    
    void handleParameterSelection(std::shared_ptr<Parameter> parameter, const std::string& action) {
        std::cout << "🎯 Parameter selected: " << parameter->getName() 
                  << " (action: " << action << ")" << std::endl;
        
        if (action == "assign") {
            // Start assignment workflow
            std::cout << "   Starting assignment workflow..." << std::endl;
            
        } else if (action == "favorite") {
            // Toggle favorite status
            bool is_fav = synth_param_manager_->isFavorite(parameter->getName());
            if (is_fav) {
                synth_param_manager_->removeFromFavorites(parameter->getName());
            } else {
                synth_param_manager_->addToFavorites(parameter->getName());
            }
            
        } else if (action == "macro") {
            // Add to macro creation workflow
            std::cout << "   Adding to macro creation..." << std::endl;
        }
    }
};

int main() {
    try {
        EnhancedParameterSystemExample example;
        
        std::cout << "\n🎉 Enhanced Parameter System Demo Complete!" << std::endl;
        std::cout << "\nKey features demonstrated:" << std::endl;
        std::cout << "✅ Synthesizer definition loading" << std::endl;
        std::cout << "✅ Advanced parameter search and filtering" << std::endl;
        std::cout << "✅ Favorites and recent parameters" << std::endl;
        std::cout << "✅ Parameter assignment tracking" << std::endl;
        std::cout << "✅ Multi-parameter macro system" << std::endl;
        std::cout << "✅ Integration with existing parameter system" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}