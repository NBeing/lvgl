#pragma once

#include "components/ui/Window.h"
#include "components/ui/Modal.h"
#include <memory>

/**
 * @brief Demo tab that demonstrates the Modal component
 * 
 * This tab shows how to use the Modal component with configurable overlays,
 * different content types, and various modal configurations.
 */
class WorldTab : public Tab {
public:
    WorldTab();
    virtual ~WorldTab() = default;

    // Window interface
    void create(lv_obj_t* parent) override;

protected:
    void onActivated() override;
    void onDeactivated() override;

private:
    void createModalButtons();
    void setupModals();

    // Modal event handlers
    void onSimpleModalOpen();
    void onSimpleModalClose();
    void onConfigModalOpen();
    void onInfoModalOpen();

    // Modal content creators
    static void createSimpleModalContent(lv_obj_t* parent);
    static void createConfigModalContent(lv_obj_t* parent);
    static void createInfoModalContent(lv_obj_t* parent);

    // UI Elements
    lv_obj_t* buttons_container_;
    lv_obj_t* simple_modal_btn_;
    lv_obj_t* config_modal_btn_;
    lv_obj_t* info_modal_btn_;

    // Modal components
    std::unique_ptr<Modal> simple_modal_;
    std::unique_ptr<Modal> config_modal_;
    std::unique_ptr<Modal> info_modal_;
};
