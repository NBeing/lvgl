#pragma once

#include <lvgl.h>
#include <memory>
#include <string>
#include <functional>

/**
 * @brief A reusable modal overlay component
 * 
 * This component provides:
 * - Overlay background that blocks interaction with underlying content
 * - Configurable modal container with custom dimensions
 * - Close button and escape key handling
 * - Event callbacks for open/close events
 * - Content area for any custom content
 * - Smooth show/hide animations (optional)
 */
class Modal {
public:
    using ModalCallback = std::function<void()>;
    using ContentCreator = std::function<void(lv_obj_t*)>;

    struct ModalConfig {
        int width_percent = 90;      // Percentage of screen width (1-100)
        int height_percent = 90;     // Percentage of screen height (1-100)
        bool show_close_button = true;
        bool close_on_background_click = true;
        bool close_on_escape = true;
        std::string title = "";
        uint32_t background_color = 0x000000;
        uint8_t background_opacity = 128;  // 0-255
        uint32_t modal_bg_color = 0x2A2A2A;
        uint32_t modal_border_color = 0x606060;
        uint32_t title_color = 0xFFFFFF;
    };

    Modal();
    ~Modal() = default;

    // UI Creation and Management
    void create(lv_obj_t* parent);
    void destroy();
    void show();
    void hide();
    bool isVisible() const { return is_visible_; }

    // Configuration
    void setConfig(const ModalConfig& config);
    void setContentCreator(ContentCreator creator) { content_creator_ = creator; }
    void setTitle(const std::string& title);
    
    // Event callbacks
    void setOnOpenCallback(ModalCallback callback) { on_open_callback_ = callback; }
    void setOnCloseCallback(ModalCallback callback) { on_close_callback_ = callback; }

    // Content management
    void setContent(lv_obj_t* content);
    void clearContent();
    lv_obj_t* getContentArea() const { return content_area_; }

    // Getters
    lv_obj_t* getContainer() const { return modal_container_; }
    bool isCreated() const { return overlay_ != nullptr; }

private:
    void createOverlay(lv_obj_t* parent);
    void createModalContainer();
    void createTitleBar();
    void createContentArea();
    void createCloseButton();
    void updateModalSize();
    void updateStyling();
    void centerModal();

    // Event handlers
    static void onOverlayClicked(lv_event_t* e);
    static void onCloseButtonClicked(lv_event_t* e);
    static void onKeyPressed(lv_event_t* e);

    // UI Elements
    lv_obj_t* overlay_;           // Full-screen background overlay
    lv_obj_t* modal_container_;   // Main modal container
    lv_obj_t* title_bar_;         // Title bar (if title is set)
    lv_obj_t* title_label_;       // Title text
    lv_obj_t* close_btn_;         // Close button
    lv_obj_t* content_area_;      // Content container

    // State
    ModalConfig config_;
    bool is_visible_;
    ContentCreator content_creator_;
    ModalCallback on_open_callback_;
    ModalCallback on_close_callback_;
};
