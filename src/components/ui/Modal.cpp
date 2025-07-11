#include "Modal.h"
#include "Constants.h"
#include <iostream>

Modal::Modal()
    : overlay_(nullptr)
    , modal_container_(nullptr)
    , title_bar_(nullptr)
    , title_label_(nullptr)
    , close_btn_(nullptr)
    , content_area_(nullptr)
    , is_visible_(false)
{
}

void Modal::create(lv_obj_t* parent) {
    if (overlay_) return; // Already created

    createOverlay(parent);
    createModalContainer();
    
    if (!config_.title.empty()) {
        createTitleBar();
    }
    
    createContentArea();
    
    if (config_.show_close_button) {
        createCloseButton();
    }

    updateStyling();
    // Don't update size/position here - do it in show() when parent has final size

    // Initially hidden
    lv_obj_add_flag(overlay_, LV_OBJ_FLAG_HIDDEN);
    is_visible_ = false;

    std::cout << "Modal created (" << config_.width_percent << "% x " 
              << config_.height_percent << "%) - size/position will be set on show()" << std::endl;
}

void Modal::destroy() {
    if (overlay_) {
        lv_obj_del(overlay_);
        overlay_ = nullptr;
        modal_container_ = nullptr;
        title_bar_ = nullptr;
        title_label_ = nullptr;
        close_btn_ = nullptr;
        content_area_ = nullptr;
        is_visible_ = false;
    }
}

