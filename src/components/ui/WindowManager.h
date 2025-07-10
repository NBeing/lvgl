#pragma once

#include "components/ui/Window.h"
#include <lvgl.h>
#include <vector>
#include <memory>
#include <string>
#include <functional>

/**
 * @brief Observer interface for window state changes
 */
class WindowObserver {
public:
    virtual ~WindowObserver() = default;
    virtual void onWindowShown(const std::string& window_name) {}
    virtual void onWindowHidden(const std::string& window_name) {}
    virtual void onTabChanged(const std::string& old_tab, const std::string& new_tab) {}
    virtual void onPopupOpened(const std::string& popup_name) {}
    virtual void onPopupClosed(const std::string& popup_name) {}
};

/**
 * @brief Central manager for all windows, tabs, and popups
 * 
 * Implements a hybrid state management approach:
 * - Centralized state for easy debugging and persistence
 * - Observer pattern for cross-component communication
 * - Command pattern for actions
 */
class WindowManager {
public:
    WindowManager(lv_obj_t* root_container);
    ~WindowManager();

    // Tab management
    void addTab(std::unique_ptr<Tab> tab);
    void showTab(const std::string& tab_name);
    void hideTab(const std::string& tab_name);
    Tab* getCurrentTab() const { return current_tab_; }
    Tab* getTab(const std::string& name) const;

    // Popup management
    void showPopup(std::unique_ptr<PopupWindow> popup);
    void hidePopup(const std::string& popup_name);
    void hideCurrentPopup();
    PopupWindow* getCurrentPopup() const { return current_popup_; }

    // Observer pattern
    void addObserver(WindowObserver* observer);
    void removeObserver(WindowObserver* observer);

    // State queries
    std::string getCurrentTabName() const;
    std::string getCurrentPopupName() const;
    bool hasActivePopup() const { return current_popup_ != nullptr; }

    // Layout management
    void createTabBar();
    void updateLayout();  // Call when screen size changes

    // Update cycle (call from main loop)
    void update();

private:
    lv_obj_t* root_container_;
    lv_obj_t* tab_bar_container_;
    lv_obj_t* content_area_;
    
    // Window storage
    std::vector<std::unique_ptr<Tab>> tabs_;
    std::vector<std::unique_ptr<PopupWindow>> popups_;
    
    // Current state
    Tab* current_tab_;
    PopupWindow* current_popup_;
    
    // Observers
    std::vector<WindowObserver*> observers_;

    // Internal helpers
    void createContentArea();
    void layoutTabBar();
    void layoutContentArea();
    
    // Tab button callbacks
    static void tab_button_callback(lv_event_t* e);
    void handleTabButtonClick(const std::string& tab_name);
    
    // Observer notifications
    void notifyWindowShown(const std::string& name);
    void notifyWindowHidden(const std::string& name);
    void notifyTabChanged(const std::string& old_tab, const std::string& new_tab);
    void notifyPopupOpened(const std::string& name);
    void notifyPopupClosed(const std::string& name);
};
