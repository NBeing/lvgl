#pragma once

#include <lvgl.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>

class DialControl;
class Parameter;
class ParameterBinder;

struct DialDefinition {
    std::string parameter_name;
    std::string label;
    uint32_t color;
    int grid_col;
    int grid_row;
};

class ParameterDialsGrid {
public:
    using ParameterChangeCallback = std::function<void(uint8_t value, const Parameter* param)>;

    explicit ParameterDialsGrid(ParameterBinder* parameter_binder);
    ~ParameterDialsGrid() = default;

    // UI Creation
    void create(lv_obj_t* parent);
    void destroy();

    // Configuration
    void setDialDefinitions(const std::vector<DialDefinition>& definitions);
    void setParameterChangeCallback(ParameterChangeCallback callback) { parameter_change_callback_ = callback; }

    // Parameter binding
    void bindParameters();

    // Getters
    lv_obj_t* getContainer() const { return container_; }
    std::shared_ptr<DialControl> getDial(const std::string& parameter_name) const;
    bool isCreated() const { return container_ != nullptr; }

private:
    void setupGridLayout();
    void createDials();
    void bindDialToParameter(std::shared_ptr<DialControl> dial, const std::string& parameter_name);

    ParameterBinder* parameter_binder_;
    lv_obj_t* container_;
    std::vector<DialDefinition> dial_definitions_;
    std::vector<std::shared_ptr<DialControl>> dials_;
    ParameterChangeCallback parameter_change_callback_;
};