void Modal::createOverlay(lv_obj_t* parent) {
    // Create overlay background that fills the parent container
    overlay_ = lv_obj_create(parent);
    lv_obj_set_size(overlay_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(overlay_, 0, 0);
    
    // Style the overlay
    lv_obj_set_style_bg_color(overlay_, lv_color_hex(config_.background_color), 0);
    lv_obj_set_style_bg_opa(overlay_, config_.background_opacity, 0);
    lv_obj_set_style_border_width(overlay_, 0, 0);
    lv_obj_set_style_pad_all(overlay_, 0, 0);

    // Handle background clicks
    if (config_.close_on_background_click) {
        lv_obj_add_event_cb(overlay_, onOverlayClicked, LV_EVENT_CLICKED, this);
    }

    // Handle escape key
    if (config_.close_on_escape) {
        lv_obj_add_event_cb(overlay_, onKeyPressed, LV_EVENT_KEY, this);
    }
}

void Modal::createModalContainer() {
    // Create the main modal container
    modal_container_ = lv_obj_create(overlay_);
    
    // Style the modal container
    lv_obj_set_style_bg_color(modal_container_, lv_color_hex(config_.modal_bg_color), 0);
    lv_obj_set_style_border_color(modal_container_, lv_color_hex(config_.modal_border_color), 0);
    lv_obj_set_style_border_width(modal_container_, 2, 0);
    lv_obj_set_style_radius(modal_container_, 8, 0);
    lv_obj_set_style_pad_all(modal_container_, 0, 0);

    // Set up flex layout for modal container
    lv_obj_set_layout(modal_container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(modal_container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(modal_container_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    
    std::cout << "Modal container created: " << modal_container_ << " with bg_color=" << std::hex << config_.modal_bg_color << std::dec << std::endl;
}

void Modal::createTitleBar() {
    if (config_.title.empty()) return;

    // Create title bar
    title_bar_ = lv_obj_create(modal_container_);
    lv_obj_set_size(title_bar_, LV_PCT(100), 18);
    lv_obj_set_style_bg_color(title_bar_, lv_color_hex(config_.modal_border_color), 0);
    lv_obj_set_style_border_width(title_bar_, 0, 0);
    lv_obj_set_style_radius(title_bar_, 0, 0);
    lv_obj_set_style_pad_all(title_bar_, 2, 0);

    // Create title label
    title_label_ = lv_label_create(title_bar_);
    lv_label_set_text(title_label_, config_.title.c_str());
    lv_obj_set_style_text_color(title_label_, lv_color_hex(config_.title_color), 0);
    lv_obj_align(title_label_, LV_ALIGN_CENTER, 0, 0);
}

void Modal::createContentArea() {
    // Create content area that takes up remaining space
    content_area_ = lv_obj_create(modal_container_);
    lv_obj_set_size(content_area_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(content_area_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content_area_, 0, 0);
    lv_obj_set_style_pad_all(content_area_, 16, 0);
    lv_obj_set_flex_grow(content_area_, 1); // Take remaining space

    std::cout << "Content area created: " << content_area_ << " size=100%x100%" << std::endl;

    // Don't create content here - wait until show() is called
    // This allows setContentCreator() to be called after create()
}

void Modal::createCloseButton() {
    if (!config_.show_close_button) return;

    // Create close button in the top-right corner
    close_btn_ = lv_btn_create(modal_container_);
    lv_obj_set_size(close_btn_, 20, 20);
    
    // Position close button
    if (title_bar_) {
        // Position in title bar
        lv_obj_set_parent(close_btn_, title_bar_);
        lv_obj_align(close_btn_, LV_ALIGN_RIGHT_MID, -8, 0);
    } else {
        // Position in top-right of modal
        lv_obj_align(close_btn_, LV_ALIGN_TOP_RIGHT, -16, 16);
    }

    // Style close button
    lv_obj_set_style_bg_color(close_btn_, lv_color_hex(0x800000), 0);
    lv_obj_set_style_border_color(close_btn_, lv_color_hex(0xFF6666), 0);
    lv_obj_set_style_border_width(close_btn_, 1, 0);
    lv_obj_set_style_radius(close_btn_, 20, 0); // Round button

    // Add close button label
    lv_obj_t* close_label = lv_label_create(close_btn_);
    lv_label_set_text(close_label, "X");
    lv_obj_set_style_text_color(close_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(close_label);

    // Add click event
    lv_obj_add_event_cb(close_btn_, onCloseButtonClicked, LV_EVENT_CLICKED, this);
}

void Modal::updateModalSize() {
    if (!modal_container_ || !overlay_) return;

    // Calculate modal size based on percentages of the overlay (parent) container
    lv_coord_t parent_width = lv_obj_get_width(overlay_);
    lv_coord_t parent_height = lv_obj_get_height(overlay_);
    
    lv_coord_t modal_width = (parent_width * config_.width_percent) / 100;
    lv_coord_t modal_height = (parent_height * config_.height_percent) / 100;
    
    std::cout << "Modal size calculation: parent=" << parent_width << "x" << parent_height 
              << " modal=" << modal_width << "x" << modal_height 
              << " (" << config_.width_percent << "% x " << config_.height_percent << "%)" << std::endl;
    
    lv_obj_set_size(modal_container_, modal_width, modal_height);
}

void Modal::updateStyling() {
    if (!overlay_ || !modal_container_) return;

    // Update overlay styling
    lv_obj_set_style_bg_color(overlay_, lv_color_hex(config_.background_color), 0);
    lv_obj_set_style_bg_opa(overlay_, config_.background_opacity, 0);

    // Update modal container styling
    lv_obj_set_style_bg_color(modal_container_, lv_color_hex(config_.modal_bg_color), 0);
    lv_obj_set_style_border_color(modal_container_, lv_color_hex(config_.modal_border_color), 0);

    // Update title styling if present
    if (title_label_) {
        lv_obj_set_style_text_color(title_label_, lv_color_hex(config_.title_color), 0);
    }
}

void Modal::centerModal() {
    if (!modal_container_) return;
    lv_obj_center(modal_container_);
    
    // Get position for debugging
    lv_coord_t x = lv_obj_get_x(modal_container_);
    lv_coord_t y = lv_obj_get_y(modal_container_);
    lv_coord_t w = lv_obj_get_width(modal_container_);
    lv_coord_t h = lv_obj_get_height(modal_container_);
    
    std::cout << "Modal centered at (" << x << "," << y << ") size=" << w << "x" << h << std::endl;
}

void Modal::setConfig(const ModalConfig& config) {
    config_ = config;
    
    if (overlay_) {
        updateStyling();
        updateModalSize();
        centerModal();
        
        // Update title if needed
        if (title_label_) {
            lv_label_set_text(title_label_, config_.title.c_str());
        } else if (!config_.title.empty() && modal_container_) {
            createTitleBar();
        }
    }
}

void Modal::setTitle(const std::string& title) {
    config_.title = title;
    if (title_label_) {
        lv_label_set_text(title_label_, title.c_str());
    }
}

void Modal::show() {
    if (!overlay_ || is_visible_) return;

    std::cout << "Modal::show() called - updating size and creating content..." << std::endl;

    // Update size and position when showing (parent container has final dimensions now)
    updateModalSize();
    centerModal();

    // Create content when showing (allows setContentCreator to be called after create)
    if (content_creator_ && content_area_) {
        std::cout << "Content creator found, clearing and creating content..." << std::endl;
        // Clear any existing content first
        clearContent();
        // Create the content
        content_creator_(content_area_);
        std::cout << "Content creation completed" << std::endl;
    } else {
        std::cout << "No content creator or content area! creator=" << (content_creator_ ? "YES" : "NO") 
                  << " area=" << (content_area_ ? "YES" : "NO") << std::endl;
    }

    lv_obj_clear_flag(overlay_, LV_OBJ_FLAG_HIDDEN);
    is_visible_ = true;

    if (on_open_callback_) {
        on_open_callback_();
    }

    std::cout << "Modal shown" << std::endl;
}

void Modal::hide() {
    if (!overlay_ || !is_visible_) return;

    lv_obj_add_flag(overlay_, LV_OBJ_FLAG_HIDDEN);
    is_visible_ = false;

    if (on_close_callback_) {
        on_close_callback_();
    }

    std::cout << "Modal hidden" << std::endl;
}

void Modal::setContent(lv_obj_t* content) {
    if (!content_area_) return;

    // Clear existing content
    clearContent();

    // Set new content parent
    lv_obj_set_parent(content, content_area_);
}

void Modal::clearContent() {
    if (!content_area_) return;

    // Clean all children
    lv_obj_clean(content_area_);
}

void Modal::onOverlayClicked(lv_event_t* e) {
    Modal* modal = static_cast<Modal*>(lv_event_get_user_data(e));
    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
    
    // Only close if clicked directly on overlay (not on modal container)
    if (modal && target == modal->overlay_) {
        modal->hide();
    }
}

void Modal::onCloseButtonClicked(lv_event_t* e) {
    Modal* modal = static_cast<Modal*>(lv_event_get_user_data(e));
    if (modal) {
        modal->hide();
    }
}

void Modal::onKeyPressed(lv_event_t* e) {
    Modal* modal = static_cast<Modal*>(lv_event_get_user_data(e));
    uint32_t key = lv_event_get_key(e);
    
    if (modal && key == LV_KEY_ESC) {
        modal->hide();
    }
}
