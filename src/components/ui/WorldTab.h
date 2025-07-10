#pragma once

#include "components/ui/Window.h"

/**
 * @brief Simple demo tab that displays "World" in the center
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
    lv_obj_t* world_label_;
};
