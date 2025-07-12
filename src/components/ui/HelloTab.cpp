#include "components/ui/HelloTab.h"
#include "components/ui/PagedView.h"
#include "components/ui/ContainerFactory.h"
#include "FontConfig.h"
#include "Constants.h"
#include <iostream>

HelloTab::HelloTab() 
    : Tab("Hello")
{
}

void HelloTab::create(lv_obj_t* parent) {
    if (container_) return; // Already created

    // Create main container for this tab using ContainerFactory
    container_ = UI::createContainer({
        .parent = parent,
        .width_pct = 98,
        .height_pct = 98,
        .align = LV_ALIGN_TOP_LEFT,
        .x_offset = 0,
        .y_offset = 0,
        .bg_color = lv_color_black(),
        .bg_opa = LV_OPA_COVER,
        .border_width = 0,
        .pad_all = 4,
        .use_bg_color = true
    });

    setContainer(container_);

    // Create the paged view
    paged_view_ = std::make_unique<PagedView>();
    paged_view_->create(container_);

    // Set up page change callback
    paged_view_->setPageChangedCallback([this](int old_page, int new_page) {
        onPageChanged(old_page, new_page);
    });

    // Style the paged view to match our theme
    paged_view_->setButtonStyle(
        SynthConstants::Color::BTN_FILTER_OFF, 
        SynthConstants::Color::BTN_FILTER_ON,
        0xFFFFFF
    );
    paged_view_->setIndicatorStyle(
        SynthConstants::Color::STATUS_BG,
        SynthConstants::Color::STATUS
    );

    setupPages();

    std::cout << "HelloTab created with PagedView demo" << std::endl;
}

void HelloTab::setupPages() {
    std::vector<PagedView::PageInfo> pages = {
        {
            "Welcome",
            createWelcomePage
        },
        {
            "Controls Demo", 
            createControlsPage
        },
        {
            "Information",
            createInfoPage
        },
        {
            "Settings",
            createSettingsPage
        }
    };

    paged_view_->setPages(pages);
    std::cout << "HelloTab: Set up " << pages.size() << " pages" << std::endl;
}

void HelloTab::onPageChanged(int old_page, int new_page) {
    std::cout << "HelloTab: Page changed from " << old_page << " to " << new_page << std::endl;
}

