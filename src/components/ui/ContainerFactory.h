
#pragma once
#include <lvgl.h>
#include "Constants.h"

namespace UI {


struct ContainerOptions {
    lv_obj_t* parent = nullptr;
    int width_pct = -1;
    int height_pct = -1;
    lv_align_t align = (lv_align_t)-1;
    int x_offset = 0;
    int y_offset = 0;
    lv_color_t bg_color = lv_color_hex(SynthConstants::Color::BG);
 // Use lv_color_t, default to {}
    lv_opa_t bg_opa = LV_OPA_COVER; // Use lv_opa_t, default to LVGL default
    int border_width = -1;
    int pad_all = -1;
    bool use_bg_color = true; // Add flags to check if set
    bool use_bg_opa = false;
};

static const ContainerOptions DefaultContainer = {
    .parent = nullptr,
    .width_pct = -1,
    .height_pct = -1,
    .align = (lv_align_t)-1,
    .x_offset = 0,
    .y_offset = 0,
    .bg_color = lv_color_black(),
    .bg_opa = LV_OPA_COVER,
    .border_width = -1,
    .pad_all = -1,
    .use_bg_color = true,
    .use_bg_opa = false
};

inline ContainerOptions mergeOptions(const ContainerOptions& base, const ContainerOptions& override) {
    ContainerOptions result = base;
    if (override.parent) result.parent = override.parent;
    if (override.width_pct != -1) result.width_pct = override.width_pct;
    if (override.height_pct != -1) result.height_pct = override.height_pct;
    if (override.align != (lv_align_t)-1) result.align = override.align;
    if (override.x_offset != 0) result.x_offset = override.x_offset;
    if (override.y_offset != 0) result.y_offset = override.y_offset;
    if (override.use_bg_color) {
        result.bg_color = override.bg_color;
        result.use_bg_color = true;
    }
    if (override.use_bg_opa) {
        result.bg_opa = override.bg_opa;
        result.use_bg_opa = true;
    }
    if (override.border_width != -1) result.border_width = override.border_width;
    if (override.pad_all != -1) result.pad_all = override.pad_all;
    return result;
}

inline lv_obj_t* createContainer(ContainerOptions opts) {
    lv_obj_t* cont = lv_obj_create(opts.parent);
    if (opts.width_pct >= 0 && opts.height_pct >= 0)
        lv_obj_set_size(cont, LV_PCT(opts.width_pct), LV_PCT(opts.height_pct));
    else if (opts.width_pct >= 0)
        lv_obj_set_width(cont, LV_PCT(opts.width_pct));
    else if (opts.height_pct >= 0)
        lv_obj_set_height(cont, LV_PCT(opts.height_pct));
    // else: use LVGL defaulte size

    if (opts.align != (lv_align_t)-1)
        lv_obj_align(cont, opts.align, opts.x_offset, opts.y_offset);
    if (opts.use_bg_color)
        lv_obj_set_style_bg_color(cont, opts.bg_color, 0);
    if (opts.use_bg_opa)
        lv_obj_set_style_bg_opa(cont, opts.bg_opa, 0);
    if (opts.border_width != -1)
        lv_obj_set_style_border_width(cont, opts.border_width, 0);
    if (opts.pad_all != -1)
        lv_obj_set_style_pad_all(cont, opts.pad_all, 0);
    return cont;
}

} // namespace UI
