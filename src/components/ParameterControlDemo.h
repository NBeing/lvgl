#pragma once

#include "ParameterControl.h"
#include "ParameterBinder.h"
#include <lvgl.h>
#include <memory>
#include <vector>
#include <iostream>

/**
 * @brief Demonstration of the new parameter-aware control system
 * 
 * This class shows how to use the new DialControl with parameter binding
 * instead of the old MidiDial with hard-coded CC numbers.
 */
class ParameterControlDemo {
public:
    ParameterControlDemo(lv_obj_t* parent);
    ~ParameterControlDemo();
    
    void createDemo();
    void cleanup();
    
private:
    lv_obj_t* parent_;
    std::unique_ptr<ParameterBinder> parameter_binder_;
    std::vector<std::shared_ptr<DialControl>> dial_controls_;
    
    void createParameterDials();
    void setupParameterBindings();
    void createDemoUI();
};

/**
 * @brief Usage example showing the difference between old and new approaches
 */
class UsageComparison {
public:
    // OLD WAY (with MidiDial)
    static void createOldStyleDial(lv_obj_t* /* parent */) {
        // auto dial = std::make_unique<MidiDial>(parent, "Cutoff", 10, 10);
        // dial->setMidiCC(74);  // Hard-coded CC number
        // dial->setRange(0, 127);
        // dial->onValueChanged([](int value) {
        //     // Manual MIDI sending
        //     // sendMidiCC(74, value);
        //     std::cout << "Cutoff CC74: " << value << std::endl;
        // });
    }
    
    // NEW WAY (with DialControl + Parameter binding)
    static void createNewStyleDial(lv_obj_t* parent) {
        // Create parameter binder and load synth definition
        auto binder = std::make_unique<ParameterBinder>();
        binder->loadSynthDefinition("hydrasynth");
        
        // Create dial control
        auto dial = std::make_shared<DialControl>(parent, 10, 10);
        
        // Find and bind parameter by name (human-readable!)
        auto cutoff_param = binder->findParameterByName("Filter 1 Cutoff");
        dial->bindParameter(cutoff_param);
        
        // Set up callback (receives parameter info)
        dial->setValueChangedCallback([](uint8_t /* value */, const Parameter* param) {
            // Note: Using printf instead of std::cout for embedded compatibility
            printf("%s (CC%d): %s\n", 
                   param->getName().c_str(), 
                   static_cast<int>(param->getCCNumber()),
                   param->getValueDisplayText().c_str());
            // Auto MIDI sending with correct CC number
            // sendMidiCC(param->getCCNumber(), value);
        });
        
        // Dial now automatically shows:
        // - Parameter name: "F1 Cutoff"
        // - Current value with proper formatting
        // - Correct CC number (74) for MIDI output
        // - Proper min/max range from parameter definition
    }
};

/**
 * @brief Benefits of the new system
 * 
 * 1. **Human-readable parameter names** - "Filter Cutoff" instead of "CC 74"
 * 2. **Automatic CC mapping** - No need to remember CC numbers
 * 3. **Parameter definitions** - Min/max, defaults, descriptions all included
 * 4. **Synthesizer awareness** - Load different synth definitions
 * 5. **Dynamic binding** - Change parameter assignments at runtime
 * 6. **Observer pattern** - Multiple controls can bind to same parameter
 * 7. **Proper value formatting** - Bipolar parameters show +/- values
 * 8. **Consistent UI** - All controls use same parameter system
 * 9. **Future extensibility** - Easy to add new control types
 * 10. **Better abstraction** - UI separated from MIDI implementation
 */
