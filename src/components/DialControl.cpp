#include <unordered_map>
#include <iostream>

#include "DialControl.h"
#include "ParameterControl.h"
#include "../../include/FontConfig.h"
#include <lvgl.h>

// Static map for dial control callbacks
std::unordered_map<lv_obj_t*, DialControl*> DialControl::dial_control_map_;

// ============================================================================
// DialControl Implementation
// ============================================================================

DialControl::DialControl(lv_obj_t* parent, int x, int y)
    : ParameterControl()
    , container_(nullptr)
    , name_label_(nullptr)
    , value_label_(nullptr)
    , arc_display_(nullptr)
    , arc_color_(lv_color_hex(0xFF00FF00))
    , dial_diameter_(50)
    , use_custom_label_(false)
    , display_value_(0)
{
    createWidgets(parent, x, y);
    setupStyling();
    updateLabels();
    updateArcDisplay();
}
DialControl::~DialControl() {
    if (container_) {
        dial_control_map_.erase(container_);
        dial_control_map_.erase(arc_display_);
    }
}

void DialControl::createWidgets(lv_obj_t* parent, int x, int y) {
    // Create main container
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, 80, 80);
    lv_obj_set_pos(container_, x, y);
    std::cout << "[DIAL] createWidgets: container_=" << container_ << ", parent=" << parent << std::endl;
    
    // Register for callbacks
    dial_control_map_[container_] = this;
    
    // Create parameter name label
    name_label_ = lv_label_create(container_);
    lv_obj_set_style_text_color(name_label_, lv_color_hex(0xFFCCCCCC), 0);
    lv_obj_set_style_text_font(name_label_, FontA.med, 0);
    lv_obj_align(name_label_, LV_ALIGN_TOP_MID, 0, 2);
    
    // Create arc display
    arc_display_ = lv_arc_create(container_);
    lv_obj_set_size(arc_display_, dial_diameter_, dial_diameter_);
    lv_obj_align(arc_display_, LV_ALIGN_CENTER, 0, -5);
    std::cout << "[DIAL] arc_display_ created: " << arc_display_ << ", dial_diameter_=" << dial_diameter_ << std::endl;
    
    // Register arc for callbacks
    dial_control_map_[arc_display_] = this;
    
    // Configure arc for semicircle style
    lv_arc_set_bg_angles(arc_display_, 180, 0);
    lv_arc_set_angles(arc_display_, 180, 180);
    lv_arc_set_range(arc_display_, 0, 127);  // Default MIDI range
    
    // Create value display label
    value_label_ = lv_label_create(container_);
    lv_obj_set_style_text_color(value_label_, arc_color_, 0);
    lv_obj_set_style_text_font(value_label_, FontA.lg, 0);
    lv_obj_align(value_label_, LV_ALIGN_BOTTOM_MID, 0, -2);
    
    // Set up event handlers
    lv_obj_add_event_cb(container_, container_click_cb, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(arc_display_, arc_event_cb, LV_EVENT_VALUE_CHANGED, this);
}

