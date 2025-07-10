#pragma once

#include <lvgl.h>
#include <string>
#include <memory>

/**
 * @brief Base class for all windows and tabs in the application
 * 
 * Provides common functionality for UI containers that can be shown/hidden,
 * positioned, and managed by the WindowManager.
 */
class Window {
public:
    enum class Type {
        TAB,        // Regular tab in the main interface
        POPUP,      // Modal popup window
        OVERLAY     // Non-modal overlay
    };

    Window(const std::string& name, Type type);
    virtual ~Window();

    // Core interface
    virtual void create(lv_obj_t* parent) = 0;
    virtual void show();
    virtual void hide();
    virtual void update() {}  // Called periodically for animations, etc.

    // Getters
    const std::string& getName() const { return name_; }
    Type getType() const { return type_; }
    lv_obj_t* getContainer() const { return container_; }
    bool isVisible() const { return is_visible_; }
    bool isCreated() const { return container_ != nullptr; }

    // State management
    virtual void onActivated() {}   // Called when window becomes active
    virtual void onDeactivated() {} // Called when window becomes inactive

protected:
    std::string name_;
    Type type_;
    lv_obj_t* container_;
    bool is_visible_;
    bool is_created_;

    // Helper for subclasses
    void setContainer(lv_obj_t* container);
};

/**
 * @brief Specialized window class for tabbed interfaces
 */
class Tab : public Window {
public:
    Tab(const std::string& name);
    virtual ~Tab() = default;

    // Tab-specific interface
    void setTabButton(lv_obj_t* button) { tab_button_ = button; }
    lv_obj_t* getTabButton() const { return tab_button_; }

    // Visual feedback for active/inactive state
    virtual void setActive(bool active);

protected:
    lv_obj_t* tab_button_;
    bool is_active_;
};

/**
 * @brief Specialized window class for popup dialogs
 */
class PopupWindow : public Window {
public:
    PopupWindow(const std::string& name, bool is_modal = true);
    virtual ~PopupWindow() = default;

    void show() override;
    void hide() override;

    bool isModal() const { return is_modal_; }

protected:
    bool is_modal_;
    lv_obj_t* backdrop_;  // Semi-transparent backdrop for modal popups

    void createBackdrop(lv_obj_t* parent);
    void destroyBackdrop();
};
