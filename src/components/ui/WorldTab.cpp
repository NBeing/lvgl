#include "components/ui/WorldTab.h"
#include "FontConfig.h"
#include "Constants.h"
#include <iostream>

WorldTab::WorldTab()
    : Tab("World")
    , world_label_(nullptr)
{
}

void WorldTab::create(lv_obj_t* parent) {
    if (container_) return;  // Already created

    // Create main container for this tab
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(98), LV_PCT(98));  // 98% to prevent overflow
    lv_obj_set_pos(container_, 0, 0);
    lv_obj_set_style_bg_color(container_, lv_color_hex(SynthConstants::Color::BG), 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 5, 0);  // Minimal padding

    setContainer(container_);

    // Create "World" label in the center
    world_label_ = lv_label_create(container_);
    lv_label_set_text(world_label_, "World");
    lv_obj_set_style_text_color(world_label_, lv_color_hex(0xFFAAAA), 0);  // Different color
    lv_obj_set_style_text_font(world_label_, FontA.lg, 0);
    lv_obj_center(world_label_);

    std::cout << "WorldTab created" << std::endl;
}

void WorldTab::onActivated() {
    std::cout << "WorldTab activated" << std::endl;
}

void WorldTab::onDeactivated() {
    std::cout << "WorldTab deactivated" << std::endl;
}
