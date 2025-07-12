#include "components/ui/WorldTab.h"
#include "FontConfig.h"
#include "Constants.h"
#include "components/ui/ContainerFactory.h"

#include <iostream>

WorldTab::WorldTab()
    : Tab("World")
    , buttons_container_(nullptr)
    , simple_modal_btn_(nullptr)
    , config_modal_btn_(nullptr)
    , info_modal_btn_(nullptr)
{
}

void WorldTab::create(lv_obj_t* parent) {
    if (container_) return; // Already created

    // Create main container for this tab using ContainerFactory
    container_ = UI::createContainer({
        .parent = parent,
        .width_pct = 98,
        .height_pct = 98,
        .align = LV_ALIGN_TOP_LEFT,
        .x_offset = 0,
        .y_offset = 0,
        .bg_color = lv_color_hex(SynthConstants::Color::BG),
        .bg_opa = LV_OPA_COVER,
        .border_width = 0,
        .pad_all = 8
    });

    
    setContainer(container_);

    // Create title
    lv_obj_t* title = lv_label_create(container_);
    lv_label_set_text(title, "Modal Component Demo");
    lv_obj_set_style_text_color(title, lv_color_hex(SynthConstants::Color::TITLE), 0);
    lv_obj_set_style_text_font(title, FontA.med, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Create subtitle
    lv_obj_t* subtitle = lv_label_create(container_);
    lv_label_set_text(subtitle, "Click buttons below to test different modal configurations");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(SynthConstants::Color::HELP), 0);
    lv_obj_set_style_text_font(subtitle, FontA.small, 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 50);

    createModalButtons();
    setupModals();

    std::cout << "WorldTab created with Modal demos" << std::endl;
}

