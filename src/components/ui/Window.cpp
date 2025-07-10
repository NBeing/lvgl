#include "components/ui/Window.h"
#include "Constants.h"
#include <iostream>

// ============================================================================
// Window Base Class
// ============================================================================

Window::Window(const std::string& name, Type type)
    : name_(name)
    , type_(type)
    , container_(nullptr)
    , is_visible_(false)
    , is_created_(false)
{
}

Window::~Window() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
    }
}

void Window::show() {
    if (!container_) {
        std::cout << "Warning: Cannot show window '" << name_ << "' - not created yet" << std::endl;
        return;
    }
    
    if (!is_visible_) {
        lv_obj_clear_flag(container_, LV_OBJ_FLAG_HIDDEN);
        is_visible_ = true;
        onActivated();
        std::cout << "Window '" << name_ << "' shown" << std::endl;
    }
}

void Window::hide() {
    if (container_ && is_visible_) {
        lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
        is_visible_ = false;
        onDeactivated();
        std::cout << "Window '" << name_ << "' hidden" << std::endl;
    }
}

void Window::setContainer(lv_obj_t* container) {
    container_ = container;
    is_created_ = (container != nullptr);
    if (container_) {
        // Start hidden by default
        lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
        is_visible_ = false;
    }
}

// ============================================================================
// Tab Class
// ============================================================================

Tab::Tab(const std::string& name)
    : Window(name, Type::TAB)
    , tab_button_(nullptr)
    , is_active_(false)
{
}

void Tab::setActive(bool active) {
    is_active_ = active;
    
    if (tab_button_) {
        if (active) {
            // Highlight active tab button
            lv_obj_set_style_bg_color(tab_button_, lv_color_hex(SynthConstants::Color::BTN_FILTER_ON), 0);
            lv_obj_set_style_text_color(tab_button_, lv_color_hex(0xFFFFFF), 0);
        } else {
            // Normal tab button appearance
            lv_obj_set_style_bg_color(tab_button_, lv_color_hex(SynthConstants::Color::BTN_FILTER_OFF), 0);
            lv_obj_set_style_text_color(tab_button_, lv_color_hex(0xCCCCCC), 0);
        }
    }
    
    if (active) {
        show();
    } else {
        hide();
    }
}

// ============================================================================
// PopupWindow Class
// ============================================================================

PopupWindow::PopupWindow(const std::string& name, bool is_modal)
    : Window(name, Type::POPUP)
    , is_modal_(is_modal)
    , backdrop_(nullptr)
{
}

void PopupWindow::show() {
    if (is_modal_ && !backdrop_) {
        createBackdrop(lv_scr_act());  // Create backdrop on active screen
    }
    
    Window::show();  // Call base implementation
}

void PopupWindow::hide() {
    Window::hide();  // Call base implementation
    
    if (backdrop_) {
        destroyBackdrop();
    }
}

void PopupWindow::createBackdrop(lv_obj_t* parent) {
    if (backdrop_) return;  // Already exists
    
    backdrop_ = lv_obj_create(parent);
    lv_obj_set_size(backdrop_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(backdrop_, 0, 0);
    lv_obj_set_style_bg_color(backdrop_, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(backdrop_, LV_OPA_50, 0);  // 50% transparent
    lv_obj_set_style_border_width(backdrop_, 0, 0);
    
    // Make backdrop clickable to prevent interaction with background
    lv_obj_add_flag(backdrop_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(backdrop_, LV_OBJ_FLAG_SCROLLABLE);
}

void PopupWindow::destroyBackdrop() {
    if (backdrop_) {
        lv_obj_del(backdrop_);
        backdrop_ = nullptr;
    }
}
