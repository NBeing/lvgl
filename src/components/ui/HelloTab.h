#pragma once

#include "components/ui/Window.h"
#include "components/ui/PagedView.h"
#include <memory>

/**
 * @brief Demo tab that demonstrates the PagedView component
 * 
 * This tab shows how to use the PagedView component with multiple pages
 * of different content types (text, controls, information panels).
 */
class HelloTab : public Tab {
public:
    HelloTab();
    virtual ~HelloTab() = default;

    // Window interface
    void create(lv_obj_t* parent) override;

protected:
    void onActivated() override;
    void onDeactivated() override;

private:
    void setupPages();
    void onPageChanged(int old_page, int new_page);

    // Page content creators
    static void createWelcomePage(lv_obj_t* parent);
    static void createControlsPage(lv_obj_t* parent);
    static void createInfoPage(lv_obj_t* parent);
    static void createSettingsPage(lv_obj_t* parent);

    std::unique_ptr<PagedView> paged_view_;
};
