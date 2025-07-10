#pragma once

#include "components/ui/Window.h"

/**
 * @brief Simple demo tab that displays "Hello" in the center
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
    lv_obj_t* hello_label_;
};
