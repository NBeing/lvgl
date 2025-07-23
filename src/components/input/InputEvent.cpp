#include "InputEvent.h"
#include <sstream>

namespace Input {

    std::string Event::toString() const {
        std::ostringstream oss;
        
        // Event type
        switch (type) {
            case EventType::KEY_PRESS: oss << "KEY_PRESS"; break;
            case EventType::KEY_RELEASE: oss << "KEY_RELEASE"; break;
            case EventType::KEY_HOLD: oss << "KEY_HOLD"; break;
            case EventType::BUTTON_PRESS: oss << "BUTTON_PRESS"; break;
            case EventType::BUTTON_RELEASE: oss << "BUTTON_RELEASE"; break;
            case EventType::BUTTON_HOLD: oss << "BUTTON_HOLD"; break;
            case EventType::ENCODER_TURN: oss << "ENCODER_TURN"; break;
            case EventType::TOUCH_PRESS: oss << "TOUCH_PRESS"; break;
            case EventType::TOUCH_RELEASE: oss << "TOUCH_RELEASE"; break;
        }
        
        oss << " ";
        
        // Source
        switch (source) {
            case Source::KEYBOARD: oss << "KEYBOARD"; break;
            case Source::HARDWARE_BUTTON: oss << "HARDWARE_BUTTON"; break;
            case Source::TOUCH_SCREEN: oss << "TOUCH_SCREEN"; break;
            case Source::ENCODER: oss << "ENCODER"; break;
            case Source::MIDI_CONTROLLER: oss << "MIDI_CONTROLLER"; break;
            case Source::VIRTUAL: oss << "VIRTUAL"; break;
        }
        
        oss << " key=" << static_cast<int>(key_code);
        
        // Modifiers
        if (modifiers != 0) {
            oss << " mods=";
            if (hasModifier(Modifier::SHIFT)) oss << "S";
            if (hasModifier(Modifier::CTRL)) oss << "C";
            if (hasModifier(Modifier::ALT)) oss << "A";
            if (hasModifier(Modifier::META)) oss << "M";
        }
        
        if (analog_value != 0) {
            oss << " analog=" << analog_value;
        }
        
        if (velocity != 0) {
            oss << " vel=" << static_cast<int>(velocity);
        }
        
        if (is_repeat) {
            oss << " [REPEAT]";
        }
        
        oss << " @" << timestamp;
        
        return oss.str();
    }

} // namespace Input