void WorldTab::createModalButtons() {
    // Create container for buttons using ContainerFactory
    buttons_container_ = UI::createContainer({
        .parent = container_,
        .width_pct = 90,
        .height_pct = 0, // LV_SIZE_CONTENT
        .align = LV_ALIGN_CENTER,
        .x_offset = 0,
        .y_offset = 20,
        .bg_opa = LV_OPA_TRANSP,
        .border_width = 0,
        .pad_all = 10
    });
    lv_obj_set_height(buttons_container_, LV_SIZE_CONTENT);
    lv_obj_set_layout(buttons_container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(buttons_container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(buttons_container_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(buttons_container_, 15, 0);

    // Simple Modal Button
    simple_modal_btn_ = lv_btn_create(buttons_container_);
    lv_obj_set_size(simple_modal_btn_, 200, 50);
    lv_obj_set_style_bg_color(simple_modal_btn_, lv_color_hex(SynthConstants::Color::BTN_FILTER_OFF), 0);
    lv_obj_set_style_border_color(simple_modal_btn_, lv_color_hex(SynthConstants::Color::BTN_FILTER_ON), 0);
    lv_obj_set_style_border_width(simple_modal_btn_, 2, 0);
    lv_obj_set_style_radius(simple_modal_btn_, 6, 0);

    lv_obj_t* simple_label = lv_label_create(simple_modal_btn_);
    lv_label_set_text(simple_label, "Open Simple Modal");
    lv_obj_set_style_text_font(simple_label, FontA.small, 0);
    lv_obj_center(simple_label);

    lv_obj_add_event_cb(simple_modal_btn_, [](lv_event_t* e) {
        WorldTab* tab = static_cast<WorldTab*>(lv_event_get_user_data(e));
        if (tab && tab->simple_modal_) {
            tab->simple_modal_->show();
        }
    }, LV_EVENT_CLICKED, this);

    // Config Modal Button
    config_modal_btn_ = lv_btn_create(buttons_container_);
    lv_obj_set_size(config_modal_btn_, 200, 50);
    lv_obj_set_style_bg_color(config_modal_btn_, lv_color_hex(SynthConstants::Color::DIAL_ORANGE), 0);
    lv_obj_set_style_border_color(config_modal_btn_, lv_color_hex(0xFFAA55), 0);
    lv_obj_set_style_border_width(config_modal_btn_, 2, 0);
    lv_obj_set_style_radius(config_modal_btn_, 6, 0);

    lv_obj_t* config_label = lv_label_create(config_modal_btn_);
    lv_label_set_text(config_label, "Open Config Modal");
    lv_obj_set_style_text_font(config_label, FontA.small, 0);
    lv_obj_center(config_label);

    lv_obj_add_event_cb(config_modal_btn_, [](lv_event_t* e) {
        WorldTab* tab = static_cast<WorldTab*>(lv_event_get_user_data(e));
        if (tab && tab->config_modal_) {
            tab->config_modal_->show();
        }
    }, LV_EVENT_CLICKED, this);

    // Info Modal Button
    info_modal_btn_ = lv_btn_create(buttons_container_);
    lv_obj_set_size(info_modal_btn_, 200, 50);
    lv_obj_set_style_bg_color(info_modal_btn_, lv_color_hex(SynthConstants::Color::DIAL_BLUE), 0);
    lv_obj_set_style_border_color(info_modal_btn_, lv_color_hex(0x6666FF), 0);
    lv_obj_set_style_border_width(info_modal_btn_, 2, 0);
    lv_obj_set_style_radius(info_modal_btn_, 6, 0);

    lv_obj_t* info_label = lv_label_create(info_modal_btn_);
    lv_label_set_text(info_label, "Open Info Modal");
    lv_obj_set_style_text_font(info_label, FontA.small, 0);
    lv_obj_center(info_label);

    lv_obj_add_event_cb(info_modal_btn_, [](lv_event_t* e) {
        WorldTab* tab = static_cast<WorldTab*>(lv_event_get_user_data(e));
        if (tab && tab->info_modal_) {
            tab->info_modal_->show();
        }
    }, LV_EVENT_CLICKED, this);
}

void WorldTab::setupModals() {
    // Simple Modal (default config)
    simple_modal_ = std::make_unique<Modal>();
    Modal::ModalConfig simple_config;
    simple_config.title = "Simple Modal";
    simple_config.width_percent = 70;
    simple_config.height_percent = 60;
    simple_modal_->setConfig(simple_config);
    simple_modal_->setContentCreator(createSimpleModalContent);
    simple_modal_->setOnOpenCallback([this]() { onSimpleModalOpen(); });
    simple_modal_->setOnCloseCallback([this]() { onSimpleModalClose(); });
    simple_modal_->create(container_);  // Use tab container instead of screen

    // Config Modal (custom styling)
    config_modal_ = std::make_unique<Modal>();
    Modal::ModalConfig config_config;
    config_config.title = "Settings & Configuration";
    config_config.width_percent = 85;
    config_config.height_percent = 80;
    config_config.modal_bg_color = 0x1A1A2E;
    config_config.modal_border_color = 0xFFAA55;
    config_config.title_color = 0xFFAA55;
    config_config.background_opacity = 200;
    config_modal_->setConfig(config_config);
    config_modal_->setContentCreator(createConfigModalContent);
    config_modal_->setOnOpenCallback([this]() { onConfigModalOpen(); });
    config_modal_->create(container_);  // Use tab container instead of screen

    // Info Modal (large, no close button)
    info_modal_ = std::make_unique<Modal>();
    Modal::ModalConfig info_config;
    info_config.title = "Component Information";
    info_config.width_percent = 95;
    info_config.height_percent = 90;
    info_config.show_close_button = false;  // No close button
    info_config.modal_bg_color = 0x0E2A47;
    info_config.modal_border_color = 0x6666FF;
    info_config.title_color = 0x6666FF;
    info_config.close_on_background_click = true;  // Close by clicking background
    info_modal_->setConfig(info_config);
    info_modal_->setContentCreator(createInfoModalContent);
    info_modal_->setOnOpenCallback([this]() { onInfoModalOpen(); });
    info_modal_->create(container_);  // Use tab container instead of screen
}

void WorldTab::createSimpleModalContent(lv_obj_t* parent) {
    std::cout << "WorldTab::createSimpleModalContent() called with parent=" << parent << std::endl;
    
    // Simple content with text and a button
    lv_obj_t* content_label = lv_label_create(parent);
    lv_label_set_text(content_label, 
        "This is a simple modal dialog!\n\n"
        "Features:\n"
        "• Overlay background\n"
        "• Close button (×)\n"
        "• Click outside to close\n"
        "• Escape key to close");
    lv_obj_set_style_text_color(content_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(content_label, FontA.small, 0);
    lv_obj_align(content_label, LV_ALIGN_TOP_LEFT, 0, 0);

    std::cout << "Created content label: " << content_label << std::endl;

    // Add a sample button
    lv_obj_t* sample_btn = lv_btn_create(parent);
    lv_obj_set_size(sample_btn, 60, 20);
    lv_obj_align(sample_btn, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(sample_btn, lv_color_hex(0x00AA00), 0);

    lv_obj_t* btn_label = lv_label_create(sample_btn);
    lv_label_set_text(btn_label, "Sample Button");
    lv_obj_set_style_text_font(btn_label, FontA.small, 0);
    lv_obj_center(btn_label);
    
    std::cout << "Created sample button: " << sample_btn << std::endl;
    std::cout << "WorldTab::createSimpleModalContent() completed" << std::endl;
}

void WorldTab::createConfigModalContent(lv_obj_t* parent) {
    // Configuration-style content
    lv_obj_t* settings_title = lv_label_create(parent);
    lv_label_set_text(settings_title, "Application Settings");
    lv_obj_set_style_text_color(settings_title, lv_color_hex(0xFFAA55), 0);
    lv_obj_set_style_text_font(settings_title, FontA.small, 0);
    lv_obj_align(settings_title, LV_ALIGN_TOP_MID, 0, 0);

    // Sample settings
    const char* settings[] = {
        "• Audio Sample Rate: 44.1kHz",
        "• MIDI Input Device: Connected",
        "• Display Brightness: 80%",
        "• Auto-save Presets: Enabled",
        "• Debug Mode: Disabled"
    };

    for (int i = 0; i < 5; ++i) {
        lv_obj_t* setting_item = lv_label_create(parent);
        lv_label_set_text(setting_item, settings[i]);
        lv_obj_set_style_text_color(setting_item, lv_color_hex(0xCCCCCC), 0);
        lv_obj_set_style_text_font(setting_item, FontA.small, 0);
        lv_obj_align(setting_item, LV_ALIGN_TOP_LEFT, 20, 40 + (i * 25));
    }

    // Save/Cancel buttons
    lv_obj_t* save_btn = lv_btn_create(parent);
    lv_obj_set_size(save_btn, 80, 35);
    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x00AA00), 0);

    lv_obj_t* save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "Save");
    lv_obj_center(save_label);

    lv_obj_t* cancel_btn = lv_btn_create(parent);
    lv_obj_set_size(cancel_btn, 80, 35);
    lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0xAA0000), 0);

    lv_obj_t* cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_center(cancel_label);
}

