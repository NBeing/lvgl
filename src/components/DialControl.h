#pragma once

#include "Parameter.h"
#include "ParameterControl.h"
#include <lvgl.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <string>
/**
 * @brief Parameter-aware dial control implementation
 * 
 * This class wraps the existing MidiDial functionality with parameter awareness.
 * It automatically displays parameter names, handles value scaling, and integrates
 * with the observer pattern for synchronized updates.
 */
class DialControl : public ParameterControl {
public:
    DialControl(lv_obj_t* parent, int x = 0, int y = 0);
    ~DialControl();
    
    // ParameterControl implementation
    void setPosition(int x, int y) override;
    void getPosition(int& x, int& y) const override;
    void setSize(int width, int height) override;
    void getSize(int& width, int& height) const override;
    lv_obj_t* getObject() override;
    
    // Dial-specific customization
    void setColor(lv_color_t color);
    lv_color_t getColor() const;
    void setDialSize(int diameter);
    int getDialSize() const;
    
    // Manual label override (if not using parameter name)
    void setLabel(const std::string& label);
    std::string getLabel() const;
    
protected:
    // ParameterControl implementation
    void updateDisplayFromParameter() override;
    void updateParameterFromControl(uint8_t value) override;
    void onParameterBound() override;
    void onParameterUnbound() override;
    void onEnabledChanged(bool enabled) override;
    void onVisibilityChanged(bool visible) override;
    
private:
    // LVGL components
    lv_obj_t* container_;
    lv_obj_t* name_label_;
    lv_obj_t* value_label_;
    lv_obj_t* arc_display_;
    
    // Properties
    lv_color_t arc_color_;
    int dial_diameter_;
    std::string custom_label_;
    bool use_custom_label_;
    
    // Internal state
    uint8_t display_value_;
    
    // Setup and styling
    void createWidgets(lv_obj_t* parent, int x, int y);
    void setupStyling();
    void updateLabels();
    void updateArcDisplay();
    
    // Event handlers
    static void arc_event_cb(lv_event_t* e);
    static void container_click_cb(lv_event_t* e);
    
    // Static mapping for event callbacks
    static std::unordered_map<lv_obj_t*, DialControl*> dial_control_map_;
};
