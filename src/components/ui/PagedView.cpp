#include "PagedView.h"
#include "components/layout/LayoutManager.h"
#include "Constants.h"
#include <iostream>

PagedView::PagedView()
    : container_(nullptr)
    , nav_bar_(nullptr)
    , prev_btn_(nullptr)
    , next_btn_(nullptr)
    , page_indicator_(nullptr)
    , content_area_(nullptr)
    , current_page_(0)
{
}

void PagedView::create(lv_obj_t* parent) {
    if (container_) return; // Already created

    // Create main container
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 0, 0);

    // Set up flex layout (column)
    lv_obj_set_layout(container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    createNavigationBar();
    createContentArea();

    // Initialize with first page if available
    if (!pages_.empty()) {
        updatePageContent();
        updatePageIndicator();
        updateNavigationButtons();
    }

    std::cout << "PagedView created with " << pages_.size() << " pages" << std::endl;
}

void PagedView::destroy() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
        nav_bar_ = nullptr;
        prev_btn_ = nullptr;
        next_btn_ = nullptr;
        page_indicator_ = nullptr;
        content_area_ = nullptr;
    }
}

void PagedView::createNavigationBar() {
    // Create navigation bar at the top
    nav_bar_ = lv_obj_create(container_);
    lv_obj_set_size(nav_bar_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(nav_bar_, lv_color_hex(style_.indicator_bg_color), 0);
    lv_obj_set_style_border_width(nav_bar_, 1, 0);
    lv_obj_set_style_border_color(nav_bar_, lv_color_hex(0x606060), 0);
    lv_obj_set_style_radius(nav_bar_, 4, 0);
    lv_obj_set_style_pad_all(nav_bar_, 8, 0);

    // Set up flex layout for navigation bar
    lv_obj_set_layout(nav_bar_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(nav_bar_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav_bar_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    const auto& layout = LayoutManager::getConfig();

    // Previous button
    prev_btn_ = lv_btn_create(nav_bar_);
    lv_obj_set_size(prev_btn_, layout.button_height, layout.button_height); // Square button
    lv_obj_set_style_bg_color(prev_btn_, lv_color_hex(style_.button_bg_color), 0);
    lv_obj_set_style_border_color(prev_btn_, lv_color_hex(style_.button_border_color), 0);
    lv_obj_set_style_border_width(prev_btn_, 1, 0);
    lv_obj_set_style_radius(prev_btn_, 4, 0);

    lv_obj_t* prev_label = lv_label_create(prev_btn_);
    lv_label_set_text(prev_label, "<");
    lv_obj_set_style_text_color(prev_label, lv_color_hex(style_.button_text_color), 0);
    lv_obj_center(prev_label);

    lv_obj_add_event_cb(prev_btn_, onPreviousClicked, LV_EVENT_CLICKED, this);

    // Page indicator
    page_indicator_ = lv_label_create(nav_bar_);
    lv_label_set_text(page_indicator_, "1 of 1");
    lv_obj_set_style_text_color(page_indicator_, lv_color_hex(style_.indicator_text_color), 0);
    lv_obj_set_style_text_align(page_indicator_, LV_TEXT_ALIGN_CENTER, 0);

    // Next button
    next_btn_ = lv_btn_create(nav_bar_);
    lv_obj_set_size(next_btn_, layout.button_height, layout.button_height); // Square button
    lv_obj_set_style_bg_color(next_btn_, lv_color_hex(style_.button_bg_color), 0);
    lv_obj_set_style_border_color(next_btn_, lv_color_hex(style_.button_border_color), 0);
    lv_obj_set_style_border_width(next_btn_, 1, 0);
    lv_obj_set_style_radius(next_btn_, 4, 0);

    lv_obj_t* next_label = lv_label_create(next_btn_);
    lv_label_set_text(next_label, ">");
    lv_obj_set_style_text_color(next_label, lv_color_hex(style_.button_text_color), 0);
    lv_obj_center(next_label);

    lv_obj_add_event_cb(next_btn_, onNextClicked, LV_EVENT_CLICKED, this);
}

void PagedView::createContentArea() {
    // Create content area that takes up remaining space
    content_area_ = lv_obj_create(container_);
    lv_obj_set_size(content_area_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(content_area_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content_area_, 0, 0);
    lv_obj_set_style_pad_all(content_area_, 8, 0);
    lv_obj_set_flex_grow(content_area_, 1); // Take remaining space
}

void PagedView::addPage(const PageInfo& page_info) {
    pages_.push_back(page_info);
    
    // If this is the first page and we're already created, update the display
    if (pages_.size() == 1 && container_) {
        updatePageContent();
        updatePageIndicator();
        updateNavigationButtons();
    }
}

void PagedView::setPages(const std::vector<PageInfo>& pages) {
    pages_ = pages;
    current_page_ = 0;
    
    if (container_) {
        updatePageContent();
        updatePageIndicator();
        updateNavigationButtons();
    }
}

void PagedView::goToPage(int page_index) {
    if (page_index < 0 || page_index >= static_cast<int>(pages_.size())) {
        return; // Invalid page index
    }

    int old_page = current_page_;
    current_page_ = page_index;

    updatePageContent();
    updatePageIndicator();
    updateNavigationButtons();

    if (page_changed_callback_) {
        page_changed_callback_(old_page, current_page_);
    }

    std::cout << "PagedView: Changed from page " << old_page << " to page " << current_page_ << std::endl;
}

void PagedView::nextPage() {
    if (current_page_ < static_cast<int>(pages_.size()) - 1) {
        goToPage(current_page_ + 1);
    }
}

void PagedView::previousPage() {
    if (current_page_ > 0) {
        goToPage(current_page_ - 1);
    }
}

void PagedView::updatePageContent() {
    if (!content_area_ || pages_.empty() || current_page_ < 0 || current_page_ >= static_cast<int>(pages_.size())) {
        return;
    }

    // Clear existing content
    clearCurrentPageContent();

    // Create new content for current page
    const auto& current_page_info = pages_[current_page_];
    if (current_page_info.create_content) {
        current_page_info.create_content(content_area_);
    }
}

void PagedView::updatePageIndicator() {
    if (!page_indicator_ || pages_.empty()) {
        return;
    }

    char indicator_text[64];
    snprintf(indicator_text, sizeof(indicator_text), "%d of %d", 
             current_page_ + 1, static_cast<int>(pages_.size()));
    lv_label_set_text(page_indicator_, indicator_text);
}

void PagedView::updateNavigationButtons() {
    if (!prev_btn_ || !next_btn_) {
        return;
    }

    // Enable/disable previous button
    bool can_go_prev = current_page_ > 0;
    lv_obj_set_style_bg_opa(prev_btn_, can_go_prev ? LV_OPA_COVER : LV_OPA_50, 0);
    
    // Enable/disable next button  
    bool can_go_next = current_page_ < static_cast<int>(pages_.size()) - 1;
    lv_obj_set_style_bg_opa(next_btn_, can_go_next ? LV_OPA_COVER : LV_OPA_50, 0);
}

void PagedView::clearCurrentPageContent() {
    if (!content_area_) return;

    // Delete all children of content area
    lv_obj_clean(content_area_);
}

void PagedView::setButtonStyle(uint32_t bg_color, uint32_t border_color, uint32_t text_color) {
    style_.button_bg_color = bg_color;
    style_.button_border_color = border_color;
    style_.button_text_color = text_color;

    // Apply styles if buttons exist
    if (prev_btn_) {
        lv_obj_set_style_bg_color(prev_btn_, lv_color_hex(bg_color), 0);
        lv_obj_set_style_border_color(prev_btn_, lv_color_hex(border_color), 0);
    }
    if (next_btn_) {
        lv_obj_set_style_bg_color(next_btn_, lv_color_hex(bg_color), 0);
        lv_obj_set_style_border_color(next_btn_, lv_color_hex(border_color), 0);
    }
}

void PagedView::setIndicatorStyle(uint32_t bg_color, uint32_t text_color) {
    style_.indicator_bg_color = bg_color;
    style_.indicator_text_color = text_color;

    // Apply styles if elements exist
    if (nav_bar_) {
        lv_obj_set_style_bg_color(nav_bar_, lv_color_hex(bg_color), 0);
    }
    if (page_indicator_) {
        lv_obj_set_style_text_color(page_indicator_, lv_color_hex(text_color), 0);
    }
}

void PagedView::onPreviousClicked(lv_event_t* e) {
    PagedView* paged_view = static_cast<PagedView*>(lv_event_get_user_data(e));
    if (paged_view) {
        paged_view->previousPage();
    }
}

void PagedView::onNextClicked(lv_event_t* e) {
    PagedView* paged_view = static_cast<PagedView*>(lv_event_get_user_data(e));
    if (paged_view) {
        paged_view->nextPage();
    }
}
