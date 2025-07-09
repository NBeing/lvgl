// ==============================================
// MIDI DIAL WIDGET CLASS - 8-Bit Style
// ==============================================
#include "MidiDial.h"


#include "../../include/FontConfig.h"

// Static map to link LVGL objects back to C++ instances
static std::unordered_map<lv_obj_t*, MidiDial*> dial_widget_map;


MidiDial::MidiDial(lv_obj_t* parent, const char* label, int x, int y) 
    : current_value_(0)
    , min_value_(0) 
    , max_value_(127)
    , midi_cc_(-1)
    , arc_color_(lv_color_hex(0xFF00FF00))
{
    // Create container object
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, 20, 20);
    lv_obj_set_pos(container_, x, y);
    
    // Store mapping for static callbacks
    dial_widget_map[container_] = this;
    
    // Create parameter name label
    name_label_ = lv_label_create(container_);
    lv_label_set_text(name_label_, label);
    lv_obj_set_style_text_color(name_label_, lv_color_hex(0xFFCCCCCC), 0);
    lv_obj_set_style_text_font(name_label_, FontA.small, 0);
    lv_obj_align(name_label_, LV_ALIGN_TOP_MID, 0, 2);
    
    // Create arc display using LVGL's built-in arc widget
    arc_display_ = lv_arc_create(container_);
    lv_obj_set_size(arc_display_, 50, 50);
    lv_obj_align(arc_display_, LV_ALIGN_CENTER, 0, -5);
    
    // Configure arc for semicircle (8-bit style)
    lv_arc_set_bg_angles(arc_display_, 180, 0);  // Background semicircle
    lv_arc_set_angles(arc_display_, 180, 180);   // Start with no fill
    lv_arc_set_range(arc_display_, min_value_, max_value_);
    
    // Style the arc for 8-bit look
    lv_obj_set_style_arc_color(arc_display_, lv_color_hex(0xFF444444), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_display_, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_display_, arc_color_, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_display_, 6, LV_PART_INDICATOR);
    
    // Hide the knob for cleaner look
    lv_obj_set_style_bg_opa(arc_display_, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_opa(arc_display_, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc_display_, 0, LV_PART_KNOB);
    
    // Create value display label
    value_label_ = lv_label_create(container_);
    lv_label_set_text(value_label_, "000");
    lv_obj_set_style_text_color(value_label_, arc_color_, 0);
    lv_obj_set_style_text_font(value_label_, FontA.small, 0);
    lv_obj_align(value_label_, LV_ALIGN_BOTTOM_MID, 0, -2);
    
    // Set up event handlers for click interaction
    lv_obj_add_event_cb(container_, click_event_cb, LV_EVENT_CLICKED, this);
    // Make the arc interactive for dragging
    lv_obj_add_event_cb(arc_display_, arc_event_cb, LV_EVENT_VALUE_CHANGED, this);
    
    // Apply 8-bit retro styling
    setupStyling();
    
    // Initialize display
    updateDisplay();
    
    std::cout << "Created MIDI dial: " << label << std::endl;
        int w = lv_obj_get_width(container_);
    int h = lv_obj_get_height(container_);
    std::cout << "MidiDial container actual size: " << w << "x" << h << std::endl;
        if (lv_obj_get_parent(container_)) {
        int pw = lv_obj_get_width(lv_obj_get_parent(container_));
        int ph = lv_obj_get_height(lv_obj_get_parent(container_));
        std::cout << "MidiDial parent actual size: " << pw << "x" << ph << std::endl;
    }
}

lv_obj_t* MidiDial::getLvObj() const { return container_; }
int MidiDial::getValue() const { return current_value_; }
void MidiDial::setMidiCC(int cc_number) { midi_cc_ = cc_number; }
int MidiDial::getMidiCC() const { return midi_cc_; }

MidiDial::~MidiDial() {
    dial_widget_map.erase(container_);
}

void MidiDial::setValue(int value) {
    if (value < min_value_) value = min_value_;
    if (value > max_value_) value = max_value_;
    
    if (current_value_ != value) {
        current_value_ = value;
        updateDisplay();
        
        if (value_callback_) {
            value_callback_(value);
        }
        
        std::cout << "MIDI Dial value changed to: " << value << std::endl;
    }
}

void MidiDial::setRange(int min_val, int max_val) {
    min_value_ = min_val;
    max_value_ = max_val;
    setValue(current_value_);
}

void MidiDial::setColor(lv_color_t color) {
    arc_color_ = color;
    lv_obj_set_style_text_color(value_label_, color, 0);
    lv_obj_set_style_arc_color(arc_display_, color, LV_PART_INDICATOR);
}

void MidiDial::setPosition(int x, int y) {
    lv_obj_set_pos(container_, x, y);
}

void MidiDial::onValueChanged(std::function<void(int)> callback) {
    value_callback_ = callback;
}

void MidiDial::updateDisplay() {
    char text[8];
    snprintf(text, sizeof(text), "%03d", current_value_);
    lv_label_set_text(value_label_, text);
    
    // Only use LVGL's built-in arc value system
    lv_arc_set_value(arc_display_, current_value_);
}

void MidiDial::setupStyling() {
    // 8-bit retro container styling
    lv_obj_set_style_bg_color(container_, lv_color_hex(0xFF1a1a1a), 0);
    lv_obj_set_style_bg_opa(container_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(container_, lv_color_hex(0xFF666666), 0);
    lv_obj_set_style_border_width(container_, 2, 0);
    lv_obj_set_style_border_opa(container_, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(container_, 4, 0);
    lv_obj_set_style_pad_all(container_, 4, 0);
}

void MidiDial::click_event_cb(lv_event_t* e) {
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    
    auto it = dial_widget_map.find(obj);
    if (it != dial_widget_map.end()) {
        MidiDial* dial = it->second;
        
        // Increment value on click (for testing)
        int new_value = dial->getValue() + 10;
        if (new_value > dial->max_value_) {
            new_value = dial->min_value_;
        }
        dial->setValue(new_value);
        
        std::cout << "MIDI Dial clicked! New value: " << new_value << std::endl;
    }
}

void MidiDial::arc_event_cb(lv_event_t* e) {
    lv_obj_t* arc = static_cast<lv_obj_t*>(lv_event_get_target(e));
    MidiDial* dial = static_cast<MidiDial*>(lv_event_get_user_data(e));
    
    if (dial) {
        // Get the new value from the arc widget
        int new_value = lv_arc_get_value(arc);
        
        // Update our internal state
        dial->current_value_ = new_value;
        
        // Update the text display
        char text[8];
        snprintf(text, sizeof(text), "%03d", new_value);
        lv_label_set_text(dial->value_label_, text);
        
        // Trigger callback if set
        if (dial->value_callback_) {
            dial->value_callback_(new_value);
        }
        
        std::cout << "MIDI Dial dragged! New value: " << new_value << std::endl;
    }
}
