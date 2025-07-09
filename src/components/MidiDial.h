#pragma once

#include <lvgl.h>
#include <functional>
#include <unordered_map>
#include <memory>
#include <iostream>

class MidiDial {
public:
    MidiDial(lv_obj_t* parent, const char* label = "", int x = 0, int y = 0);
    ~MidiDial();

    void setValue(int value);
    int getValue() const;
    void setRange(int min_val, int max_val);
    void setColor(lv_color_t color);
    void setPosition(int x, int y);
    void onValueChanged(std::function<void(int)> callback);
    lv_obj_t* getObject();
    int getMidiCC() const;
    void setMidiCC(int cc_number);
    lv_obj_t* getLvObj() const;

private:
    lv_obj_t* container_;
    lv_obj_t* value_label_;
    lv_obj_t* name_label_;
    lv_obj_t* arc_display_;

    int current_value_;
    int min_value_;
    int max_value_;
    int midi_cc_;
    lv_color_t arc_color_;
    std::function<void(int)> value_callback_;

    void updateDisplay();
    void setupStyling();
    static void click_event_cb(lv_event_t* e);
    static void arc_event_cb(lv_event_t* e);
    
};