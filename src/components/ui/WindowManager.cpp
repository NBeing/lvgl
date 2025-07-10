#include "components/ui/WindowManager.h"
#include "components/layout/LayoutManager.h"
#include "Constants.h"
#include <algorithm>
#include <iostream>
#include <unordered_map>

// Static callback data storage
static std::unordered_map<lv_obj_t*, std::pair<WindowManager*, std::string>> tab_button_map;

WindowManager::WindowManager(lv_obj_t* root_container)
    : root_container_(root_container)
    , tab_bar_container_(nullptr)
    , content_area_(nullptr)
    , current_tab_(nullptr)
    , current_popup_(nullptr)
{
    std::cout << "WindowManager constructor called" << std::endl;
    
    if (!root_container_) {
        std::cout << "ERROR: root_container is null!" << std::endl;
        return;
    }
    
    // Debug root container size
    int root_width = lv_obj_get_width(root_container_);
    int root_height = lv_obj_get_height(root_container_);
    std::cout << "WindowManager root container size: " << root_width << "x" << root_height << std::endl;
    
    createTabBar();
    createContentArea();
    
    std::cout << "WindowManager layout creation complete" << std::endl;
}

WindowManager::~WindowManager() {
    // Clean up static callback map
    for (auto& tab : tabs_) {
        if (tab->getTabButton()) {
            tab_button_map.erase(tab->getTabButton());
        }
    }
}

