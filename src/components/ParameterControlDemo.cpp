#include "ParameterControlDemo.h"
#include <iostream>

ParameterControlDemo::ParameterControlDemo(lv_obj_t* parent)
    : parent_(parent)
    , parameter_binder_(std::make_unique<ParameterBinder>())
{
}

ParameterControlDemo::~ParameterControlDemo() {
    cleanup();
}

void ParameterControlDemo::createDemo() {
    // Load Hydrasynth parameter definitions
    if (!parameter_binder_->loadSynthDefinition("hydrasynth")) {
        std::cout << "Failed to load Hydrasynth definition!" << std::endl;
        return;
    }
    
    std::cout << "Loaded " << parameter_binder_->getParameterCount() 
              << " parameters for " << parameter_binder_->getCurrentSynthName() << std::endl;
    
    createParameterDials();
    setupParameterBindings();
    
    // Demonstrate parameter search
    auto filter_params = parameter_binder_->searchParameters("filter");
    std::cout << "Found " << filter_params.size() << " filter-related parameters:" << std::endl;
    for (const auto& param : filter_params) {
        std::cout << "  - " << param->getName() << " (CC " 
                  << static_cast<int>(param->getCCNumber()) << ")" << std::endl;
    }
}

void ParameterControlDemo::createParameterDials() {
    // Create a grid of 6 dial controls
    const int cols = 3;
    const int rows = 2;
    const int dial_spacing_x = 100;
    const int dial_spacing_y = 100;
    const int start_x = 50;
    const int start_y = 50;
    
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            int x = start_x + col * dial_spacing_x;
            int y = start_y + row * dial_spacing_y;
            
            auto dial = std::make_shared<DialControl>(parent_, x, y);
            
            // Set different colors for visual distinction
            lv_color_t colors[] = {
                lv_color_hex(0xFF00FF00), // Green
                lv_color_hex(0xFF0080FF), // Blue
                lv_color_hex(0xFFFF8000), // Orange
                lv_color_hex(0xFFFF00FF), // Magenta
                lv_color_hex(0xFF80FF00), // Yellow-green
                lv_color_hex(0xFFFF0080)  // Pink
            };
            dial->setColor(colors[row * cols + col]);
            
            dial_controls_.push_back(dial);
        }
    }
}

void ParameterControlDemo::setupParameterBindings() {
    // Define which parameters to bind to each dial
    std::vector<std::string> parameter_names = {
        "Filter 1 Cutoff",
        "Filter 1 Resonance", 
        "Filter 1 Drive",
        "ENV 1 Attack",
        "ENV 1 Release",
        "LFO 1 Rate"
    };
    
    // Bind each dial to a parameter
    for (size_t i = 0; i < dial_controls_.size() && i < parameter_names.size(); ++i) {
        auto parameter = parameter_binder_->findParameterByName(parameter_names[i]);
        if (parameter) {
            dial_controls_[i]->bindParameter(parameter);
            
            // Set up value change callback
            dial_controls_[i]->setValueChangedCallback([](uint8_t /* value */, const Parameter* param) {
                std::cout << "Parameter changed: " << param->getName() 
                          << " = " << param->getValueDisplayText() 
                          << " (CC " << static_cast<int>(param->getCCNumber()) << ")" << std::endl;
                
                // Here you would send actual MIDI CC message:
                // midiHandler->sendCC(param->getCCNumber(), value);
            });
            
            std::cout << "Bound dial " << i << " to parameter: " 
                      << parameter->getName() << std::endl;
        } else {
            std::cout << "Parameter not found: " << parameter_names[i] << std::endl;
        }
    }
    
    // Demonstrate parameter categories
    auto filter_params = parameter_binder_->getParametersByCategory(ParameterCategory::FILTERS);
    std::cout << "Filter category has " << filter_params.size() << " parameters:" << std::endl;
    for (const auto& param : filter_params) {
        std::cout << "  - " << param->getShortName() << " (CC " 
                  << static_cast<int>(param->getCCNumber()) << ")" << std::endl;
    }
}

void ParameterControlDemo::cleanup() {
    dial_controls_.clear();
    parameter_binder_.reset();
}

/**
 * @brief Example of how to integrate this with existing SynthApp
 */
void integrateWithSynthApp() {
    /*
    // In SynthApp.h, replace:
    std::vector<std::unique_ptr<MidiDial>> midi_dials_;
    
    // With:
    std::unique_ptr<ParameterBinder> parameter_binder_;
    std::vector<std::shared_ptr<DialControl>> parameter_dials_;
    
    // In SynthApp.cpp constructor:
    parameter_binder_ = std::make_unique<ParameterBinder>();
    parameter_binder_->loadSynthDefinition("hydrasynth");
    
    // Create parameter-aware dials:
    for (int i = 0; i < 6; ++i) {
        auto dial = std::make_shared<DialControl>(container_, x, y);
        parameter_dials_.push_back(dial);
    }
    
    // Bind to specific parameters:
    parameter_dials_[0]->bindParameter(parameter_binder_->findParameterByName("Filter 1 Cutoff"));
    parameter_dials_[1]->bindParameter(parameter_binder_->findParameterByName("Filter 1 Resonance"));
    // etc...
    
    // Set up MIDI output:
    for (auto& dial : parameter_dials_) {
        dial->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
            // Send MIDI CC using existing MidiHandler
            midi_handler_->sendControlChange(param->getCCNumber(), value);
        });
    }
    */
}