void HelloTab::createWelcomePage(lv_obj_t* parent) {
    // Create a centered welcome message
    lv_obj_t* welcome_container = lv_obj_create(parent);
    lv_obj_set_size(welcome_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(welcome_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(welcome_container, 0, 0);

    lv_obj_t* title = lv_label_create(welcome_container);
    lv_label_set_text(title, "Welcome to PagedView!");
    lv_obj_set_style_text_color(title, lv_color_hex(SynthConstants::Color::TITLE), 0);
    lv_obj_set_style_text_font(title, FontA.lg, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t* subtitle = lv_label_create(welcome_container);
    lv_label_set_text(subtitle, "This demonstrates our new paging component");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(SynthConstants::Color::HELP), 0);
    lv_obj_set_style_text_font(subtitle, FontA.med, 0);
    lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t* instructions = lv_label_create(welcome_container);
    lv_label_set_text(instructions, "Use < and > buttons to navigate between pages");
    lv_obj_set_style_text_color(instructions, lv_color_hex(SynthConstants::Color::STATUS), 0);
    lv_obj_set_style_text_font(instructions, FontA.small, 0);
    lv_obj_align(instructions, LV_ALIGN_CENTER, 0, 20);
}

void HelloTab::createControlsPage(lv_obj_t* parent) {
    // Create some sample controls to show variety
    lv_obj_t* controls_container = lv_obj_create(parent);
    lv_obj_set_size(controls_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(controls_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(controls_container, 0, 0);
    lv_obj_set_style_pad_all(controls_container, 10, 0);

    // Set up flex layout
    lv_obj_set_layout(controls_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(controls_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(controls_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    // Page title
    lv_obj_t* title = lv_label_create(controls_container);
    lv_label_set_text(title, "Sample Controls");
    lv_obj_set_style_text_color(title, lv_color_hex(SynthConstants::Color::TITLE), 0);
    lv_obj_set_style_text_font(title, FontA.med, 0);

    // Sample button
    lv_obj_t* sample_btn = lv_btn_create(controls_container);
    lv_obj_set_size(sample_btn, 120, 40);
    lv_obj_set_style_bg_color(sample_btn, lv_color_hex(SynthConstants::Color::BTN_FILTER_OFF), 0);
    lv_obj_set_style_border_color(sample_btn, lv_color_hex(SynthConstants::Color::BTN_FILTER_ON), 0);
    lv_obj_set_style_border_width(sample_btn, 2, 0);

    lv_obj_t* btn_label = lv_label_create(sample_btn);
    lv_label_set_text(btn_label, "Sample Button");
    lv_obj_set_style_text_font(btn_label, FontA.small, 0);
    lv_obj_center(btn_label);

    // Sample slider
    lv_obj_t* slider_label = lv_label_create(controls_container);
    lv_label_set_text(slider_label, "Sample Slider:");
    lv_obj_set_style_text_color(slider_label, lv_color_hex(SynthConstants::Color::HELP), 0);
    lv_obj_set_style_text_font(slider_label, FontA.small, 0);

    lv_obj_t* sample_slider = lv_slider_create(controls_container);
    lv_obj_set_size(sample_slider, 200, 20);
    lv_obj_set_style_bg_color(sample_slider, lv_color_hex(0x404040), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sample_slider, lv_color_hex(SynthConstants::Color::DIAL_GREEN), LV_PART_INDICATOR);
    lv_slider_set_value(sample_slider, 50, LV_ANIM_OFF);
}

void HelloTab::createInfoPage(lv_obj_t* parent) {
    // Create an information display
    lv_obj_t* info_container = lv_obj_create(parent);
    lv_obj_set_size(info_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(info_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(info_container, 0, 0);
    lv_obj_set_style_pad_all(info_container, 10, 0);

    // Set up flex layout
    lv_obj_set_layout(info_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(info_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    // Page title
    lv_obj_t* title = lv_label_create(info_container);
    lv_label_set_text(title, "Component Information");
    lv_obj_set_style_text_color(title, lv_color_hex(SynthConstants::Color::TITLE), 0);
    lv_obj_set_style_text_font(title, FontA.med, 0);

    // Information items
    const char* info_items[] = {
        "• PagedView Component: Reusable paging system",
        "• Navigation: < and > buttons for page switching", 
        "• Indicator: Shows current page (e.g., '2 of 4')",
        "• Content: Each page can have custom content",
        "• Callbacks: Page change events available",
        "• Styling: Configurable colors and appearance"
    };

    for (size_t i = 0; i < sizeof(info_items) / sizeof(info_items[0]); ++i) {
        lv_obj_t* info_item = lv_label_create(info_container);
        lv_label_set_text(info_item, info_items[i]);
        lv_obj_set_style_text_color(info_item, lv_color_hex(SynthConstants::Color::HELP), 0);
        lv_obj_set_style_text_font(info_item, FontA.small, 0);
    }
}

void HelloTab::createSettingsPage(lv_obj_t* parent) {
    // Create a settings-like page
    lv_obj_t* settings_container = lv_obj_create(parent);
    lv_obj_set_size(settings_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(settings_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(settings_container, 0, 0);
    lv_obj_set_style_pad_all(settings_container, 10, 0);

    // Set up flex layout
    lv_obj_set_layout(settings_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(settings_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(settings_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    // Page title
    lv_obj_t* title = lv_label_create(settings_container);
    lv_label_set_text(title, "Demo Settings");
    lv_obj_set_style_text_color(title, lv_color_hex(SynthConstants::Color::TITLE), 0);
    lv_obj_set_style_text_font(title, FontA.med, 0);

    // Sample setting items
    lv_obj_t* setting1 = lv_label_create(settings_container);
    lv_label_set_text(setting1, "Setting 1: Enabled");
    lv_obj_set_style_text_color(setting1, lv_color_hex(SynthConstants::Color::STATUS), 0);
    lv_obj_set_style_text_font(setting1, FontA.small, 0);

    lv_obj_t* setting2 = lv_label_create(settings_container);
    lv_label_set_text(setting2, "Setting 2: Medium");
    lv_obj_set_style_text_color(setting2, lv_color_hex(SynthConstants::Color::STATUS), 0);
    lv_obj_set_style_text_font(setting2, FontA.small, 0);

    lv_obj_t* setting3 = lv_label_create(settings_container);
    lv_label_set_text(setting3, "Setting 3: Auto");
    lv_obj_set_style_text_color(setting3, lv_color_hex(SynthConstants::Color::STATUS), 0);
    lv_obj_set_style_text_font(setting3, FontA.small, 0);

    lv_obj_t* note = lv_label_create(settings_container);
    lv_label_set_text(note, "Note: This is just a demonstration page");
    lv_obj_set_style_text_color(note, lv_color_hex(SynthConstants::Color::HELP), 0);
    lv_obj_set_style_text_font(note, FontA.small, 0);
}

void HelloTab::onActivated() {
    std::cout << "HelloTab activated - PagedView demo ready" << std::endl;
}

void HelloTab::onDeactivated() {
    std::cout << "HelloTab deactivated" << std::endl;
}
