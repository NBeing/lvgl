#include "components/ui/HelloTab.h"
#include "FontConfig.h"
#include "Constants.h"
#include <iostream>

HelloTab::HelloTab()
    : Tab("Hello")
    , hello_label_(nullptr)
{
}

void HelloTab::create(lv_obj_t* parent) {
    if (container_) return;  // Already created

    // Create main container for this tab
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(98), LV_PCT(98));  // 98% to prevent overflow
    lv_obj_set_pos(container_, 0, 0);
    lv_obj_set_style_bg_color(container_, lv_color_hex(SynthConstants::Color::BG), 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 5, 0);  // Minimal padding

    setContainer(container_);

    // Create "Hello" label in the center
    hello_label_ = lv_label_create(container_);
    lv_label_set_text(hello_label_, "Hello");
    lv_obj_set_style_text_color(hello_label_, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(hello_label_, FontA.lg, 0);
    lv_obj_center(hello_label_);

    std::cout << "HelloTab created" << std::endl;
}

void HelloTab::onActivated() {
    std::cout << "HelloTab activated" << std::endl;
}

void HelloTab::onDeactivated() {
    std::cout << "HelloTab deactivated" << std::endl;
}