void WorldTab::createInfoModalContent(lv_obj_t* parent) {
    // Comprehensive information display
    lv_obj_t* info_content = lv_label_create(parent);
    lv_label_set_text(info_content,
        "Modal Component Features:\n\n"
        "Configuration Options:\n"
        "• Configurable width/height (percentage-based)\n"
        "• Custom background colors and opacity\n"
        "• Optional close button\n"
        "• Optional title bar\n"
        "• Background click to close\n"
        "• Escape key support\n\n"
        "Event System:\n"
        "• onOpen and onClose callbacks\n"
        "• Content creator functions\n"
        "• Flexible content management\n\n"
        "Usage Examples:\n"
        "• Settings dialogs\n"
        "• Information displays\n"
        "• Confirmation dialogs\n"
        "• Image/media viewers\n"
        "• Form overlays\n\n"
        "Click anywhere outside this modal to close it!");
    
    lv_obj_set_style_text_color(info_content, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(info_content, FontA.small, 0);
    lv_label_set_long_mode(info_content, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(info_content, LV_PCT(100));
    lv_obj_align(info_content, LV_ALIGN_TOP_LEFT, 0, 0);
}

void WorldTab::onSimpleModalOpen() {
    std::cout << "WorldTab: Simple modal opened" << std::endl;
}

void WorldTab::onSimpleModalClose() {
    std::cout << "WorldTab: Simple modal closed" << std::endl;
}

void WorldTab::onConfigModalOpen() {
    std::cout << "WorldTab: Config modal opened" << std::endl;
}

void WorldTab::onInfoModalOpen() {
    std::cout << "WorldTab: Info modal opened" << std::endl;
}

void WorldTab::onActivated() {
    std::cout << "WorldTab activated - Modal demos ready" << std::endl;
}

void WorldTab::onDeactivated() {
    std::cout << "WorldTab deactivated" << std::endl;
}
