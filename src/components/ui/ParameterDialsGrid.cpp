#include "ParameterDialsGrid.h"
#include "components/controls/DialControl.h"
#include "components/parameter/ParameterBinder.h"
#include "components/parameter/Parameter.h"
#include "Constants.h"
#include <iostream>

ParameterDialsGrid::ParameterDialsGrid(ParameterBinder* parameter_binder)
    : parameter_binder_(parameter_binder)
    , container_(nullptr)
{
}

void ParameterDialsGrid::create(lv_obj_t* parent) {
    if (container_) return; // Already created

    // Create container for parameter dials
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(container_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 2, 0);

    setupGridLayout();
    createDials();

    std::cout << "ParameterDialsGrid created with " << dials_.size() << " dials" << std::endl;
}

void ParameterDialsGrid::destroy() {
    if (container_) {
        dials_.clear();
        lv_obj_del(container_);
        container_ = nullptr;
    }
}

void ParameterDialsGrid::setDialDefinitions(const std::vector<DialDefinition>& definitions) {
    dial_definitions_ = definitions;
}

void ParameterDialsGrid::setupGridLayout() {
    // Set up grid layout for dials
    lv_obj_set_layout(container_, LV_LAYOUT_GRID);
    
    // Define grid template (3 columns, 2 rows)
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(container_, col_dsc, row_dsc);
}

void ParameterDialsGrid::createDials() {
    if (dial_definitions_.empty()) {
        std::cout << "Warning: No dial definitions provided" << std::endl;
        return;
    }

    dials_.clear();

    for (const auto& def : dial_definitions_) {
        auto dial = std::make_shared<DialControl>(container_, 0, 0);
        dial->setColor(lv_color_hex(def.color));
        
        // Position in grid
        lv_obj_set_grid_cell(dial->getObject(), 
                             LV_GRID_ALIGN_CENTER, def.grid_col, 1, 
                             LV_GRID_ALIGN_CENTER, def.grid_row, 1);
        
        dials_.push_back(dial);
        
        std::cout << "Created dial for " << def.parameter_name << " at grid position (" 
                  << def.grid_col << "," << def.grid_row << ")" << std::endl;
    }
}

void ParameterDialsGrid::bindParameters() {
    if (!parameter_binder_) {
        std::cout << "ERROR: ParameterBinder is null" << std::endl;
        return;
    }

    for (size_t i = 0; i < dials_.size() && i < dial_definitions_.size(); ++i) {
        bindDialToParameter(dials_[i], dial_definitions_[i].parameter_name);
    }

    std::cout << "ParameterDialsGrid bound " << dials_.size() << " parameters" << std::endl;
}

void ParameterDialsGrid::bindDialToParameter(std::shared_ptr<DialControl> dial, const std::string& parameter_name) {
    auto parameter = parameter_binder_->getParameter(parameter_name);
    if (!parameter) {
        std::cout << "Warning: Parameter '" << parameter_name << "' not found" << std::endl;
        return;
    }

    dial->bindParameter(parameter);
    
    // Set up value changed callback
    dial->setValueChangedCallback([this](uint8_t value, const Parameter* param) {
        if (parameter_change_callback_) {
            parameter_change_callback_(value, param);
        }
    });

    std::cout << "Bound dial to parameter: " << parameter_name << std::endl;
}

std::shared_ptr<DialControl> ParameterDialsGrid::getDial(const std::string& parameter_name) const {
    for (size_t i = 0; i < dial_definitions_.size() && i < dials_.size(); ++i) {
        if (dial_definitions_[i].parameter_name == parameter_name) {
            return dials_[i];
        }
    }
    return nullptr;
}