void WindowManager::addTab(std::unique_ptr<Tab> tab) {
    if (!tab) return;
    
    std::cout << "Adding tab: " << tab->getName() << std::endl;
    
    // Create the tab's content in the content area
    tab->create(content_area_);
    
    // Create tab button in tab bar
    lv_obj_t* tab_button = lv_btn_create(tab_bar_container_);
    lv_obj_t* tab_label = lv_label_create(tab_button);
    lv_label_set_text(tab_label, tab->getName().c_str());
    lv_obj_center(tab_label);
    
    // Tab button size scales with the tab bar (which is 15% of root container)
    // Each button takes roughly 25% of tab bar width, 80% of tab bar height
    lv_obj_set_size(tab_button, LV_PCT(25), LV_PCT(80));
    lv_obj_set_style_bg_color(tab_button, lv_color_hex(SynthConstants::Color::BTN_FILTER_OFF), 0);
    lv_obj_set_style_text_color(tab_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_radius(tab_button, 4, 0);
    lv_obj_set_style_border_width(tab_button, 1, 0);
    lv_obj_set_style_border_color(tab_button, lv_color_hex(0xFF555555), 0);
    
    // Set up callback
    tab_button_map[tab_button] = {this, tab->getName()};
    lv_obj_add_event_cb(tab_button, tab_button_callback, LV_EVENT_CLICKED, nullptr);
    
    tab->setTabButton(tab_button);
    
    // If this is the first tab, make it active
    if (tabs_.empty()) {
        current_tab_ = tab.get();
        tab->setActive(true);
        notifyTabChanged("", tab->getName());
    }
    
    tabs_.push_back(std::move(tab));
    layoutTabBar();
}

void WindowManager::showTab(const std::string& tab_name) {
    Tab* new_tab = getTab(tab_name);
    if (!new_tab) {
        std::cout << "Warning: Tab '" << tab_name << "' not found" << std::endl;
        return;
    }
    
    if (new_tab == current_tab_) {
        return;  // Already active
    }
    
    std::string old_tab_name = current_tab_ ? current_tab_->getName() : "";
    
    // Deactivate current tab
    if (current_tab_) {
        current_tab_->setActive(false);
    }
    
    // Activate new tab
    current_tab_ = new_tab;
    current_tab_->setActive(true);
    
    notifyTabChanged(old_tab_name, tab_name);
    std::cout << "Switched to tab: " << tab_name << std::endl;
}

void WindowManager::hideTab(const std::string& tab_name) {
    Tab* tab = getTab(tab_name);
    if (tab) {
        tab->setActive(false);
        if (tab == current_tab_) {
            current_tab_ = nullptr;
        }
        notifyWindowHidden(tab_name);
    }
}

Tab* WindowManager::getTab(const std::string& name) const {
    auto it = std::find_if(tabs_.begin(), tabs_.end(),
        [&name](const std::unique_ptr<Tab>& tab) {
            return tab->getName() == name;
        });
    return (it != tabs_.end()) ? it->get() : nullptr;
}

void WindowManager::showPopup(std::unique_ptr<PopupWindow> popup) {
    if (!popup) return;
    
    // Hide current popup if any
    if (current_popup_) {
        hideCurrentPopup();
    }
    
    std::cout << "Showing popup: " << popup->getName() << std::endl;
    
    // Create and show the popup
    popup->create(root_container_);
    popup->show();
    
    current_popup_ = popup.get();
    popups_.push_back(std::move(popup));
    
    notifyPopupOpened(current_popup_->getName());
}

void WindowManager::hidePopup(const std::string& popup_name) {
    auto it = std::find_if(popups_.begin(), popups_.end(),
        [&popup_name](const std::unique_ptr<PopupWindow>& popup) {
            return popup->getName() == popup_name;
        });
    
    if (it != popups_.end()) {
        (*it)->hide();
        if (it->get() == current_popup_) {
            current_popup_ = nullptr;
        }
        notifyPopupClosed(popup_name);
        popups_.erase(it);
        std::cout << "Closed popup: " << popup_name << std::endl;
    }
}

void WindowManager::hideCurrentPopup() {
    if (current_popup_) {
        hidePopup(current_popup_->getName());
    }
}

void WindowManager::addObserver(WindowObserver* observer) {
    if (observer && std::find(observers_.begin(), observers_.end(), observer) == observers_.end()) {
        observers_.push_back(observer);
    }
}

void WindowManager::removeObserver(WindowObserver* observer) {
    observers_.erase(std::remove(observers_.begin(), observers_.end(), observer), observers_.end());
}

std::string WindowManager::getCurrentTabName() const {
    return current_tab_ ? current_tab_->getName() : "";
}

std::string WindowManager::getCurrentPopupName() const {
    return current_popup_ ? current_popup_->getName() : "";
}

void WindowManager::createTabBar() {
    if (!root_container_) {
        std::cout << "ERROR: createTabBar called with null root_container_!" << std::endl;
        return;
    }
    
    tab_bar_container_ = lv_obj_create(root_container_);
    
    // Tab bar takes 15% of the ROOT CONTAINER height
    lv_obj_set_size(tab_bar_container_, LV_PCT(100), LV_PCT(15));
    lv_obj_align(tab_bar_container_, LV_ALIGN_TOP_MID, 0, 0);
    
    // Style the tab bar
    lv_obj_set_style_bg_color(tab_bar_container_, lv_color_hex(0x404040), 0);
    lv_obj_set_style_border_color(tab_bar_container_, lv_color_hex(0x606060), 0);
    lv_obj_set_style_border_width(tab_bar_container_, 1, 0);
    lv_obj_set_style_pad_all(tab_bar_container_, 2, 0);  // Reduced padding
    
    // Set up flex layout for tab buttons
    lv_obj_set_layout(tab_bar_container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tab_bar_container_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tab_bar_container_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(tab_bar_container_, 5, 0);  // Reduced gap between buttons
    
    // Debug output
    int root_width = lv_obj_get_width(root_container_);
    int root_height = lv_obj_get_height(root_container_);
    int tab_bar_height = (root_height * 15) / 100;  // 15% of root height
    
    std::cout << "Tab bar created: " << root_width << "x" << tab_bar_height 
              << " (15% of root " << root_width << "x" << root_height << ")" << std::endl;
}

void WindowManager::createContentArea() {
    if (!root_container_ || !tab_bar_container_) {
        std::cout << "ERROR: createContentArea called with null containers!" << std::endl;
        return;
    }
    
    content_area_ = lv_obj_create(root_container_);
    
    // Content area takes 85% of the ROOT CONTAINER height
    lv_obj_set_size(content_area_, LV_PCT(100), LV_PCT(85));
    
    // Position below tab bar
    lv_obj_align_to(content_area_, tab_bar_container_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    
    // Style the content area
    lv_obj_set_style_bg_color(content_area_, lv_color_hex(SynthConstants::Color::BG), 0);
    lv_obj_set_style_border_width(content_area_, 0, 0);
    lv_obj_set_style_pad_all(content_area_, 0, 0);
    
    // Debug output
    int root_width = lv_obj_get_width(root_container_);
    int root_height = lv_obj_get_height(root_container_);
    int content_height = (root_height * 85) / 100;  // 85% of root height
    
    std::cout << "Content area created: " << root_width << "x" << content_height 
              << " (85% of root " << root_width << "x" << root_height << ")" << std::endl;
}

void WindowManager::layoutTabBar() {
    // LVGL flex layout will handle this automatically
    // This method is here for future customization if needed
}

void WindowManager::layoutContentArea() {
    // Content area always takes 85% of root container height
    lv_obj_set_size(content_area_, LV_PCT(100), LV_PCT(85));
    
    // Ensure it's positioned correctly below the tab bar
    lv_obj_align_to(content_area_, tab_bar_container_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
}

void WindowManager::updateLayout() {
    layoutTabBar();
    layoutContentArea();
}

void WindowManager::update() {
    // Update all active windows
    if (current_tab_) {
        current_tab_->update();
    }
    
    if (current_popup_) {
        current_popup_->update();
    }
}

// Static callback function
void WindowManager::tab_button_callback(lv_event_t* e) {
    lv_obj_t* button = static_cast<lv_obj_t*>(lv_event_get_target(e));
    
    auto it = tab_button_map.find(button);
    if (it != tab_button_map.end()) {
        WindowManager* manager = it->second.first;
        const std::string& tab_name = it->second.second;
        manager->handleTabButtonClick(tab_name);
    }
}

void WindowManager::handleTabButtonClick(const std::string& tab_name) {
    showTab(tab_name);
}

// Observer notification methods
void WindowManager::notifyWindowShown(const std::string& name) {
    for (auto observer : observers_) {
        observer->onWindowShown(name);
    }
}

void WindowManager::notifyWindowHidden(const std::string& name) {
    for (auto observer : observers_) {
        observer->onWindowHidden(name);
    }
}

void WindowManager::notifyTabChanged(const std::string& old_tab, const std::string& new_tab) {
    for (auto observer : observers_) {
        observer->onTabChanged(old_tab, new_tab);
    }
}

void WindowManager::notifyPopupOpened(const std::string& name) {
    for (auto observer : observers_) {
        observer->onPopupOpened(name);
    }
}

void WindowManager::notifyPopupClosed(const std::string& name) {
    for (auto observer : observers_) {
        observer->onPopupClosed(name);
    }
}