void DialControl::setupStyling() {
    // Container styling
    lv_obj_set_style_bg_color(container_, lv_color_hex(0xFF1a1a1a), 0);
    lv_obj_set_style_bg_opa(container_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(container_, lv_color_hex(0xFF666666), 0);
    lv_obj_set_style_border_width(container_, 2, 0);
    lv_obj_set_style_border_opa(container_, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(container_, 4, 0);
    lv_obj_set_style_pad_all(container_, 4, 0);
    
    // Arc styling
    lv_obj_set_style_arc_color(arc_display_, lv_color_hex(0xFF444444), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_display_, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_display_, arc_color_, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_display_, 6, LV_PART_INDICATOR);
    
    // Hide the knob for cleaner look
    lv_obj_set_style_bg_opa(arc_display_, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_opa(arc_display_, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc_display_, 0, LV_PART_KNOB);
}

void DialControl::setPosition(int x, int y) {
    if (container_) {
        lv_obj_set_pos(container_, x, y);
    }
}

void DialControl::getPosition(int& x, int& y) const {
    if (container_) {
        x = lv_obj_get_x(container_);
        y = lv_obj_get_y(container_);
    } else {
        x = y = 0;
    }
}

void DialControl::setSize(int width, int height) {
    if (container_) {
        lv_obj_set_size(container_, width, height);
        // Adjust arc size proportionally
        int arc_size = std::min(width - 20, height - 30);
        lv_obj_set_size(arc_display_, arc_size, arc_size);
        lv_obj_align(arc_display_, LV_ALIGN_CENTER, 0, -5);
        std::cout << "[DIAL] setSize: container_=" << container_ << " size=" << width << "x" << height << ", arc_display_=" << arc_display_ << " arc_size=" << arc_size << std::endl;
        std::cout << "[DIAL] actual container size: " << lv_obj_get_width(container_) << "x" << lv_obj_get_height(container_) << ", arc actual size: " << lv_obj_get_width(arc_display_) << "x" << lv_obj_get_height(arc_display_) << std::endl;
    }
}

void DialControl::getSize(int& width, int& height) const {
    if (container_) {
        width = lv_obj_get_width(container_);
        height = lv_obj_get_height(container_);
    } else {
        width = height = 0;
    }
}

lv_obj_t* DialControl::getObject() {
    return container_;
}

void DialControl::setColor(lv_color_t color) {
    arc_color_ = color;
    if (arc_display_) {
        lv_obj_set_style_arc_color(arc_display_, color, LV_PART_INDICATOR);
    }
    if (value_label_) {
        lv_obj_set_style_text_color(value_label_, color, 0);
    }
}

lv_color_t DialControl::getColor() const {
    return arc_color_;
}

void DialControl::setDialSize(int diameter) {
    dial_diameter_ = diameter;
    if (arc_display_) {
        lv_obj_set_size(arc_display_, diameter, diameter);
        lv_obj_align(arc_display_, LV_ALIGN_CENTER, 0, -5);
    }
}

int DialControl::getDialSize() const {
    return dial_diameter_;
}

void DialControl::setLabel(const std::string& label) {
    custom_label_ = label;
    use_custom_label_ = true;
    updateLabels();
}

std::string DialControl::getLabel() const {
    if (use_custom_label_) {
        return custom_label_;
    } else if (isParameterBound()) {
        return getBoundParameter()->getShortName();
    }
    return "N/A";
}

void DialControl::updateDisplayFromParameter() {
    if (!isParameterBound()) return;
    
    auto param = getBoundParameter();
    display_value_ = param->getCurrentValue();
    
    updateLabels();
    updateArcDisplay();
}

void DialControl::updateParameterFromControl(uint8_t value) {
    if (!isParameterBound() || isUpdatingFromParameter()) return;
    
    auto param = getBoundParameter();
    param->setValue(value);
    display_value_ = value;
    
    updateLabels();
    notifyValueChanged(value);
}

void DialControl::onParameterBound() {
    use_custom_label_ = false;  // Use parameter name by default
    
    if (isParameterBound()) {
        auto param = getBoundParameter();
        // Set arc range based on parameter
        lv_arc_set_range(arc_display_, param->getMinValue(), param->getMaxValue());
        display_value_ = param->getCurrentValue();
    }
    
    updateLabels();
    updateArcDisplay();
}

void DialControl::onParameterUnbound() {
    // Reset to default state
    lv_arc_set_range(arc_display_, 0, 127);
    display_value_ = 0;
    updateLabels();
    updateArcDisplay();
}

void DialControl::onEnabledChanged(bool enabled) {
    if (container_) {
        lv_obj_set_style_bg_opa(container_, enabled ? LV_OPA_COVER : LV_OPA_50, 0);
    }
    if (arc_display_) {
        lv_obj_set_style_arc_opa(arc_display_, enabled ? LV_OPA_COVER : LV_OPA_50, LV_PART_INDICATOR);
    }
}

void DialControl::onVisibilityChanged(bool visible) {
    if (container_) {
        if (visible) {
            lv_obj_clear_flag(container_, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void DialControl::updateLabels() {
    // Update name label
    std::string label_text = getLabel();
    if (label_text.length() > 8) {
        label_text = label_text.substr(0, 8);  // Truncate long names
    }
    lv_label_set_text(name_label_, label_text.c_str());
    
    // Update value label
    std::string value_text;
    if (isParameterBound()) {
        value_text = getBoundParameter()->getValueDisplayText();
    } else {
        value_text = std::to_string(display_value_);
    }
    
    // Pad with zeros for consistent display
    if (value_text.length() < 3 && value_text[0] != '+' && value_text[0] != '-') {
        value_text = std::string(3 - value_text.length(), '0') + value_text;
    }
    
    lv_label_set_text(value_label_, value_text.c_str());
}

void DialControl::updateArcDisplay() {
    if (arc_display_) {
        lv_arc_set_value(arc_display_, display_value_);
    }
}

// Static event handlers
void DialControl::arc_event_cb(lv_event_t* e) {
    lv_obj_t* arc = static_cast<lv_obj_t*>(lv_event_get_target(e));
    
    auto it = dial_control_map_.find(arc);
    if (it != dial_control_map_.end()) {
        DialControl* control = it->second;
        
        uint8_t new_value = static_cast<uint8_t>(lv_arc_get_value(arc));
        control->updateParameterFromControl(new_value);
        
        std::cout << "Dial arc changed: " << static_cast<int>(new_value) << std::endl;
    }
}

void DialControl::container_click_cb(lv_event_t* e) {
    lv_obj_t* container = static_cast<lv_obj_t*>(lv_event_get_target(e));
    
    auto it = dial_control_map_.find(container);
    if (it != dial_control_map_.end()) {
        DialControl* control = it->second;
        
        // Reset to default on click
        if (control->isParameterBound()) {
            auto param = control->getBoundParameter();
            param->resetToDefault();
            std::cout << "Dial reset to default: " << param->getName() << std::endl;
        }
    }
}
