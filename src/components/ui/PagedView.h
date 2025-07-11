#pragma once

#include <lvgl.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>

/**
 * @brief A reusable paging component that can display multiple pages of content
 * 
 * This component provides:
 * - Previous/Next navigation buttons
 * - Page indicator (e.g., "1 of 3")
 * - Content area for each page
 * - Smooth transitions between pages
 * - Event callbacks for page changes
 */
class PagedView {
public:
    using PageChangedCallback = std::function<void(int old_page, int new_page)>;

    struct PageInfo {
        std::string title;
        std::function<void(lv_obj_t*)> create_content;  // Function to create page content
    };

    PagedView();
    ~PagedView() = default;

    // UI Creation
    void create(lv_obj_t* parent);
    void destroy();

    // Page management
    void addPage(const PageInfo& page_info);
    void setPages(const std::vector<PageInfo>& pages);
    void goToPage(int page_index);
    void nextPage();
    void previousPage();

    // Configuration
    void setPageChangedCallback(PageChangedCallback callback) { page_changed_callback_ = callback; }
    void setButtonStyle(uint32_t bg_color, uint32_t border_color, uint32_t text_color);
    void setIndicatorStyle(uint32_t bg_color, uint32_t text_color);

    // Getters
    int getCurrentPage() const { return current_page_; }
    int getPageCount() const { return pages_.size(); }
    lv_obj_t* getContainer() const { return container_; }
    lv_obj_t* getCurrentPageContainer() const { return content_area_; }
    bool isCreated() const { return container_ != nullptr; }

private:
    void createNavigationBar();
    void createContentArea();
    void updatePageContent();
    void updatePageIndicator();
    void updateNavigationButtons();
    void clearCurrentPageContent();

    // Static event handlers
    static void onPreviousClicked(lv_event_t* e);
    static void onNextClicked(lv_event_t* e);

    // UI Elements
    lv_obj_t* container_;
    lv_obj_t* nav_bar_;
    lv_obj_t* prev_btn_;
    lv_obj_t* next_btn_;
    lv_obj_t* page_indicator_;
    lv_obj_t* content_area_;

    // State
    std::vector<PageInfo> pages_;
    int current_page_;
    PageChangedCallback page_changed_callback_;

    // Styling
    struct {
        uint32_t button_bg_color = 0x404040;
        uint32_t button_border_color = 0x808080;
        uint32_t button_text_color = 0xFFFFFF;
        uint32_t indicator_bg_color = 0x202020;
        uint32_t indicator_text_color = 0xCCCCCC;
    } style_;
};
